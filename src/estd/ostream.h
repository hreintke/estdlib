#ifndef UTIL_EMBEDDED_OSTREAM_H
#define UTIL_EMBEDDED_OSTREAM_H

#if defined(ESP_OPEN_RTOS)
#else
// ESP_OPEN_RTOS has some non-sprintf ways to convert numeric to strings
#define USING_SPRINTF
#endif


extern "C" {

#if defined(USING_SPRINTF) || defined(ESTD_POSIX)
#include <inttypes.h>
#endif

}

#include "streambuf.h"
#include "ios.h"
#include "internal/string_convert.h"
#include "traits/char_traits.h"

namespace estd {

namespace internal {

//template<class TChar, class traits = std::char_traits<TChar>>
template <class TStreambuf, class TBase = basic_ios<TStreambuf> >
class basic_ostream :
#ifdef FEATURE_IOS_STREAMBUF_FULL
        virtual
#endif
        public TBase
{
    typedef TBase base_t;
    typedef typename TBase::char_type char_type;

public:
    struct sentry
    {
        explicit sentry(basic_ostream&) {}

        // NOTE: deviate from spec so that we don't risk extra stuff floating
        // around on stack.  This won't always be convenient though
        // NOTE also we might opt for a compile-time flag on unitbuf to further
        // optimize things
        inline static void destroy(basic_ostream& os)
        {
            //if(os.flags() & ios_base::unitbuf)
            if(os.is_unitbuf_set() && os.good())
                os.flush();
        }

        ~sentry()
        {

        }
    };

    typedef typename TBase::traits_type traits_type;

    typedef basic_ostream<TStreambuf, TBase> __ostream_type;

    __ostream_type& flush()
    {
        if(this->rdbuf()->pubsync() == -1)
            this->setstate(base_t::badbit);

        return *this;
    }

    // When the time comes, these will replace the old virtual ones
    __ostream_type& write(const char_type* s, streamsize n)
    {
        this->rdbuf()->sputn(s, n);
        sentry::destroy(*this);
        return *this;
    }


    __ostream_type& put(char_type ch, bool bypass_sentry = false)
    {
        if(this->rdbuf()->sputc(ch) == std::char_traits<char_type>::eof())
            this->setstate(base_t::eofbit);

        if(!bypass_sentry)
            sentry::destroy(*this);

        return *this;
    }

    //friend basic_ostream& operator<<(basic_ostream& (*__pf)(basic_ostream&));

    __ostream_type& operator<<(__ostream_type& (*__pf)(__ostream_type&))
    {
        return __pf(*this);
    }

    /*
    basic_ostream& operator<<(basic_ostream& (*__pf)(basic_ostream&))
    {
        return __pf(*this);
    }*/

#ifndef FEATURE_IOS_STREAMBUF_FULL
    //typedef typename base_t::stream_type stream_t;

    basic_ostream() {}

    template <class TParam1>
    basic_ostream(TParam1& p1) : base_t(p1) {}

    template <class TParam1>
    basic_ostream(TParam1* p1) : base_t(p1) {}
#endif

};

// Using TBase::char_type as it's the most reliable non-reference
template <class TStreambuf, class TBase>
inline basic_ostream<TStreambuf, TBase>& operator <<(basic_ostream<TStreambuf, TBase>& out,
                                                         const typename TBase::char_type* s)
{
    typedef typename TBase::traits_type traits_type;
    return out.write(s, traits_type::length(s));
}


// FIX: SFINAE works, but targets who aren't available for maxStringLength seem to be generating
// warnings here
template <class TStreambuf, class T,
          int N = internal::maxStringLength<T>(),
          class enabled = typename enable_if<(N > 1), bool>::type >
inline basic_ostream<TStreambuf>& operator<<(basic_ostream<TStreambuf>& out, T value)
{
    char buffer[N];

    internal::toString(buffer, value, sizeof(buffer) - 1);

    return out << buffer;
}



template <class TStreambuf, class TBase>
inline basic_ostream<TStreambuf, TBase>& operator <<(basic_ostream<TStreambuf, TBase>& out,
                                                        typename TBase::char_type ch)
{
    return out.put(ch);
}


template <class TStreambuf>
inline basic_ostream<TStreambuf>& operator<<(basic_ostream<TStreambuf>& out, void* addr)
{
    char buffer[sizeof(uintptr_t) * 3];

#ifdef ESP_OPEN_RTOS
    __utoa((uint32_t)addr, buffer, 16);
#else
    snprintf(buffer, sizeof(buffer), "%" PRIXPTR, (uintptr_t)addr);
#endif
    return out << buffer;
}


}

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class TChar, class CharTraits = std::char_traits<TChar> >
using basic_ostream = internal::basic_ostream<
    basic_streambuf<TChar, CharTraits>,
    internal::basic_ios<basic_streambuf<TChar, CharTraits>, true> >;

typedef basic_ostream<char> ostream;
#endif

namespace experimental {

// manage both a native TStreambuf as well as a pointer to a traditional-style
// basic_streambuf.  Note that this will auto wrap TStreambuf, because otherwise
// if TStreambuf didn't need wrapping, you wouldn't use wrapped_ostream in the first
// place (you'd instead use traditional basic_ostream)
template <class TStreambuf, class TBase =
        estd::internal::basic_ios<estd::basic_streambuf<
            typename estd::remove_reference<TStreambuf>::type::char_type,
            typename estd::remove_reference<TStreambuf>::type::traits_type
            >, true>
        >
struct wrapped_ostream : internal::basic_ostream<
        estd::basic_streambuf<
            typename TBase::char_type,
            typename TBase::traits_type>,
        TBase>
{
    typedef internal::basic_ostream<
        estd::basic_streambuf<
            typename TBase::char_type,
            typename TBase::traits_type>,
        TBase> base_type;

    // NOTE: Not well supported TStreambuf being a value vs a reference yet, needs work
    typedef estd::internal::basic_streambuf_wrapped<TStreambuf> streambuf_type;

    // need a value type here so that the wrapped streambuf has a place to live
    // i.e. be allocated
    streambuf_type wrapped_streambuf;

    wrapped_ostream(TStreambuf& native_streambuf) :
        base_type(&wrapped_streambuf),
        wrapped_streambuf(native_streambuf)
    {

    }
};


template <class TStreambuf, class TBase>
wrapped_ostream<TStreambuf&>
convert(internal::basic_ostream<TStreambuf, TBase>& os)
{
    wrapped_ostream<TStreambuf&> wrapped_os(*os.rdbuf());
    return wrapped_os;
}

}


template <class TStreambuf>
inline internal::basic_ostream<TStreambuf>& endl(internal::basic_ostream<TStreambuf>& os)
{
    // uses specialized call to bypass sentry so that we don't needlessly check
    // unitbuf and potentially double-flush
    return os.put(os.widen('\n'), true);
    os.flush();
}


template <class TStreambuf>
inline internal::basic_ostream<TStreambuf>& dec(internal::basic_ostream<TStreambuf>& __os)
{
    __os.flags((__os.flags() & ~ios_base::basefield) | ios_base::dec);
    return __os;
}

template <class TStreambuf>
inline internal::basic_ostream<TStreambuf>& hex(internal::basic_ostream<TStreambuf>& __os)
{
    __os.flags((__os.flags() & ~ios_base::basefield) | ios_base::hex);
    return __os;
}


// TODO: Put this in layer1/layer2 since it isn't the traditional/fully virtual-capable version
#ifdef ESTD_POSIX
template<class TChar, class Traits = std::char_traits<TChar> >
using posix_ostream = internal::basic_ostream< posix_streambuf<TChar, Traits> >;

//typedef posix_ostream<char> ostream;
#endif


}

#ifdef ARDUINO
#include "streams/ostream_arduino.h"
#endif

#endif //UTIL_EMBEDDED_OSTREAM_H
