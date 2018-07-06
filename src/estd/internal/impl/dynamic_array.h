#pragma once

#include "allocated_array.h"
#include "../../allocators/fixed.h"
#include "../../allocators/handle_desc.h"

namespace estd { namespace internal { namespace impl {


// See reference implementation near the bottom
template <class TAllocator, class TPolicy>
struct dynamic_array;

// TODO: Fixup name.  Specializer to reveal size of either
// an explicitly-sized or null-terminated entity
template <class TAllocator, bool null_terminated>
struct dynamic_array_length;

// null terminated
template <class TAllocator>
struct dynamic_array_length<TAllocator, true>
{
    typedef typename estd::remove_reference<TAllocator>::type allocator_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::handle_type handle_type;

    bool empty(const allocator_type& a, const handle_type& h) const
    {
        const value_type* v = &a.clock(h, 0, 1);

        bool is_terminator = *v == 0;

        a.cunlock(h);

        return is_terminator;
    }

    size_type size(const TAllocator& a, const handle_type& h) const
    {
#ifdef FEATURE_CPP_STATIC_ASSERT
        // specialization required if we aren't null terminated (to track size variable)
        //static_assert(null_terminated, "Utilizing this size method requires null termination = true");
#endif

        const value_type* s = &a.clock(h);

        // FIX: use char_traits string length instead
        size_type sz = strlen(s);

        a.cunlock(h);

        return sz;
    }

    // +++ temporary
    // semi-brute forces size by stuffing a null terminator at the specified spot
    void size(TAllocator& a, const handle_type& h, size_type len)
    {
        /*
        if(len > base_t::capacity())
        {
            // FIX: issue some kind of warning
        } */

        a.lock(h, len, 1) = 0;
        a.unlock(h);
    }
    // ---
};


// explicitly sized
template <class TAllocator>
struct dynamic_array_length<TAllocator, false>
{
private:
    typedef typename std::remove_reference<TAllocator>::type allocator_type;
    typedef typename estd::allocator_traits<allocator_type> allocator_traits;

public:
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_traits::handle_type handle_type;

protected:
    size_type m_size;

    dynamic_array_length() :
            m_size(0)
    {}


public:

    bool empty(const allocator_type& a, const handle_type& h) const
    {
        return m_size == 0;
    }

    size_type size(const TAllocator& a, const handle_type& h) const
    {
        return m_size;
    }

    // +++ temporary
    void size(TAllocator& a, const handle_type& h, size_type len)
    {
        m_size = len;
    }
    // ---
};

// ---


// intermediate class as we transition to handle_descriptor.  Eventually phase this out
template <class TAllocator, bool null_terminated>
class dynamic_array_base :
        public estd::handle_descriptor<TAllocator>,
        dynamic_array_length<TAllocator, null_terminated>
{
    typedef estd::handle_descriptor<TAllocator> base_t;
    typedef dynamic_array_length<TAllocator, null_terminated> length_helper_t;

public:
    dynamic_array_base() {}

    template <class TAllocatorParameter>
    dynamic_array_base(TAllocatorParameter& p) : base_t(p) {}


    static CONSTEXPR bool uses_termination() { return null_terminated; }

    typedef typename base_t::allocator_type allocator_type;
    // NOTE: Necessary to do allocator_traits here to disambiguate all the ones from
    // multiple inheritance above (though I think that should have worked, unless
    // they are resolving to different types... i.e. one is a TAllocator&)
    typedef typename base_t::allocator_traits allocator_traits;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_traits::handle_with_offset handle_with_offset;

    // account for null-termination during a max_size request
    size_type max_size() const
    {
        return base_t::get_allocator().max_size() - (null_terminated ? 1 : 0);
    }


    // repurposing/renaming of what size meant before (ALLOCATED) vs now
    // (USED within ALLOCATED)
    size_type capacity() const { return base_t::size(); }


    // Helper for old dynamic_array code.  New one I'm thinking caller
    // can shoulder this burden
    handle_with_offset offset(size_type pos) const
    {
        return base_t::get_allocator().offset(base_t::handle(), pos);
    }

    // remember, dynamic_array_helper size() refers not to ALLOCATED size, but rather
    // 'used' size within that allocation.  For this variety, we are null terminated
    size_type size() const
    {
        if(!base_t::is_allocated()) return 0;

        return length_helper_t::size(base_t::get_allocator(), base_t::handle());
    }

    void size(size_type n)
    {
        length_helper_t::size(base_t::get_allocator(), base_t::handle(), n);
    }

    bool empty() const
    {
        return length_helper_t::empty(base_t::get_allocator(), base_t::handle());
    }
};

/*
// applies generally to T[N], RW buffer but also to non-const T*
// applies specifically to null-terminated
template <class T, size_t len, class TBuffer, class TPolicy>
class dynamic_array<single_fixedbuf_allocator<T, len, true, TBuffer>, TPolicy>
        : public dynamic_array_base<single_fixedbuf_allocator<T, len, true, TBuffer>, true >
{
    typedef dynamic_array_base<single_fixedbuf_allocator<T, len, true, TBuffer>, true > base_t;

public:
    typedef typename base_t::allocator_type allocator_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::handle_type handle_type;

    // FIX: Iron out exactly where we really assign size(0) - sometimes we
    // want to pre-initialize our buffer so size(0) is not an always thing
    dynamic_array(TBuffer& b) : base_t(b)
    {
        base_t::size(0);
    }

    dynamic_array(const TBuffer& b) : base_t(b)
    {
        // FIX: This only works for NULL-terminated scenarios
        // we still need to assign a length of 0 for explicit lenght scenarios
    }

    dynamic_array()
    {
        base_t::size(0);
    }
}; */






// runtime (layer3-ish) version, no policy (void) -
// hard wired to no null termination
template <class T>
class dynamic_array<single_fixedbuf_runtimesize_allocator<T>, void> :
        public dynamic_array_base<single_fixedbuf_runtimesize_allocator<T>, false >
{
    typedef dynamic_array_base<single_fixedbuf_runtimesize_allocator<T>, false > base_t;
    typedef typename base_t::size_type size_type;

public:
    template <class TInitParam>
    dynamic_array(const TInitParam& p) : base_t(p) {}
};


// Passes unit tests, needs just a bit more inspection and
// should be good to go
//#define FEATURE_ESTD_STRICT_DYNAMIC_ARRAY

#ifdef FEATURE_ESTD_STRICT_DYNAMIC_ARRAY
template <class TAllocator, class TPolicy>
struct dynamic_array : public
        dynamic_array_base<typename std::remove_reference<TAllocator>::type, false>
{
    typedef dynamic_array_base<typename std::remove_reference<TAllocator>::type, false> base_t;
    typedef typename base_t::allocator_type allocator_type;

    dynamic_array(allocator_type& alloc) : base_t(alloc) {}

    template <class TAllocatorParam>
    dynamic_array(const TAllocatorParam& p) : base_t(p) {}

    dynamic_array() {}
};
#else
// General-case dynamic_array where we don't attempt to optimize anything.  This is a fullback
// TODO: #ifdef this out in some kind of strict mode
template <class TAllocator, class TPolicy>
class dynamic_array
{
public:
    // default implementation is 'full fat' to handle all scenarios
    typedef typename std::remove_reference<TAllocator>::type allocator_type;
    typedef estd::allocator_traits<allocator_type> allocator_traits;
    typedef typename allocator_type::value_type value_type;

    typedef typename allocator_traits::handle_type handle_type;
    typedef typename allocator_traits::handle_with_size handle_with_size;
    typedef typename allocator_traits::size_type size_type;
    typedef typename allocator_traits::handle_with_offset handle_with_offset;

private:
    // don't fiddle with ref juggling here - if that's absolutely necessary use
    // the RefAllocator helper
    TAllocator allocator;
    // handle.size represents currently allocation portion
    handle_with_size handle;
    // remember, size represents 'user/app' portion.
    size_type m_size;

public:
    static CONSTEXPR bool uses_termination() { return false; }

    size_type capacity() const { return allocator.size(handle); }
    size_type size() const { return m_size; }

    allocator_type& get_allocator() { return allocator; }

    // +++ intermediate calls, phase these out eventually
    handle_with_size get_handle() { return handle; }
    void size(size_type s) { m_size = s; }
    // ---

    handle_with_offset offset(size_type pos) const
    {
        return allocator.offset(handle, pos);
    }

    value_type& lock(size_type pos = 0, size_type count = 0)
    {
        return allocator_traits::lock(allocator, handle, pos, count);
    }

    // constant-return-lock
    const value_type& clock(size_type pos = 0, size_type count = 0) const
    {
        return allocator.clock(handle, pos, count);
    }

    void unlock()
    {
        allocator_traits::unlock(allocator, handle);
    }

    void cunlock() const
    {
        allocator_traits::cunlock(allocator, handle);
    }

    bool is_allocated() const
    {
        handle_type h = handle;
        return h != allocator_traits::invalid();
    }

    bool allocate(size_type capacity)
    {
        handle = allocator.allocate_ext(capacity);
        return is_allocated();
    }


    bool reallocate(size_type capacity)
    {
        handle = allocator.reallocate_ext(handle, capacity);
        return is_allocated();
    }

    explicit dynamic_array(allocator_type& alloc) :
        allocator(alloc),
        handle(allocator_traits::invalid()),
        m_size(0)
    {

    }

    template <class T>
    dynamic_array(T init) :
            allocator(init),
            handle(allocator_traits::invalid()),
            m_size(0)
    {

    }

    dynamic_array() :
            handle(allocator_traits::invalid()),
            m_size(0)
    {

    }

    ~dynamic_array()
    {
        if(handle != allocator_traits::invalid())
            allocator.deallocate(handle, 1);
    }

    bool empty() const { return m_size == 0; }
};
#endif

}}}
