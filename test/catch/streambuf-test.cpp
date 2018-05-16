#include <estd/ios.h>
#include <estd/ostream.h>
#include <estd/string.h>

#include <catch.hpp>


//basic_streambuf - seems to be in the wrong namespace
template <class TChar, class TTraits, class TAllocator>
estd::basic_ostream<TChar>& operator <<(estd::basic_ostream<TChar>& os,
                                        const estd::basic_string<TChar, TTraits, TAllocator>& str )
{
    os.write(str.fake_const_lock(), str.size());
    str.fake_const_unlock();
    return os;
}


TEST_CASE("streambuf")
{
    SECTION("raw streambuf")
    {
        estd::basic_streambuf<char> s(*stdout);
        const char* t = "Test from streambuf\n";

        s.sputn(t, strlen(t));
    }
    SECTION("ostream 1")
    {
        estd::ostream o(*stdout);

        o << "Hi2u" << estd::endl;
    }
    SECTION("ostream 2")
    {
        estd::ostream o(*stdout);
        estd::layer1::string<128> s = "Hi2u:2";

        o << s << estd::endl;
    }
}
