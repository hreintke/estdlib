#include <catch.hpp>

#include <estd/tuple.h>

#include "test-data.h"

using namespace estd::test;

TEST_CASE("tuple")
{
    SECTION("std lib version")
    {
        using namespace std;

        tuple<int> t(5);

        REQUIRE(get<0>(t) == 5);
    }
    SECTION("estd lib version")
    {
        using namespace estd;

        tuple<int> t(5);

        REQUIRE(get<0>(t) == 5);
    }
    SECTION("value initialization")
    {
        using namespace estd;

        // As per https://en.cppreference.com/w/cpp/utility/tuple/tuple, tuples
        // value-initialize, which means default constructors shall run

        tuple<DefaultConstructor, int> t;

        const DefaultConstructor& dc = get<0>(t);

        REQUIRE(dc.val == DefaultConstructor::default_value());
    }
    SECTION("tuple size")
    {
        using namespace estd;

        SECTION("simple type")
        {
            tuple<bool, bool, bool> t(true, false, true);

            int sz = sizeof(t);

            REQUIRE(sz == 3);
        }
        SECTION("complex empty type")
        {
            tuple<EmptyClass, EmptyClass, EmptyClass> t;

            int sz = sizeof(t);

            // Cool, tuple_evaporator is doing its job.  Normally this would be a 3
            REQUIRE(sz == 1);
        }
        SECTION("intermixed types")
        {
            tuple<EmptyClass, Dummy, EmptyClass> t;

            int sz = sizeof(t);

            // on debian x64, this comes out to 16, since dummy has an int and a ptr which pads
            // up to two 8-byte words
            REQUIRE(sz == 16);
        }
    }
}
