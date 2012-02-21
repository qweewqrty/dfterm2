#include "utf8.h"
#include "logger.hpp"
#include "unicode/ucnv.h"

using namespace dfterm;

#ifndef WIN32
#include <langinfo.h>
void checkForUTF8Locale()
{
    const char* codeset;

    codeset = nl_langinfo(CODESET);
    if (strcmp(codeset, "UTF-8"))
    {
        LOG(Error, "The current locale is not a UTF-8 locale. "
                   "UTF-8 games (such as DF) may look weird and wrong.");
    }
    else
    {
        ucnv_setDefaultName("UTF-8");
    }
}
#endif

int getUTF8Str(unsigned int unicode_symbol, char* str)
{
    // If symbol can be encoded as ASCII character, so be it
    if (unicode_symbol < 0x80)
    {
        //                          0zzzzzzz
        str[0] = unicode_symbol;
        return 1;
    };
    // If the symbol is to be encoded in two bytes
    if (unicode_symbol >= 0x80 && unicode_symbol < 0x800)
    {
        //                 110yyyyy 10zzzzzz
        unsigned char first_byte = 0xC0 | ( (unicode_symbol & 0x7C0) >> 6);
        unsigned char second_byte = 0x80 | (unicode_symbol & 0x3F);
       
        str[0] = first_byte;
        str[1] = second_byte;
        return 2;
    };
    // If the symbol is to be encoded in three bytes (range 0xD800 - 0xDFFF is disallowed by unicode standard)
    if ( (unicode_symbol >= 0x800 && unicode_symbol <= 0xD7FF) ||
         (unicode_symbol >= 0xE000 && unicode_symbol <= 0xFFFF) )
    {
        //        1110xxxx 10yyyyyy 10zzzzzz
        unsigned char first_byte = 0xE0 | ( (unicode_symbol & 0xF000) >> 12);
        unsigned char second_byte = 0x80 | ( (unicode_symbol & 0xFC0) >> 6);
        unsigned char third_byte = 0x80 | ( (unicode_symbol & 0x3F) );

        str[0] = first_byte;
        str[1] = second_byte;
        str[2] = third_byte;
        return 3;
    };
    // If the symbol is to be encoded in four bytes
    if (unicode_symbol >= 0x10000 && unicode_symbol <= 0x10FFFF)
    {
        // 11110www 10xxxxxx 10yyyyyy 10zzzzzz
        unsigned char first_byte = 0xF0 | ( (unicode_symbol & 0x1C0000) >> 18);
        unsigned char second_byte = 0x80 | ( (unicode_symbol & 0x3F000) >> 12);
        unsigned char third_byte = 0x80 | ( (unicode_symbol & 0xFC0) >> 6);
        unsigned char fourth_byte = 0x80 | ( (unicode_symbol & 0x3F) );
       
        str[0] = first_byte;
        str[1] = second_byte;
        str[2] = third_byte;
        str[3] = fourth_byte;
        return 4;
    };

    return 0;
}

std::string getUTF8Str(unsigned int unicode_symbol)
{
    std::string result;
    
    // If symbol can be encoded as ASCII character, so be it
    if (unicode_symbol < 0x80)
    {
        //                          0zzzzzzz
        result.push_back((unsigned char) unicode_symbol);
        return result;
    };
    // If the symbol is to be encoded in two bytes
    if (unicode_symbol >= 0x80 && unicode_symbol < 0x800)
    {
        //                 110yyyyy 10zzzzzz
        unsigned char first_byte = 0xC0 | ( (unicode_symbol & 0x7C0) >> 6);
        unsigned char second_byte = 0x80 | (unicode_symbol & 0x3F);
        
        result.push_back(first_byte);
        result.push_back(second_byte);
        return result;
    };
    // If the symbol is to be encoded in three bytes (range 0xD800 - 0xDFFF is disallowed by unicode standard)
    if ( (unicode_symbol >= 0x800 && unicode_symbol <= 0xD7FF) ||
         (unicode_symbol >= 0xE000 && unicode_symbol <= 0xFFFF) )
    {
        //        1110xxxx 10yyyyyy 10zzzzzz
        unsigned char first_byte = 0xE0 | ( (unicode_symbol & 0xF000) >> 12);
        unsigned char second_byte = 0x80 | ( (unicode_symbol & 0xFC0) >> 6);
        unsigned char third_byte = 0x80 | ( (unicode_symbol & 0x3F) );
        
        result.push_back(first_byte);
        result.push_back(second_byte);
        result.push_back(third_byte);
        return result;
    };
    // If the symbol is to be encoded in four bytes
    if (unicode_symbol >= 0x10000 && unicode_symbol <= 0x10FFFF)
    {
        // 11110www 10xxxxxx 10yyyyyy 10zzzzzz
        unsigned char first_byte = 0xF0 | ( (unicode_symbol & 0x1C0000) >> 18);
        unsigned char second_byte = 0x80 | ( (unicode_symbol & 0x3F000) >> 12);
        unsigned char third_byte = 0x80 | ( (unicode_symbol & 0xFC0) >> 6);
        unsigned char fourth_byte = 0x80 | ( (unicode_symbol & 0x3F) );
        
        result.push_back(first_byte);
        result.push_back(second_byte);
        result.push_back(third_byte);
        result.push_back(fourth_byte);
        return result;
    };
    
    // Symbol is something not encodeable in UTF-8, return empty string
    return result;
}

unsigned int fetchUTF8Unicode(const char* str, int &cursor)
{
    unsigned char first_byte = (unsigned char) str[0];
    if (first_byte == 0)
        return 0;
        
    // One-byte character?
    if (!(first_byte & 0x80))
    {
        cursor++;
        return first_byte;
    };
    // Two-byte character?
    if ((first_byte & 0xE0) == 0xC0)
    {
        unsigned char second_byte = (unsigned char) str[1];
        if (!second_byte)
            return 0;
             
        if (!((second_byte & 0xC0) == 0x80)) // Is there an invalid sequence
        {
            cursor++;
            return 0;
        };
                
        cursor += 2;
        return ((((unsigned int) first_byte) & 0x1F) << 6) + 
               (((unsigned int) second_byte) & 0x3F);
    };
    // Three-byte character?
    if ((first_byte & 0xF0) == 0xE0)
    {
        unsigned char second_byte = (unsigned char) str[1];
        if (!second_byte) return 0;
        unsigned char third_byte = (unsigned char) str[2];
        if (!third_byte) return 0;
        
        if (!((second_byte & 0xC0) == 0x80) || !((third_byte & 0xC0) == 0x80))
        {
            cursor++;
            return 0;
        };
                
        cursor += 3;
        return ((((unsigned int) first_byte) & 0xF) << 12) + 
               ((((unsigned int) second_byte) & 0x3F) << 6) +
               (((unsigned int) third_byte) & 0x3F);        
    };
    // Four-byte character?
    if ((first_byte & 0xF8) == 0xF0)
    {
        unsigned char second_byte = (unsigned char) str[1];
        if (!second_byte) return 0;
        unsigned char third_byte = (unsigned char) str[2];
        if (!third_byte) return 0;
        unsigned char fourth_byte = (unsigned char) str[3];
        if (!fourth_byte) return 0;
        
        if (!((second_byte & 0xC0) == 0x80) || 
            !((third_byte & 0xC0) == 0x80) || 
            !((fourth_byte & 0xC0) == 0x80))
        {
            cursor++;
            return 0;
        };
        
        cursor += 4;
        return ((((unsigned int) first_byte) & 0x7) << 18) +
               ((((unsigned int) second_byte) & 0x3F) << 12) +
               ((((unsigned int) third_byte) & 0x3F) << 6) +
               (((unsigned int) fourth_byte) & 0x3F);  
    };
    
    // Some invalid character
    cursor++;
    return 0;
}

unsigned int getTrueLengthUTF8(const char* str)
{
    unsigned int true_length = 0;
    int cursor = 0;
    
    while(1)
    {
        unsigned int unicode_num = fetchUTF8Unicode(&str[cursor], cursor);
        if (unicode_num == 0)
            break;
        
        true_length++;
    };    
    
    return true_length;
};

std::string getPrefixUTF8(const char* str, int char_num)
{
    unsigned int true_length = 0;
    int cursor = 0;
    std::string result;
    
    while((unsigned int) char_num > true_length)
    {
        unsigned int unicode_num = fetchUTF8Unicode(&str[cursor], cursor);
        if (unicode_num == 0)
            break;
        
        result = result + getUTF8Str(unicode_num);
        true_length++;
    };    
    
    return result;
};

