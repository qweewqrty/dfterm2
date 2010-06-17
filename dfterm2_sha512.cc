/* A program for testing SHA512 correctness used in dfterm2 */

#include "types.hpp"
#include "dfterm2_configuration.hpp"
#include <iostream>

using namespace dfterm;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2) return 0;

    data1D result = hash_data(argv[1]);
    cout << result << endl;
    LOG(Note, "unicode test: הצהצהצהו");
    
    return 0;
}

