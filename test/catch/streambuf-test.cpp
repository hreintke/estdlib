#include <estd/ios.h>

#include <catch.hpp>


TEST_CASE("streambuf")
{
    SECTION("A")
    {
        estd::basic_streambuf<char> s(*stdout);
        const char* t = "Test from streambuf\n";

        s.sputn(t, strlen(t));
    }
}
