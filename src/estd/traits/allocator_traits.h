#pragma once

namespace estd {

template <class TAllocator>
struct allocator_traits;

}

#include "../type_traits.h"
#include "../utility.h"
#ifdef FEATURE_STD_MEMORY
#include <memory> // for std::allocator
#endif
#include <stdint.h> // for uint8_t and friends
#include "../internal/deduce_fixed_size.h"
#include "../internal/handle_with_offset.h"

namespace estd {

namespace internal {

struct has_member_base
{
    // For the compile time comparison.
    typedef char yes[1];
    typedef yes no[2];

    // This helper struct permits us to check that serialize is truly a method.
    // The second argument must be of the type of the first.
    // For instance reallyHas<int, 10> would be substituted by reallyHas<int, int 10> and works!
    // reallyHas<int, &C::serialize> would be substituted by reallyHas<int, int &C::serialize> and fail!
    // Note: It only works with integral constants and pointers (so function pointers work).
    // In our case we check that &C::serialize has the same signature as the first argument!
    // reallyHas<std::string (C::*)(), &C::serialize> should be substituted by
    // reallyHas<std::string (C::*)(), std::string (C::*)() &C::serialize> and work!
    template <typename U, U u> struct reallyHas;
};

// lifted directly from https://jguegant.github.io/blogs/tech/sfinae-introduction.html
template <class T> struct hasSerialize : has_member_base
{
    // Two overloads for yes: one for the signature of a normal method, one is for the signature of a const method.
    // We accept a pointer to our helper struct, in order to avoid to instantiate a real instance of this type.
    // std::string (C::*)() is function pointer declaration.
    template <typename C> static yes& test(reallyHas<std::string (C::*)(), &C::serialize>* /*unused*/) { }
    template <typename C> static yes& test(reallyHas<std::string (C::*)() const, &C::serialize>* /*unused*/) { }

    // The famous C++ sink-hole.
    // Note that sink-hole must be templated too as we are testing test<T>(0).
    // If the method serialize isn't available, we will end up in this method.
    template <typename> static no& test(...) { /* dark matter */ }

    // The constant used as a return value for the test.
    // The test is actually done here, thanks to the sizeof compile-time evaluation.
    static CONSTEXPR bool value = sizeof(test<T>(0)) == sizeof(yes);
};


template <class T> struct has_construct_method : has_member_base
{
    template <typename C> static yes& test(reallyHas<std::string (C::*)(), &C::construct>* /*unused*/) { }
    // The famous C++ sink-hole.
    // Note that sink-hole must be templated too as we are testing test<T>(0).
    // If the method serialize isn't available, we will end up in this method.
    template <typename> static no& test(...) { /* dark matter */ }

    // The constant used as a return value for the test.
    // The test is actually done here, thanks to the sizeof compile-time evaluation.
    static CONSTEXPR bool value = sizeof(test<T>(0)) == sizeof(yes);
};


template <class TAlloc2, class T, class... TArgs>
static typename estd::enable_if<internal::has_construct_method<TAlloc2>::value, int>::type
    construct_sfinae(TAlloc2& a, T* p, TArgs&&... args)
{
    a.construct(p, std::forward<TArgs>(args)...);
    return true;
}

// no construct method in underlying allocator, so use our own placement new
template <class TAlloc2, class T, class... TArgs>
static typename estd::enable_if<!internal::has_construct_method<TAlloc2>::value, int>::type
    construct_sfinae(TAlloc2& a, T* p, TArgs&&... args)
{
    new (static_cast<void*>(p)) T(std::forward<TArgs>(args)...);
    return false;
}



// experimental feature, has_typedef (lifted from PGGCC-13)
template<typename>
struct has_typedef
{
    typedef void type;
};

// since < c++11 can't do static constexpr bool functions, we need an alternate
// way to build up specializations based on attribute_traits.  Also has benefit
// of easier SFINAE application for allocators who don't have tags present

template<typename T, typename = void>
struct has_locking_tag : estd::false_type {};

template<typename T>
struct has_locking_tag<T, typename estd::internal::has_typedef<typename T::is_locking_tag>::type> : estd::true_type {};


template<typename T, typename = void>
struct has_stateful_tag : estd::false_type {};

template<typename T>
struct has_stateful_tag<T, typename estd::internal::has_typedef<typename T::is_stateful_tag>::type> : estd::true_type {};

template<typename T, typename = void>
struct has_singular_tag : estd::false_type {};

template<typename T>
struct has_singular_tag<T, typename estd::internal::has_typedef<typename T::is_singular_tag>::type> : estd::true_type {};

template<typename T, typename = void>
struct has_size_tag : estd::false_type {};

template<typename T>
struct has_size_tag<T, typename estd::internal::has_typedef<typename T::has_size_tag>::type> : estd::true_type {};

template<typename T, typename = void>
struct has_difference_type : estd::false_type {};

template<typename T>
struct has_difference_type<T, typename estd::internal::has_typedef<typename T::difference_type>::type> : estd::true_type {};

}

namespace experimental {


// FIX: eventually use something a bit like our Range<bool> trick in the fixed_size_t finder
template <class TAllocator, bool is_locking>
struct locking_allocator_traits;

// interact with actual underlying locking allocator
template <class TAllocator>
struct locking_allocator_traits<TAllocator, true>
{
    typedef TAllocator                          allocator_type;
    typedef typename TAllocator::value_type     value_type;
    typedef typename TAllocator::pointer        pointer;
    typedef typename TAllocator::size_type      size_type;
    typedef typename TAllocator::handle_type            handle_type;

    // handle_with_size is phased out with advent of has_size_tag coupled with
    // outside-of-allocator specializations
    //typedef typename TAllocator::handle_with_size       handle_with_size;
    typedef typename TAllocator::handle_with_offset     handle_with_offset;

    typedef value_type&                         reference; // deprecated in C++17 but relevant for us due to lock/unlock

    static reference lock(allocator_type& a, handle_type h, size_type pos = 0, size_type count = 0)
    {
        return a.lock(h, pos, count);
    }

    static reference lock(allocator_type &a, handle_with_offset h, size_type pos = 0, size_type count = 0)
    {
        return a.lock(h, pos, count);
    }

    static const value_type& clock(const allocator_type& a, handle_type h, size_type pos = 0, size_type count = 0)
    {
        return a.clock(h, pos, count);
    }

    static const value_type& clock(const allocator_type& a, const handle_with_offset& h, size_type pos = 0)
    {
        return a.clock(h, pos);
    }

    static void unlock(allocator_type& a, handle_type h)
    {
        a.unlock(h);
    }

    static void cunlock(const allocator_type& a, handle_type h)
    {
        a.cunlock(h);
    }
};


// shim for no locking activities
template <class TAllocator>
struct locking_allocator_traits<TAllocator, false>
{
    typedef TAllocator                          allocator_type;
    typedef typename TAllocator::value_type     value_type;
    typedef typename TAllocator::pointer        pointer;
    typedef typename TAllocator::size_type      size_type;
    typedef typename TAllocator::pointer        handle_type;

    // handle_with_size is phased out with advent of has_size_tag coupled with
    // outside-of-allocator specializations
    //typedef typename TAllocator::handle_with_size       handle_with_size;

    typedef estd::internal::handle_with_offset_raw<pointer>    handle_with_offset;

    typedef value_type&                         reference; // deprecated in C++17 but relevant for us due to lock/unlock

    static reference lock(allocator_type& a, handle_type h, size_type pos = 0, size_type count = 0)
    {
        return *(h + pos);
    }

    static reference lock(allocator_type &a, handle_with_offset h, size_type pos = 0, size_type count = 0)
    {
        return *(h.handle() + pos);
        //return a.lock(h, pos, count);
    }

    static const value_type& clock(const allocator_type& a, handle_type h, size_type pos = 0, size_type count = 0)
    {
        return *(h + pos);
    }

    static reference clock(const allocator_type &a, handle_with_offset h, size_type pos = 0, size_type count = 0)
    {
        return *(h.handle() + pos);
    }

    static void unlock(allocator_type& a, handle_type h)
    {
    }

    static void cunlock(const allocator_type& a, handle_type h)
    {
    }
};


}

// NOTE: May very well be better off using inbuilt version and perhaps extending it with
// our own lock mechanism
// NOTE: I erroneously made our burgeouning custom allocators not-value_type aware
template <class TAllocator>
struct allocator_traits
#ifdef FEATURE_ESTD_STRICT_DYNAMIC_ARRAY
        :
        experimental::locking_allocator_traits<TAllocator, internal::has_locking_tag<TAllocator>::value >
#endif
{
    typedef TAllocator                          allocator_type;
    typedef typename TAllocator::value_type     value_type;
    typedef typename TAllocator::pointer        pointer;
    typedef typename TAllocator::size_type      size_type;
    // FIX: Do a SFINAE extraction of difference type
    // doesn't work, still tries to resolve allocator_type::difference_type always
    /*
    typedef estd::conditional<
        experimental::has_difference_type<allocator_type>::value,
            typename allocator_type::difference_type,
            estd::make_signed<size_type> >
        difference_type; */

    typedef value_type&                         reference; // deprecated in C++17 but relevant for us due to lock/unlock

    // non-standard, for handle based scenarios
    typedef typename TAllocator::handle_type            handle_type;
    //typedef typename TAllocator::handle_with_size       handle_with_size;
    typedef typename TAllocator::handle_with_offset     handle_with_offset;
    typedef typename allocator_type::const_void_pointer     const_void_pointer;

    //typedef typename allocator_type::accessor           accessor;

#ifdef FEATURE_ESTD_ALLOCATOR_LOCKCOUNTER
    // non-standard, and phase this out in favor of 'helpers' to wrap up
    // empty counters
    typedef typename TAllocator::lock_counter           lock_counter;
#endif

    static CONSTEXPR handle_type invalid() { return allocator_type::invalid(); }

#ifndef FEATURE_ESTD_STRICT_DYNAMIC_ARRAY
    static CONSTEXPR bool is_locking() { return allocator_type::is_locking(); }

    // indicates whether the allocator_type is stateful (requiring an instance variable)
    // or purely static
    static CONSTEXPR bool is_stateful() { return TAllocator::is_stateful(); }

    // indicates whether the allocator_type is stateful (requiring an instance variable)
    // or purely static
    static CONSTEXPR bool is_singular() { return TAllocator::is_singular(); }

    // indicates whether handles innately can be queried for their size
    static CONSTEXPR bool has_size() { return TAllocator::has_size(); }
#endif

    static CONSTEXPR bool is_locking_exp = internal::has_locking_tag<allocator_type>::value;

    static CONSTEXPR bool is_stateful_exp = internal::has_stateful_tag<allocator_type>::value;

    static CONSTEXPR bool is_singular_exp = internal::has_singular_tag<allocator_type>::value;

    static CONSTEXPR bool has_size_exp = internal::has_size_tag<allocator_type>::value;

    static CONSTEXPR value_type invalid_handle() { return TAllocator::invalid_handle(); }

    static handle_type allocate(allocator_type& a, size_type n, const_void_pointer hint = NULLPTR)
    {
        return a.allocate(n);
    }

    static void deallocate(allocator_type& a, handle_type p, size_type n)
    {
        return a.deallocate(p, n);
    }


#ifndef FEATURE_ESTD_STRICT_DYNAMIC_ARRAY
    static reference lock(allocator_type& a, handle_type h, size_type pos = 0, size_type count = 0)
    {
        return a.lock(h, pos, count);
    }

    static reference lock(allocator_type &a, handle_with_offset h, size_type pos = 0, size_type count = 0)
    {
        return a.lock(h, pos, count);
    }

    static const value_type& clock(const allocator_type& a, handle_type h, size_type pos = 0, size_type count = 0)
    {
        return a.clock(h, pos, count);
    }

    static const value_type& clock(const allocator_type& a, const handle_with_offset& h, size_type pos = 0)
    {
        return a.clock(h, pos);
    }

    static void unlock(allocator_type& a, handle_type h)
    {
        a.unlock(h);
    }

    static void cunlock(const allocator_type& a, handle_type h)
    {
        a.cunlock(h);
    }
#endif

    static size_type max_size(const allocator_type& a)
    {
        // note that a.max_size is no longer required (though spec
        // strongly implies it's optionally permitted) in C++17, though
        // allocator_traits::max_size is not
        return a.max_size();
    }

#ifdef FEATURE_CPP_VARIADIC
private:
public:
    // TODO: Consider changing T* p to handle_type for locking variant
    //       note that we'd have to do a handle_with_offset version too since
    //       this call is often used in an allocated-array capacity
    template <class T, class... TArgs>
    static void construct(allocator_type& a, T* p, TArgs&&... args)
    {
        internal::construct_sfinae(a, p, std::forward<TArgs>(args)...);
        //new (static_cast<void*>(p)) T(std::forward<TArgs>(args)...);
    }
#endif

    // just for feature parity with construct
    // same concepts apply, namely:
    // a) SFINAE approach should be coded here
    // b) consider a handle_type/handle_with_offset variety
    template <class T>
    static void destroy(allocator_type& a, T* p)
    {
        p->~T();
    }
};


}
