#include "id.hpp"
#include <openssl/rand.h>
#include <string>
#include "hash.hpp"
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include <cstring>
#include "rng.hpp"

using namespace trankesbel;
using namespace dfterm;
using namespace std;

ID::ID()
{
    memset(id_value, 0, 64);
    makeRandomBytes( (trankesbel::ui8*) id_value, 64);
}

string ID::serialize() const
{
    return (string) bytes_to_hex(data1D((char*) id_value, 64));
}

void ID::serialize(std::string &s) const
{
    s = bytes_to_hex(data1D((char*) id_value, 64));
}

ID ID::getUnSerialized(const std::string &s)
{
    ID id_value;
    if (s.size() == 128)
    {
        string s2 = hex_to_bytes(s);
        memcpy(id_value.id_value, s2.c_str(), 64);
    }
    else
    {
        string hashed = hash_data(s);
        if (hashed.size() == 128)
        {
            hashed = hex_to_bytes(hashed);
            memcpy(id_value.id_value, hashed.c_str(), 64);
        }
        else
            makeRandomBytes( (trankesbel::ui8*) id_value.id_value, 64);
    }
    return id_value;
}

ID::ID(const std::string &s)
{
    if (s.size() == 128)
    {
        string s2 = hex_to_bytes(s);
        memcpy(this->id_value, s2.c_str(), 64);
    }
    else
    {
        string hashed = hash_data(s);
        if (hashed.size() == 128)
        {
            hashed = hex_to_bytes(hashed);
            memcpy(this->id_value, hashed.c_str(), 64);
        }
        else
            makeRandomBytes( (trankesbel::ui8*) this->id_value, 64);
    }
}

ID::ID(const char* s, size_t s_size)
{
    if (s_size == 128)
    {
        string s2 = hex_to_bytes(string(s, s_size));
        memcpy(this->id_value, s2.c_str(), 64);
    }
    else
    {
        string hashed = hash_data(string(s, s_size));
        if (hashed.size() == 128)
        {
            hashed = hex_to_bytes(hashed);
            memcpy(this->id_value, hashed.c_str(), 64);
        }
        else
            makeRandomBytes( (trankesbel::ui8*) this->id_value, 64);
    }
}

void ID::unSerialize(const std::string &s)
{
    if (s.size() == 128)
    {
        string s2 = hex_to_bytes(s);
        memcpy(this->id_value, s2.c_str(), 64);
    }
    else
    {
        string hashed = hash_data(s);
        if (hashed.size() == 128)
        {
            hashed = hex_to_bytes(hashed);
            memcpy(this->id_value, hashed.c_str(), 64);
        }
        else
            makeRandomBytes( (trankesbel::ui8*) this->id_value, 64);
    }
}

bool ID::operator==(const ID& id_value) const
{
    ui32 i1;
    for (i1 = 0; i1 < 64; ++i1)
        if (this->id_value[i1] != id_value.id_value[i1])
            return false;
    return true;
}
