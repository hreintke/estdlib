#include <catch.hpp>

#include <estd/exp/memory_pool_1.h>

using namespace estd::experimental1;

TEST_CASE("experimental memory_pool_1 tests")
{
    SECTION("basics")
    {
        memory_pool<int, 10> pool;

        std::size_t sz = sizeof(pool);
        std::size_t sz_item = sizeof(memory_pool<int, 10>::item);

        // DEBT: Test accounting for platform-specific padding
        //REQUIRE(sz_item == sizeof(int) + sizeof(void*));
        // DEBT: forward_list has a lingering allocator/traits floating in it that isn't optimized out
        //REQUIRE(sz == 10 * sz_item + sizeof(void*));
    }
}