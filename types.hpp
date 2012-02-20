#ifndef types_hpp
#define types_hpp


/* Various integer types and structures */
#include <boost/cstdint.hpp>

#include <unicode/unistr.h>
#include <unicode/umachine.h>

#include <boost/smart_ptr.hpp>
#include <string>

#include <map>
#include <set>

namespace trankesbel {

/* 64-bit wide integers. */
typedef boost::uint64_t ui64;
typedef boost::int64_t i64;

/* 32-bit wide integers. */
typedef boost::uint32_t ui32;
typedef boost::int32_t i32;

/* 16-bit wide integers. */
typedef boost::uint16_t ui16;
typedef boost::int16_t i16;

/* 8-bit wide integers. */
typedef boost::uint8_t ui8;
typedef boost::int8_t i8;

/* 32-but wide floating points. (used in dfterm memory reading) */
typedef float float32;

/* smart pointer (the current C++ doesn't support template typedefs so we have to go with a macro) */
#define SP boost::shared_ptr
#define WP boost::weak_ptr

/* Map/set types. Currently don't seem to work well so these
   are actually mapped to ordered types. */
#define UnorderedMap std::map
#define UnorderedSet std::set

/* some basic enums */
enum Direction { Left = 4, Top = 8, Right = 6, Bottom = 2, Up = 5, Down = 10,
                 TopRight = 9, TopLeft = 7,
                 BottomRight = 3, BottomLeft = 1 };

};

/* 1-dimensional 8-bit data-type. C++ strings work just fine for now. */
#define data1D std::string

extern std::string TO_UTF8(const UnicodeString &us);
extern inline std::string TO_UTF8(const std::string &us) { return us; };
extern std::string TO_UTF8(const wchar_t* wc);
extern std::string TO_UTF8(const wchar_t* wc, size_t wc_len);

/* Put wc_len to point to length of wc. It will be set to the actual length
   that was needed, which may be more (you can call this with 
   wc == null and (*wc_len == zero) to check how much space you need) */
extern void TO_WCHAR_STRING(const UnicodeString &us, wchar_t* wc, size_t* wc_len);
extern void TO_WCHAR_STRING(const std::string &us, wchar_t* wc, size_t* wc_len);

extern UnicodeString TO_UNICODESTRING(const std::string &us);
extern inline UnicodeString TO_UNICODESTRING(const UnicodeString &us) { return us; };

#ifdef _WIN32
    #ifndef __WIN32
    #define __WIN32
    #endif
    #ifndef _WIN32
    #define _WIN32
    #endif
#endif

#endif

