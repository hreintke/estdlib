#include "unit-test.h"

#include <estd/thread.h>

void test_thread_get_id()
{
    estd::thread::id id = estd::this_thread::get_id();
    
    // pretty fundamental, just makes sure we're interacting
    // with *something* consistent
    TEST_ASSERT_EQUAL(id, estd::this_thread::get_id());
}


static void test_thread_ctor_fn(int param1)
{

}

void test_thread_ctor()
{
    // NOTE: This compiles -- and we don't want it to!  Be careful
    //estd::thread::thread spinup();

    estd::thread::thread(test_thread_ctor_fn, 1);
}


void test_thread()
{
    RUN_TEST(test_thread_get_id);
    RUN_TEST(test_thread_ctor);
}