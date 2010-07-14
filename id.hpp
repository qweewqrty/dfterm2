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
        ID(const std::string &s);
        ID(const char* buf, size_t buf_size);
        
        ID& operator=(const ID &i)
        { 
            if (this == &i) return (*this);
            
            for (trankesbel::ui8 i1 = 0; i1 < 64; i1++)
                id_value[i1] = i.id_value[i1];
            
            return (*this);
        };
        ID(const ID& i)
        {
            for (trankesbel::ui8 i1 = 0; i1 < 64; i1++)
                id_value[i1] = i.id_value[i1];
        }
        
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
        bool operator!=(const ID& id_value) const
        { return !((*this) == id_value); };

        bool operator<(const ID& id) const
        {
            for (trankesbel::ui8 i1 = 0; i1 < 64; i1++)
            {
                if (id_value[i1] == id.id_value[i1]) continue;
                if (id_value[i1] < id.id_value[i1]) return true; else return false;
            }
                
            return false;
        }
};

}

#endif
