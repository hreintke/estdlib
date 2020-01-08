#include "unit-test.h"

#include <estd/thread.h>

void test_thread_get_id()
{
    estd::thread::id id = estd::this_thread::get_id();
    
    // pretty fundamental, just makes sure we're interacting
    // with *something* consistent
    TEST_ASSERT_EQUAL(id, estd::this_thread::get_id());
}


void test_thread()
{
    RUN_TEST(test_thread_get_id);
}