#ifndef dfscreendataformat_hpp
#define dfscreendataformat_hpp

namespace dfterm
{

enum DFScreenDataFormat { 
                          DistinctFloatingPointVarying,   /* Varying size for 32-bit floating point buffers. */
                          PackedVarying,                  /* Varying size for 32-bit integer buffer. */
                          Packed256x256,                  /* Same as above, but limited to 256x256 buffer. */
                          Unknown };
}

#endif
