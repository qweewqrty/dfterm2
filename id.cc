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
    ID id_value(false);
    if (s.size() == 64)
        memcpy(id_value.id_value, s.c_str(), 64);
    else
    {
        string hashed = hash_data(s);
        if (hashed.size() == 64)
            memcpy(id_value.id_value, hashed.c_str(), 64);
        else
            makeRandomBytes( (trankesbel::ui8*) id_value.id_value, 64);
    }
    return id_value.id_value;
}

ID::ID(const std::string &s)
{
    if (s.size() == 64)
        memcpy(this->id_value, s.c_str(), 64);
    else
    {
        string hashed = hash_data(s);
        if (hashed.size() == 64)
            memcpy(this->id_value, hashed.c_str(), 64);
        else
            makeRandomBytes( (trankesbel::ui8*) this->id_value, 64);
    }
}

ID::ID(const char* s, size_t s_size)
{
    if (s_size == 64)
        memcpy(this->id_value, s, 64);
    else
    {
        string hashed = hash_data(data1D(s, s_size));
        if (hashed.size() == 64)
            memcpy(this->id_value, hashed.c_str(), 64);
        else
            makeRandomBytes( (trankesbel::ui8*) this->id_value, 64);
    }
}

void ID::unSerialize(const std::string &s)
{
    if (s.size() == 64)
        memcpy(this->id_value, s.c_str(), 64);
    else
    {
        string hashed = hash_data(s);
        if (hashed.size() == 64)
            memcpy(this->id_value, hashed.c_str(), 64);
        else
            makeRandomBytes( (trankesbel::ui8*) this->id_value, 64);
    }
}

bool ID::operator==(const ID& id_value) const
{
    ui32 i1;
    for (i1 = 0; i1 < 64; i1++)
        if (this->id_value[i1] != id_value.id_value[i1])
            return false;
    return true;
}
