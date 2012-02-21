#ifndef address_settings_hpp
#define address_settings_hpp

#include <string>
#include <stdint.h>
#include "dfscreendataformat.hpp"

namespace dfterm
{

/* This class holds addresses and offsets for the purposes of
 * knowing where the screen data is stored inside DF. 
 *
 * At the moment it's 32-bit (DF is only available in 32 bits). */
class AddressSettings32
{
    public:
    /* The format. At the moment can only be PackedVarying. 
     * Other types are not handled with this class. */
    DFScreenDataFormat method;

    /* Where the size is located. */
    uint32_t size_address;
    /* Where the screen data address is located that points to the
     * actual address. */
    uint32_t screendata_address;

    /* Checksum of the DF process on which to use these addresses. */
    uint32_t checksum;

    /* And the name of process to which these apply. This is used only
     * for informative purposes so that people can see for what this
     * applies. */
    std::string name;
};

}

#endif

