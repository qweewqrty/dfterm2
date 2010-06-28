#include "hash.hpp"
#include <openssl/sha.h>
#include <cstring>

using namespace dfterm;

data1D dfterm::bytes_to_hex(const data1D &bytes)
{
    data1D result;
    result.reserve(bytes.size() * 2);

    unsigned int i1;
    for (i1 = 0; i1 < bytes.size(); i1++)
    {
        unsigned char c1 = ((bytes[i1] & 0xf0) >> 4) + '0';
        if (c1 > '9') c1 = c1 - '9' + 'A' - 1;
        unsigned char c2 = ((bytes[i1] & 0x0f) ) + '0';
        if (c2 > '9') c2 = c2 - '9' + 'A' - 1;

        result.push_back(c1);
        result.push_back(c2);
    }

    return result;
}

data1D dfterm::hash_data(const data1D &chunk)
{
    SHA512_CTX c;
    SHA512_Init(&c);
    SHA512_Update(&c, (void*) chunk.c_str(), chunk.size());
    unsigned char out_sha512[64];
    memset(out_sha512, 0, 64);
    SHA512_Final(out_sha512, &c);

    return dfterm::bytes_to_hex(data1D((char*) out_sha512, 64));
}

