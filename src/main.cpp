
//#include "test/test_delegate.h"
#include "test/test_auto_multicast.h"
#include "test/test_auto_delegate.h"
//#include "test/test_auto_reference.h"

int main(int argc, char* argv[])
{
    test_auto_delegate::test();
    test_auto_multicast_delegate::test();
//    test_auto_reference::test();
    return 0;
}



