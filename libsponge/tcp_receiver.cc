#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    WrappingInt32 _seqno = seg.header().seqno;
    if (!_isn.has_value()) {
        if (seg.header().syn) {
            _isn = _seqno;
        } else {
            return;
        }
    }
    if (!_fin.has_value()) {
        if (seg.header().fin) {
            _fin = _seqno;
        }
    }
    string _payload = seg.payload().copy();
    // if (!_ackno.has_value() || _seqno == _ackno.value())
    //     _ackno = _seqno + _payload.length() + seg.header().syn + seg.header().fin;
    size_t offset = int(!seg.header().syn);
    size_t index = unwrap(_seqno, _isn.value(), static_cast<uint64_t>(stream_out().bytes_written())) - offset;

    _reassembler.push_substring(_payload, index, seg.header().fin);

    uint64_t first_unassembled = static_cast<uint64_t>(stream_out().bytes_read() + stream_out().buffer_size());
    /* cout << "index : " << index << " "
     *      << "payload " << _payload << "\n";
     * printf("first_unassembled : %lu\n", first_unassembled);
     * printf("unassembled : %lu\n", unassembled_bytes()); */
    _ackno = wrap(first_unassembled + 1 +
                      (_fin.has_value() && first_unassembled  + 1>= unwrap(_fin.value(), _isn.value(), first_unassembled)),
                  _isn.value());
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
