#pragma once

#include "../../traits/allocator_traits.h"

namespace estd { namespace internal { namespace impl {

template <class TValue, bool is_present, TValue default_value>
class value_evaporator;

template <class TValue, TValue default_value>
class value_evaporator<TValue, true, default_value>
{

};

template <class TValue, TValue default_value>
class value_evaporator<TValue, false, default_value>
{
protected:
};


// contains no members, just specialized copy helpers
template <class TAllocator, bool is_contiguous>
class contiguous_descriptor;


// experimental, unused, untested, but simple and useful enough I think it will graduate quickly
// no bounds checks are performed, as this is expected to be done by querying largest contiguous chunk size
template <class TAllocator>
struct contiguous_descriptor<TAllocator, true>
{
    typedef typename remove_reference<TAllocator>::type allocator_type;
    typedef typename estd::allocator_traits<allocator_type> allocator_traits;
    typedef typename allocator_type::handle_with_offset handle_with_offset;
    typedef typename allocator_type::handle_type handle_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    // copy outside buffer into this handle-based memory
    static void copy_into(allocator_type& a, handle_type h, const value_type* source, size_type pos, size_type len)
    {
        value_type* dest = &allocator_traits::lock(a, h, pos, len);

        while(len--) *dest++ = *source++;

        allocator_traits::unlock(a, h);
    }


    // copy this handle-based memory to outside buffer
    static void copy_from(const allocator_type& a, handle_type h, value_type* dest, size_type pos, size_type len)
    {
        const value_type* source = &allocator_traits::clock(a, h, pos, len);

        while(len--) *dest++ = *source++;

        allocator_traits::cunlock(a, h);
    }
};

// https://en.cppreference.com/w/cpp/language/ebo we can have specialized base classes which are empty
// and don't hurt our sizing
template <class TAllocator, bool is_stateful>
class allocator_descriptor;


// TAllocator could be a ref here
template <class TAllocator>
class allocator_descriptor<TAllocator, true>
{
    TAllocator allocator;

protected:
    // NOTE: variadic would be nice, but obviously not always available
    template <class TAllocatorParameter>
    allocator_descriptor(TAllocatorParameter& p) :
        allocator(p) {}

    // Not unusual for a stateful allocator to default construct itself just
    // how we want it
    allocator_descriptor() {}

public:
    typedef typename remove_reference<TAllocator>::type allocator_type;
    typedef typename allocator_type::handle_with_offset handle_with_offset;
    typedef typename allocator_type::handle_type handle_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    // Would be nice if we could const this, but for stateful allocators that's not reasonable
    allocator_type& get_allocator() { return allocator; }

    const allocator_type& get_allocator() const { return allocator; }

protected:
    typedef allocator_type& allocator_ref;
};


template <class TAllocator>
struct allocator_descriptor<TAllocator, false>
{
    typedef typename remove_reference<TAllocator>::type allocator_type;
    typedef allocator_type allocator_ref;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    // NOTE: odd, but OK.  Since we're stateless, we can return what otherwise
    // would be an invalid reference
    allocator_type get_allocator() const { return allocator_type(); }
};

// singular technically doesn't track a handle
// TODO: refactor to utilize value_evaporator (get_allocator, too)
template <class TAllocator, bool is_singular>
class handle_descriptor;

template <class TAllocator>
class handle_descriptor<TAllocator, true>
{
    typedef typename remove_reference<TAllocator>::type allocator_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::size_type size_type;

protected:
    handle_descriptor(bool) {}

    typedef typename allocator_type::handle_type handle_type;

    void handle(bool) {}

    value_type& lock(allocator_type& a, size_type pos = 0, size_type len = 0)
    {
        return a.lock(true, pos, len);
    }

    void unlock(allocator_type& a) { a.unlock(true); }

public:
    bool handle() const { return true; }
};


template <class TAllocator>
struct handle_descriptor<TAllocator, false>
{
    typedef typename remove_reference<TAllocator>::type allocator_type;
    typedef typename allocator_type::handle_type handle_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::size_type size_type;

private:
    handle_type m_handle;

protected:
    void handle(const handle_type& h) { m_handle = h; }

    handle_descriptor(const handle_type& h) : m_handle(h) {}

    value_type& lock(allocator_type& a, size_type pos = 0, size_type len = 0)
    {
        return a.lock(m_handle, pos, len);
    }

    void unlock(allocator_type& a) { a.unlock(m_handle); }

public:
    bool handle() const { return m_handle; }
};


}}}
