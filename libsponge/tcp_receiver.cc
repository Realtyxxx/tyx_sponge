#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    WrappingInt32 _seqno = seg.header().seqno;
    if (!_isn.has_value()) {  // 处理syn
        if (seg.header().syn) {
            _isn = _seqno;
        } else {
            return;
        }
    }
    if (!_fin.has_value()) {  // 处理fin
        if (seg.header().fin) {
            _fin = _seqno;
        }
    }
    string _payload = seg.payload().copy();
    size_t offset = int(!seg.header().syn);
    size_t index = unwrap(_seqno, _isn.value(), static_cast<uint64_t>(stream_out().bytes_written())) - offset;
    // 因为syn也占用一个字节号,所以除了第一个之外其他的所有段都需要-1 得到index
    _reassembler.push_substring(_payload, index, seg.header().fin);

    uint64_t first_unassembled = static_cast<uint64_t>(stream_out().bytes_read() + stream_out().buffer_size()) + 1;
    // 还未组合的左边界
    bool add_fin = _fin.has_value() && first_unassembled >= unwrap(_fin.value(), _isn.value(), first_unassembled);
    // 如果用了最后这条包含了fin的段，那么需要将字节号+1
    _ackno = wrap(first_unassembled + add_fin, _isn.value());
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
