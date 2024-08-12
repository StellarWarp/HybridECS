
//#include "test/test_delegate.h"
#include "test/test_auto_multicast.h"
#include "test/test_auto_delegate.h"
#include "test/test_auto_reference.h"
#include "test/test_weak_multicast.h"

using namespace std;


int main(int argc, char* argv[])
{
    test_auto_delegate::test();
    test_auto_multicast_delegate::test();
    test_auto_reference::test();
    test_weak_multicast_delegate::test();

    return 0;
}



