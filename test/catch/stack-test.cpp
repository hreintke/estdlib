#include <catch.hpp>

#include <estd/stack.h>

#include "test-data.h"

TEST_CASE("stack")
{
    SECTION("layer1")
    {
        estd::layer1::stack<int, 10> s;

        typedef decltype (s)::size_type size_type;
        typedef decltype (s)::container_type container_type;

        s.push(1);
        s.push(2);
        s.push(3);

        int sz = sizeof(s);

        REQUIRE(sizeof(size_type) == 1);
        // FIX: Want this to come out 41, clearly alignment and padding are at play
        // this is not a specific failure, but kind of a 'loose screw' which I'd rather
        // not completely fall out and cause other problems
        REQUIRE(sizeof(container_type) == 44);
        //REQUIRE(sz == sizeof(int) * 10 + sizeof(void*) * 2);

        int counter = 0;

        for(auto& i : s)
        {
            REQUIRE(++counter == i);
        }

        REQUIRE(counter == 3);

        REQUIRE(s.top() == 3);
        s.pop();
        REQUIRE(s.top() == 2);
        s.pop();
        REQUIRE(s.top() == 1);
        s.pop();

        // underlying fixed structure does have a pseudo-handle, so it's 'locking'.
        // we need to split is_locking into something like is_pinned and is_handle
        // because even though 'is_handle' implies NOT pinned, that's not always the case
        //REQUIRE(!estd::internal::has_locking_tag<container_type::allocator_type>::value);
        //REQUIRE(!container_type::accessor::is_locking);

        REQUIRE(container_type::accessor::is_pinned);

        SECTION("aggressive push of accessor")
        {
            // NOTE: this test probably belongs in 'vector' area
            estd::layer1::stack<const estd::test::Dummy*, 10> s;
            estd::test::Dummy dummy1(7, "hi2u");

            s.push(&dummy1);
            // FIX: Need to clean this up so a lock() isn't required for layer1-layer3
            // (or other 'pinned' memory scenarios)
            const estd::test::Dummy* val = s.top();

            REQUIRE(val->val1 == 7);
            REQUIRE(s.top().lock()->val1 == 7);
            REQUIRE(s.top()->val1 == 7);

        }
    }
}
