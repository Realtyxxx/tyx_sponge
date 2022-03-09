#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

#define PUSH(data, index, len)                                                                                         \
    size_t write_len = stream_out().write(data);                                                                       \
    _headIdx += write_len;                                                                                             \
    if (_eof && (_headIdx == _finIdx)) {                                                                               \
        stream_out().end_input();                                                                                      \
    }                                                                                                                  \
    if (write_len < len) {                                                                                             \
        _bufs.emplace_front(segment(move(string().assign(data, write_len, len - write_len)), index + write_len));      \
        _unassembled_bytes += len - write_len;                                                                         \
    }

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t len = data.length();
    if (!_eof && eof) {
        _eof = true;
        _finIdx = index + len;
    }
    size_t remaining_capacity = stream_out().remaining_capacity();
    // 在区间[first unread : first unacceptable]外的就丢弃
    if (len + index < _headIdx || index > _headIdx + remaining_capacity)
        return;
    // 限制头尾部
    const size_t start = index > _headIdx ? index : _headIdx;
    size_t end = index + len > _headIdx + remaining_capacity ? _headIdx + remaining_capacity : index + len;
    tryPush(data.substr(start - index, end - index), start);
}

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

void StreamReassembler::tryPush(const string &data, const size_t index) {
    const size_t len = data.length();
    if (index == _headIdx) {
        size_t write_len = stream_out().write(data);
        _headIdx += write_len;
        if (_eof && (_headIdx == _finIdx)) {  // 全部发完，end一下
            stream_out().end_input();
        }
        if (write_len < len) {  // 没足够空间全部写入,  存下 data 剩余部分 data[write_len : len];
            // store(move(string().assign(data, write_len, len - write_len)), index + write_len);
            _bufs.emplace_back(segment(move(string().assign(data, write_len, len - write_len)), index + write_len));
            _unassembled_bytes += len - write_len;  //加入缓存处就得 记录下
        }
    } else {
        _bufs.emplace_back(segment(move(string().assign(data, 0, len)), index + 0));
        _unassembled_bytes += len;  //加入缓存处就得 记录下
    }
    // 整理存储的 unassembled 数据 处理冗余数据
    deque<segment> tmp;
    sort(_bufs.begin(), _bufs.end());
    while (!_bufs.empty() && (_bufs.front()._index + _bufs.front()._data.length() < _headIdx)) { // 忘记了判断是否空
        _unassembled_bytes -= _bufs.front()._data.length();
        _bufs.pop_front();
    }
    if (_bufs.empty())
        return;
    tmp.emplace_back(_bufs.front());
    for (size_t i = 1; i < _bufs.size(); i++) {
        auto &back = tmp.back();
        if (back._index + back._data.length() < _bufs[i]._index) {
            // 前后不相交
            tmp.emplace_back(_bufs[i]);
        } else if (back._index <= _bufs[i]._index &&
                   back._index + back._data.length() >= _bufs[i]._index + _bufs[i]._data.length()) {  // 包含
            _unassembled_bytes -= _bufs[i]._data.length();
            continue;
        } else {  // 合并
            // const size_t add_idx = back._index + back._data.length() - _bufs[i]._index;
            // const size_t add_len = _bufs[i]._data.length() - (add_idx - _bufs[i]._index);
            // if (add_len > 0) {  // 不考虑 前一个包含第二个的情况, 处理重合部分
            //     back._data += string().assign(_bufs[i]._data, add_idx, add_len);
            //     _unassembled_bytes -= add_idx - _bufs[i]._index;
            // }
            const int32_t buf_length = _bufs[i]._data.length();
            const int32_t sub_begin = back._index + back._data.length() - _bufs[i]._index;
            const int32_t sub_len = buf_length - sub_begin;
            if (sub_begin >= 0 && sub_begin < buf_length && sub_len > 0 && sub_len <= buf_length) {
                back._data += _bufs[i]._data.substr(sub_begin, sub_len);
                _unassembled_bytes -= sub_begin;
            }
        }
    }
    swap(tmp, _bufs);
    // _bufs = tmp;
    if (!_bufs.empty() && _bufs.front()._index <= _headIdx) {  //如果可以发出就发出
        auto tmpSeg = move(_bufs.front());
        _bufs.pop_front();
        string str = tmpSeg._data;
        _unassembled_bytes -= str.length();
        size_t tmpIdx = tmpSeg._index;
        size_t tmpLen = str.length() - _headIdx + tmpIdx;
        string &&tmpStr = str.substr(_headIdx - tmpIdx, tmpLen);
        PUSH(tmpStr, _headIdx, tmpLen);
    }
}
