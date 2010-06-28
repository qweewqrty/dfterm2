#ifndef id_hpp
#define id_hpp

#include "types.hpp"
#include <string>

namespace dfterm
{

/* Generates IDs that persist across
   process life time. They are internally 512 bit
   random byte sequences. Needs OpenSSL for secure random number
   generation and SHA512. */
class ID
{
    private:
        trankesbel::ui8 id_value[64];
        
    public:
        ID();
        ID(bool dont_initialize) { };
        ID(const std::string &s);
        ID(const char* buf, size_t buf_size);
        
        /* You can use these to convert the ID
           to an ASCII-string. The result will be a
           128 character long sequence. (64 bytes in hex) */
        std::string serialize() const;
        void serialize(std::string &s) const;
        std::string str() const { return serialize(); };
        
        /* And these unserialize the ID.
           If the string is in some way malformed,
           it will hash it with SHA512 to produce
           a valid ID. */
        static ID getUnSerialized(const std::string &s);
        void unSerialize(const std::string &s);
        
        bool operator==(const ID& id_value) const;
};

}

#endif
