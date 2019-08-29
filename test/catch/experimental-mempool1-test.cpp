#include <catch.hpp>

#include <estd/exp/memory_pool_1.h>

using namespace estd::experimental1;

TEST_CASE("experimental memory_pool_1 tests")
{
    SECTION("basics")
    {
        CONSTEXPR std::size_t N = 10;
        typedef memory_pool<int, N> memory_pool_type;
        memory_pool_type pool;

        std::size_t sz = sizeof(pool);
        std::size_t sz_item = sizeof(memory_pool_type::item);

        // DEBT: Test accounting for platform-specific padding
        //REQUIRE(sz_item == sizeof(int) + sizeof(void*));
        // DEBT: forward_list has a lingering allocator/traits floating in it that isn't optimized out
        //REQUIRE(sz == N * sz_item + sizeof(void*));

        auto free_slots = pool.free_slots();

        REQUIRE(free_slots == N);

        memory_pool_type::item& item = pool.reserve();

        REQUIRE(pool.free_slots() == N - 1);

        pool.release(item);

        REQUIRE(free_slots == N);
    }
}