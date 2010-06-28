#ifndef hash_hpp
#define hash_hpp

#include "types.hpp"

namespace dfterm
{

/* Hash function (SHA512). This uses OpenSSL as backend. */

/* This one hashes the given data chunk and returns an ASCII data chunk
 * containing the hash in hex. */
data1D hash_data(const data1D &chunk);
/* Turns a byte chunk into an ASCII hex-encoded chunk */
data1D bytes_to_hex(const data1D &bytes);

}

#endif
