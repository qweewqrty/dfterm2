#include <vector>
#include <iostream>
#include "lockedresource.hpp"

using namespace std;
using namespace dfterm;

class data
{
    public:
        std::vector<int> values;

        void mushValue(int v2)
        {
            values.push_back((v2 + values.size() + 1) * (values.size()+1));
        }

        int getResult()
        {
            int i1;
            int total = 0;
            for (i1 = 0; i1 < values.size(); i1++)
                total += values[i1];
            return total;
        }
};

int main(int argc, char* argv[])
{
    /* Constructor/Destructor test */
    {
    LockedResource<data> lr_dummy;
    }

    /* operator & general workiness test */
    LockedResource<data> lr;

    LockedObject<data> lo = lr.lock();
    int i1;
    for (i1 = 0; i1 < 1000; i1++)
        lo->mushValue(i1+5);
    lo.release();

    {
    LockedObject<data> lo2 = lr.lock();
    for (i1 = 0; i1 < 1000; i1++)
        lo2->mushValue(i1 - 5);
    }

    LockedObject<data> lo3 = lr.lock();
    for (i1 = 0; i1 < 1111; i1++)
        lo3->mushValue( (i1+5)*3 );

    if (lo3->getResult() != -863889368)
        return 1;


    /* Copying tests */
    if (lo3.get() == (data*) 0)
        return 2;

    data* copy_addr = lo3.get();

    /* copy constructor */
    LockedObject<data> lo_copied(lo3);

    if (lo3.get() == (data*) 0)
        return 3;

    if (lo_copied.get() != copy_addr)
        return 4;

    /* operator= */
    lo3 = lo_copied;

    if (lo3.get() != copy_addr)
        return 5;

    if (lo_copied.get() != copy_addr)
        return 6;

    return 0;
}
