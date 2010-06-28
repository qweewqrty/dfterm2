#ifndef rng_hpp
#define rng_hpp

namespace dfterm
{

/* These functions make random bytes.
   You don't actually need to call seedRNG, it is called
   automatically by makeRandomBytes, if there's not enough entropy */

void seedRNG();
void makeRandomBytes(unsigned char* output, int output_size);

}

#endif
