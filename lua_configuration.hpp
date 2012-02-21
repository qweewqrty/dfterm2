#ifndef lua_configuration_hpp
#define lua_configuration_hpp

#include <string>
#include <vector>
#include "address_settings.hpp"

namespace dfterm
{

/* Reads from given lua script (conffile), what logfile and database file should be.
   UTF-8 for all fields. */
bool readConfigurationFile(const std::string &conffile, std::string* logfile,
                           std::string* database,
                           std::vector<AddressSettings32>* settings);

}

#endif
