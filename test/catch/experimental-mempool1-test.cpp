#include <catch.hpp>

#include <estd/exp/memory_pool_1.h>

using namespace estd::experimental1;

template <class TItem>
void basics_section()
{
    SECTION("basics")
    {
        CONSTEXPR std::size_t N = 10;
        typedef memory_pool<int, N, TItem> memory_pool_type;
        typedef typename memory_pool_type::item_type memory_pool_item_type;
        memory_pool_type pool;

        /*
        std::size_t sz = sizeof(pool);
        std::size_t sz_item = sizeof(memory_pool_type::item);
        */
        // DEBT: Test accounting for platform-specific padding
        //REQUIRE(sz_item == sizeof(int) + sizeof(void*));
        // DEBT: forward_list has a lingering allocator/traits floating in it that isn't optimized out
        //REQUIRE(sz == N * sz_item + sizeof(void*));

        SECTION("free slots")
        {
            auto free_slots = pool.free_slots();

            REQUIRE(free_slots == N);

            memory_pool_item_type& item = pool.reserve();

            REQUIRE(pool.free_slots() == N - 1);

            pool.release(item);

            REQUIRE(free_slots == N);
        }
        SECTION("assignments")
        {
            // NOTE: Was able to "reuse" item from earlier code at one point.  Unexpected from
            // a reference type
            memory_pool_item_type& item = pool.reserve();
            int* value = &item.value();

            item.value() = N;

            REQUIRE(*value == N);

            SECTION("get_item_from_tracked")
            {
                memory_pool_item_type* retrieved_item = pool.get_item_from_tracked(value);

                REQUIRE(retrieved_item == &item);
            }

            pool.release(item);
        }
        SECTION("free slots + assignments")
        {
            auto free_slots = pool.free_slots();

            REQUIRE(free_slots == N);

            memory_pool_item_type& item1 = pool.reserve();
            memory_pool_item_type& item2 = pool.reserve();
            memory_pool_item_type& item3 = pool.reserve();
            memory_pool_item_type& item4 = pool.reserve();

            item4.value() = N;

            REQUIRE(pool.free_slots() == N - 4);

            pool.release(item1);
            pool.release(item2);
            pool.release(item3);

            memory_pool_item_type& item5 = pool.reserve();

            REQUIRE(item4.value() == N);

            pool.release(item4);
            pool.release(item5);

            REQUIRE(free_slots == N);
        }
    }
}


// catch.hpp segfaults if we don't use two different test cases.  I speculate this
// is because two very different tests fall under the same "namespace" defined in
// basics_section via SECTION, etc.

TEST_CASE("experimental memory_pool_1 tests: type1")
{
    basics_section<memory_pool_item<int>::type_1>();
}

TEST_CASE("experimental memory_pool_1 tests: type2")
{
    basics_section<memory_pool_item<int>::type_2>();
}


TEST_CASE("experimental memory_pool_1 tests: type3")
{
    // forward_node_base_base constructor making this difficult, almost there
    //basics_section<memory_pool_item<int>::type_3>();
}
