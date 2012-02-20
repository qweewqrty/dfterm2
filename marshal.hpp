#ifndef marshal_hpp
#define marshal_hpp

#include <string>
#include <sstream>
#include "types.hpp"

/* Various functions that marshal data. */

namespace trankesbel
{

inline data1D marshalUnsignedInteger32(ui32 integer)
{
    std::stringstream ss;
    ss << (unsigned char) ((integer & 0xff000000) >> 24);
    ss << (unsigned char) ((integer & 0x00ff0000) >> 16);
    ss << (unsigned char) ((integer & 0x0000ff00) >> 8);
    ss << (unsigned char) ((integer & 0x000000ff));
    return ss.str();
}

inline data1D marshalSignedInteger32(i32 integer)
{
    ui32 integer2 = (ui32) integer;
    if (integer < 0)
    {
        integer = -integer;
        integer2 = (ui32) integer | 0x80000000LL;
    }

    return marshalUnsignedInteger32(integer2);
}

inline data1D marshalUnsignedInteger64(ui64 integer)
{
    std::stringstream ss;
    ss << (unsigned char) ((integer & (ui64) 0xff00000000000000ULL) >> 56);
    ss << (unsigned char) ((integer & (ui64) 0x00ff000000000000ULL) >> 48);
    ss << (unsigned char) ((integer & (ui64) 0x0000ff0000000000ULL) >> 40);
    ss << (unsigned char) ((integer & (ui64) 0x000000ff00000000ULL) >> 32);
    ss << (unsigned char) ((integer & (ui64) 0x00000000ff000000ULL) >> 24);
    ss << (unsigned char) ((integer & (ui64) 0x0000000000ff0000ULL) >> 16);
    ss << (unsigned char) ((integer & (ui64) 0x000000000000ff00ULL) >> 8);
    ss << (unsigned char) ((integer & (ui64) 0x00000000000000ffULL));
    return ss.str();
}

inline data1D marshalSignedInteger64(i64 integer)
{
    ui64 integer2 = (ui64) integer;
    if (integer < 0)
    {
        integer = -integer;
        integer2 = (ui64) integer | 0x8000000000000000LL;
    }

    return marshalUnsignedInteger64(integer2);
}

inline data1D marshalString(const std::string &str)
{
    /* Let's do it very lazily. Marshal a 64-bit unsigned integer
       that tells the length of the string and then just put string after it. */

    return marshalUnsignedInteger64(str.size()) + str;
}




/* These functions set 'consumed' to the number of consumed octets
   from 'marshaled_data' that were used. It will be zero, if unmarshaling
   fails for some reason. 'start' refers to the point in 'marshaled_data' from
   where to start unmarshaling. */

inline ui32 unMarshalUnsignedInteger32(const data1D &marshaled_data, size_t start, size_t* consumed)
{
    assert(consumed);

    (*consumed) = 0;

    if (marshaled_data.size() - start < 4 || marshaled_data.size() - start > marshaled_data.size()) 
        return 0;

    (*consumed) = 4;
    const ui8* cstr = (ui8*) marshaled_data.c_str();

    return ((ui64) cstr[0+start] << 24) |
           ((ui64) cstr[1+start] << 16) |
           ((ui64) cstr[2+start] << 8) |
           ((ui64) cstr[3+start]);
}

inline i32 unMarshalSignedInteger32(const data1D &marshaled_data, size_t start, size_t* consumed)
{
    assert(consumed);

    ui32 integer = unMarshalUnsignedInteger32(marshaled_data, start, consumed);
    if ((*consumed) != 4) 
        return 0;

    if (integer & 0x80000000LL)
    {
        i32 integer2 = integer & 0x7FFFFFFFLL;
        return -integer2;
    }

    return (i64) integer;
}

inline ui64 unMarshalUnsignedInteger64(const data1D &marshaled_data, size_t start, size_t* consumed)
{
    assert(consumed);

    (*consumed) = 0;

    if (marshaled_data.size() - start < 8 || marshaled_data.size() - start > marshaled_data.size()) 
        return 0;

    (*consumed) = 8;
    const ui8* cstr = (ui8*) marshaled_data.c_str();

    return ((ui64) cstr[0+start] << 56) |
           ((ui64) cstr[1+start] << 48) |
           ((ui64) cstr[2+start] << 40) |
           ((ui64) cstr[3+start] << 32) |
           ((ui64) cstr[4+start] << 24) |
           ((ui64) cstr[5+start] << 16) |
           ((ui64) cstr[6+start] << 8) |
           ((ui64) cstr[7+start]);
}

inline i64 unMarshalSignedInteger64(const data1D &marshaled_data, size_t start, size_t* consumed)
{
    assert(consumed);

    ui64 integer = unMarshalUnsignedInteger64(marshaled_data, start, consumed);
    if ((*consumed) != 8) 
        return 0;

    if (integer & 0x8000000000000000LL)
    {
        i64 integer2 = integer & 0x7FFFFFFFFFFFFFFFLL;
        return -integer2;
    }

    return (i64) integer;
}

inline std::string unMarshalString(const data1D &data, size_t start, size_t* consumed)
{
    assert(consumed);

    (*consumed) = 0;

    ui64 len = unMarshalUnsignedInteger64(data, start, consumed);
    if ( (*consumed) == 0) 
        return std::string();

    if (data.size() - (*consumed) - start < len || data.size() - (*consumed) - start > data.size())
    {
        (*consumed) = 0;
        return std::string();
    }

    size_t consumed1 = (*consumed);
    (*consumed) += len;

    return data.substr(consumed1 + start, len);
}

inline ui32 unMarshalUnsignedInteger32(const data1D &data, size_t* consumed)
{ return unMarshalUnsignedInteger32(data, 0, consumed); };
inline i32 unMarshalSignedInteger32(const data1D &data, size_t* consumed)
{ return unMarshalSignedInteger32(data, 0, consumed); };
inline ui64 unMarshalUnsignedInteger64(const data1D &data, size_t* consumed)
{ return unMarshalUnsignedInteger64(data, 0, consumed); };
inline i64 unMarshalSignedInteger64(const data1D &data, size_t* consumed)
{ return unMarshalSignedInteger64(data, 0, consumed); };
inline std::string unMarshalString(const data1D &data, size_t* consumed)
{ return unMarshalString(data, 0, consumed); };

};

#endif

