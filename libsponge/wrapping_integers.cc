#include "wrapping_integers.hh"

#include <cmath>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

/* namespace mine {
 * //! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
 * //! \param n The input absolute 64-bit sequence number
 * //! \param isn The initial sequence number
 * WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
 *     DUMMY_CODE(n, isn);
 *     uint32_t ret = static_cast<uint32_t>((n + static_cast<uint64_t>(isn.raw_value())) % (1ll << 32));
 *     return WrappingInt32(ret);
 * }
 *
 * //! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
 * //! \param n The relative sequence number
 * //! \param isn The initial sequence number
 * //! \param checkpoint A recent absolute 64-bit sequence number
 * //! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
 * //! ! \note Each of the two streams of the TCP connection has its own ISN. One stream
 * //! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
 * //! and the other stream runs from the remote TCPSender to the local TCPReceiver and
 * //! has a different ISN.
 * uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
 *     uint64_t n64 = static_cast<uint64_t>(n.raw_value());
 *     uint64_t isn64 = static_cast<uint64_t>(isn.raw_value());
 *     uint64_t ret = (n64 >= isn64) ? n64 - isn64 : n64 + (1ul << 32) - isn64;
 *     ret += (checkpoint / (1ul << 32)) * (1ul << 32);
 *     if (ret > checkpoint && ret - checkpoint > (1ul << 31) && ret > (1ull << 32))
 *         return ret - (1ul << 32);
 *     else if (ret < checkpoint && checkpoint - ret > (1ul << 31) && ret < (1ull << 63))
 *         return ret + (1ul << 32);
 *     else
 *         return ret;
 * }
 * }  // namespace mine */

// others answer
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return WrappingInt32(isn + static_cast<uint32_t>(n)); }

uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    int32_t diff = n - wrap(checkpoint, isn);
    int64_t result = checkpoint + diff;
    return static_cast<uint64_t>((result >= 0) ? result : result + (1ll << 32));
}
