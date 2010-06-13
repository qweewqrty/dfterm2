#include <sstream>
#include <string>
#include "dfterm2_configuration.hpp"

using namespace std;
using namespace dfterm;

data1D UserGroup::serialize() const
{
    stringstream ss;
    ss << (has_nobody ? 'Y' : 'N');
    ss << (has_anybody ? 'Y' : 'N');
    ss << (has_launcher ? 'Y' : 'N');

    set<UnicodeString>::iterator i1;
    for (i1 = has_user.begin(); i1 != has_user.end(); i1++)
    {
        string user_utf8;
        (*i1).toUTF8String(user_utf8);

        size_t i2, len = user_utf8.size();
        for (i2 = 0; i2 < len; i2++)
            if (user_utf8[i2] != ';' && user_utf8[i2] != '\\')
                ss << user_utf8[i2];
            else if (user_utf8[i2] == ';')
                ss << "\\;";
            else if (user_utf8[i2] == '\\')
                ss << "\\\\";
        ss << ";";
    }

    return ss.str();
};

void UserGroup::unSerialize(data1D data)
{
    if (data.size() < 3) return;

    has_user.clear();

    bool has_nobody = (data[0] == 'Y');
    bool has_anybody = (data[1] == 'Y');
    bool has_launcher = (data[2] == 'Y');

    size_t i1, len = data.size();
    string name_utf8;
    for (i1 = 3; i1 < len; i1++)
    {
        char c = data[i1];
        if (c == '\\' && i1 < len-1)
        {
            name_utf8.push_back(data[i1+1]);
            i1++;
            continue;
        }
        if (c == ';')
        {
            has_user.insert(UnicodeString::fromUTF8(name_utf8));
            name_utf8.clear();
            continue;
        }
        name_utf8.push_back(c);
    }
}

