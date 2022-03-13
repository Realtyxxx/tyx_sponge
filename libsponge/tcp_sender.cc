#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ack_seqno; }
// 使自己发出的一定会遵守window_size的原则,那么就只需要检查这个之间的差距就可以知道已经发出了多少的字节流

void TCPSender::fill_window() {
    if (_window_size == 0 || _fin_sent)
        return;
    TCPSegment seg;
    if (!_syn_sent) {
        seg.header().syn = true;
        seg.header().seqno = next_seqno();
        _next_seqno++;
        _window_size--;
        segments_out().push(seg);
        outstandings().push(seg);
        _syn_sent = true;
        return;
    } else if (stream_in().eof()) {
        seg.header().fin = true;
        seg.header().seqno = next_seqno();
        _next_seqno++;
        _window_size--;
        segments_out().push(seg);
        outstandings().push(seg);
        _fin_sent = true;
        return;
    } else {
        while (_window_size > 0 && stream_in().buffer_size()) {
            seg.header().seqno = next_seqno();
            uint64_t read_size = min(min(TCPConfig::MAX_PAYLOAD_SIZE, stream_in().buffer_size()), _window_size);
            seg.payload() = stream_in().read(read_size);
            // printf("payload size == %lu\n", seg.payload().size());
            if (seg.length_in_sequence_space() < _window_size && stream_in().eof() && stream_in().buffer_empty()) {
                _fin_sent = true;
                seg.header().fin = true;
            }
            _next_seqno += seg.length_in_sequence_space();
            _window_size -= seg.length_in_sequence_space();
            segments_out().push(seg);
            outstandings().push(seg);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ack_abs = unwrap(ackno, _isn, next_seqno_absolute());
    // _window_size = window_size;
    uint32_t tmp_win = window_size;
    // changed here
    _window_size =  (ackno +  tmp_win)- next_seqno();
    if (ack_abs > _next_seqno || ack_abs <= _ack_seqno)  // 无效确认号
        return;

    _retransmission_timeout = _initial_retransmission_timeout;  // 如果有效就需要重置重传时限
    _consecutive_retransmissions = 0;

    while (!outstandings().empty()) {  // 清理发出的备份队列
        TCPSegment front = outstandings().front();
        uint64_t seqno = unwrap(front.header().seqno, _isn, _next_seqno) + front.length_in_sequence_space();
        // 一个文件是否被检查就看这个文件字节号末尾是否已经被确认
        if (seqno <= ack_abs) {
            _ack_seqno = seqno;
            outstandings().pop();
        } else {
            break;
        }
    }

    if (outstandings().empty()) {  // 没发出东西的话计时器就可以关了
        _timer_running = false;
    } else {  // 否则继续
        _total_time = 0;
        _timer_running = true;
        if (bytes_in_flight() > window_size) {
            _window_size = 0;
            _win_0_flag = true;
            return;
        }
    }
    if (window_size == 0) {  // 如果发过来说明没有空间了
        _window_size = 1;
        _win_0_flag = true;
    } else {
        _win_0_flag = false;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _total_time += ms_since_last_tick;

    if (_total_time >= _retransmission_timeout && !outstandings().empty()) {
        // 需要重传最早发出
        segments_out().push(outstandings().front());
        if (!_win_0_flag) {
            _consecutive_retransmissions++;
            _retransmission_timeout *= 2;
        }
        _total_time = 0;
    }
    if (outstandings().empty()) {
        _timer_running = false;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty;
    empty.header().seqno = next_seqno();
    segments_out().push(empty);
}

// TODO: 我感觉这里面的window_size 可能有点儿小小的问题,这里直接把window_size
// 作为了新的window_size,而实际上这个只是指明了还可以接受的多少：！！这个窗口大小应当是ackno + window_size - next_seqno
