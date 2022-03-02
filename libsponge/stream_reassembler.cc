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
    if (_eof && (index + write_len == _finIdx)) {                                                                      \
        stream_out().end_input();                                                                                      \
    }                                                                                                                  \
    if (write_len < len) {                                                                                             \
        _bufs.emplace_back(segment(move(string().assign(data, write_len, len - write_len)), index + write_len));       \
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
    // 在区间[first unread : first unacceptable]外的就丢弃
    if (len + index < _headIdx || index > _headIdx + _capacity)
        return;
    // 限制头尾部
    const size_t idx = index > _headIdx ? index : _headIdx;
    const size_t length = idx - index + len > _headIdx + _capacity ? _headIdx + _capacity : idx - index + len;
    tryPush(move(string().assign(data, (idx - index), length)), idx);
}

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

void StreamReassembler::tryPush(const string &&data, const size_t index) {
    const size_t len = data.length();
    if (index == _headIdx) {
        size_t write_len = stream_out().write(data);
        _headIdx += write_len;
        if (_eof && (index + write_len == _finIdx)) {  // 全部发完，end一下
            stream_out().end_input();
        }
        if (write_len < len) {  // 没足够空间全部写入,  存下 data 剩余部分 data[write_len : len];
            // store(move(string().assign(data, write_len, len - write_len)), index + write_len);
            _bufs.emplace_back(segment(move(string().assign(data, write_len, len - write_len)), index + write_len));
        }
    }
    if (_bufs.empty())
        return;
    // 整理存储的 unassembled 数据 处理冗余数据
    deque<segment> tmp;
    sort(_bufs.begin(), _bufs.end());
    tmp.emplace_back(_bufs.front());
    for (size_t i = 1; i < _bufs.size(); i++) {
        auto &back = tmp.back();
        if (back._index + back._data.length() < _bufs[i]._index) {
            // 前后不想交
            tmp.emplace_back(_bufs[i]);
        } else {
            const size_t add_idx = back._index + back._data.length();
            const size_t add_len = _bufs[i]._data.length() - (add_idx - _bufs[i]._index);
            if (add_len > 0)  // 不考虑 前一个包含第二个的情况
                back._data += string().assign(_bufs[i]._data, add_idx, add_len);
        }
    }
    swap(tmp, _bufs);
    while (!_bufs.empty() && _bufs.front()._index <= _headIdx) {
        string str = _bufs.front()._data;
        size_t tmpIdx = _bufs.front()._index;
        size_t tmpLen = str.length() - _headIdx + tmpIdx;
        string &&tmpStr = str.substr(_headIdx - tmpIdx, tmpLen);
        PUSH(tmpStr, _headIdx, tmpLen);
        _bufs.pop_front();
    }
}

// void StreamReassembler::store(const std::string &&data, size_t index) { while () }
