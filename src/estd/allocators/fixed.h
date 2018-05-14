#pragma once

#include "generic.h"
#include "../traits/allocator_traits.h"
#include "../internal/handle_with_offset.h"
#include <string.h> // for strlen

namespace estd {

namespace internal {

// Can only have its allocate function called ONCE
// maps to one and only one regular non-locking buffer
template <class T, class TBuffer>
struct single_allocator_base
{
    typedef const void* const_void_pointer;
    typedef bool handle_type; // really I want it an empty struct
    typedef handle_type handle_with_size;
    //typedef T& handle_with_offset; // represents a pointer location past initial location of buffer
    typedef estd::internal::handle_with_offset<handle_type> handle_with_offset;
    typedef T value_type;
    typedef T* pointer;
    typedef std::size_t size_type;
    typedef estd::experimental::stateful_nonlocking_accessor<single_allocator_base> accessor;

protected:

    TBuffer buffer;

    single_allocator_base() {}

    single_allocator_base(const TBuffer& buffer) : buffer(buffer) {}

public:
    static CONSTEXPR handle_type invalid() { return false; }

    // technically we ARE locking since we have to convert the dummy 'bool' handle
    // to a pointer
    static CONSTEXPR bool is_locking() { return true; }


    value_type& lock(handle_type h, int pos = 0, int count = 0)
    {
        return buffer[pos];
    }

    value_type& lock(const handle_with_offset& h, int pos = 0, int count = 0)
    {
        return buffer[h.offset() + pos];
    }

    const value_type& clock_experimental(handle_type h, int pos = 0, int count = 0) const
    {
        return buffer[pos];
    }

    const value_type& clock_experimental(const handle_with_offset& h, int pos = 0, int count = 0) const
    {
        return buffer[h.offset() + pos];
    }

    void unlock(handle_type h) {}

    handle_with_offset offset(handle_type h, size_t pos)
    {
        return handle_with_offset(h, pos);
    }

    void deallocate(handle_with_size h, size_type count)
    {
    }


    typedef typename nothing_allocator<T>::lock_counter lock_counter;
};

// Can only have its allocate function called ONCE
// tracks how much of the allocator has been allocated
// null_terminated flag mainly serves as a trait/clue to specializations
// len can == 0 in which case we're in unbounded mode
template <class T, size_t len, bool null_terminated = false, class TBuffer = T[len]>
struct single_fixedbuf_allocator : public single_allocator_base<T, TBuffer>
{
    typedef single_allocator_base<T, TBuffer> base_t;

    typedef T value_type;
    typedef bool handle_type; // really I want it an empty struct
    typedef handle_type handle_with_size;

public:
    single_fixedbuf_allocator() {}

    // FIX: something bizzare is happening here and base_t is ending
    // up as map_base during debug session
    single_fixedbuf_allocator(const TBuffer& buffer) : base_t(buffer) {}


    handle_with_size allocate_ext(size_t size)
    {
        // we can't be sure if alloc fails or succeeds with unspecified length (when TBuffer = *)
        // but since we are embedded-oriented, permit it to succeed and trust the programmer
        // took precautions
        if(len == 0)   return true;

        // remember our handle is a true/false
        return size <= len;
    }

    handle_type allocate(size_t size)
    {
        return allocate_ext(size);
    }


    handle_with_size reallocate_ext(handle_type, size_t size)
    {
        // NOTE: assuming incoming handle_type is valid

        return allocate_ext(size);
    }

    // FIX: returns actual maximum size of unsigned,not ideal
    size_t max_size() const { return len == 0 ? -1 : len; }

    size_t size(handle_with_size h) const { return max_size(); }
};


// mainly for layer3:
// runtime (but otherwise constant) size()
// runtime (but otherwise constant) buffer*
// as before, null_terminated is merely a clue/trait for consumer class
template <class T, bool null_terminated = false>
class single_fixedbuf_runtimesize_allocator : public single_allocator_base<T, T*>
{
public:
    typedef single_allocator_base<T, T*> base_t;
    typedef typename base_t::size_type size_type;
    typedef typename base_t::handle_type handle_type;
    typedef handle_type handle_with_size;

private:
    size_type m_buffer_size;

public:
    struct InitParam
    {
        T* buffer;
        size_type size;

        InitParam(T* buffer, size_type size) : buffer(buffer), size(size) {}
    };

    single_fixedbuf_runtimesize_allocator(const InitParam& p)
        : base_t(p.buffer), m_buffer_size(p.size)
    {}

    size_type size(handle_with_size h) const { return m_buffer_size; }

    size_type max_size() const { return m_buffer_size; }

    handle_with_size allocate_ext(size_t size)
    {
        // TODO: put in a flag only in debug mode to detect multiple alloc
        // (should only have one for single+fixed allocator)

        // remember our handle is a true/false
        return size <= max_size();
    }

    handle_type allocate(size_t size)
    {
        return allocate_ext(size);
    }


    handle_with_size reallocate_ext(handle_type, size_t size)
    {
        // NOTE: assuming incoming handle_type is valid

        return allocate_ext(size);
    }
};

// See reference implementation in dynamic_array.h
template <class TAllocator>
struct dynamic_array_helper;

template <class T, size_t len, bool null_terminated, class TBuffer>
class dynamic_array_fixedbuf_helper_base
{
    typedef single_fixedbuf_allocator<T, len, null_terminated, TBuffer> allocator_type;

    allocator_type allocator;

protected:
    typedef estd::allocator_traits<allocator_type> allocator_traits;
    typedef typename allocator_traits::size_type size_type;
    typedef typename allocator_traits::value_type value_type;
    typedef typename allocator_type::handle_with_size handle_with_size;
    typedef typename allocator_type::handle_with_offset handle_with_offset;

    dynamic_array_fixedbuf_helper_base() {}

    template <class TParam>
    dynamic_array_fixedbuf_helper_base(TParam p) : allocator(p) {}

public:
    static CONSTEXPR bool uses_termination() { return null_terminated; }

    value_type& lock(size_type pos = 0, size_type count = 0)
    {
        return allocator.lock(true, pos, count);
    }

    const value_type& clock_experimental(size_type pos = 0, size_type count = 0) const
    {
        return allocator.clock_experimental(true, pos, count);
    }


    void unlock() {}

    void cunlock_experimental() const {}

    size_type capacity() const { return allocator.max_size(); }

    allocator_type& get_allocator() { return allocator; }

    handle_with_offset offset(size_type pos)
    {
        return allocator.offset(true, pos);
    }

    bool allocate(size_type sz) { return sz <= capacity(); }
    bool reallocate(size_type sz) { return sz <= capacity(); }

    bool is_allocated() const { return true; }

    size_type size() const
    {
#ifdef FEATURE_CPP_STATIC_ASSERT
        // specialization required if we aren't null terminated (to track size variable)
        static_assert(null_terminated, "Utilizing this size method requires null termination = true");
#endif

        const T* s = &clock_experimental();

        // FIX: use char_traits string length instead
        size_type sz = strlen(s);

        cunlock_experimental();

        return sz;
    }
};

// as per https://stackoverflow.com/questions/4189945/templated-class-specialization-where-template-argument-is-a-template
// and https://stackoverflow.com/questions/49283587/templated-class-specialization-where-template-argument-is-templated-difference
// template <>
// TODO: Reconcile with single_fixedbuf_runtimesize_allocator.  Looks like it should be, but can't focus in this
// environment enough to be sure
template <class T, size_t len, class TBuffer>
class dynamic_array_helper<single_fixedbuf_allocator<T, len, false, TBuffer> >
        : public dynamic_array_fixedbuf_helper_base<T, len, false, TBuffer>
{
protected:
    typedef single_fixedbuf_allocator<T, len, false, TBuffer> allocator_type;
    typedef estd::allocator_traits<allocator_type> allocator_traits;
    typedef typename allocator_traits::size_type size_type;
    typedef typename allocator_traits::value_type value_type;
    typedef typename allocator_type::handle_with_size handle_with_size;
    typedef typename allocator_type::handle_with_offset handle_with_offset;

    size_type m_size;

public:
    dynamic_array_helper(allocator_type*) : m_size(0) {}

    dynamic_array_helper() : m_size(0) {}

    size_type size() const { return m_size; }

    // +++ intermediate
    void size(size_type s) { m_size = s; }
    // ---
};





// applies generally to T[N], RW buffer but also to non-const T*
// applies specifically to null-terminated
template <class T, size_t len, class TBuffer>
class dynamic_array_helper<single_fixedbuf_allocator<T, len, true, TBuffer> >
        : public dynamic_array_fixedbuf_helper_base<T, len, true, TBuffer>
{
protected:
    typedef dynamic_array_fixedbuf_helper_base<T, len, true, TBuffer> base_t;
    typedef single_fixedbuf_allocator<T, len, true, TBuffer> allocator_type;
    typedef estd::allocator_traits<allocator_type> allocator_traits;
    typedef typename allocator_traits::size_type size_type;
    typedef typename allocator_type::handle_with_size handle_with_size;
    typedef typename allocator_type::handle_with_offset handle_with_offset;
    typedef T value_type;

public:
    // +++ intermediate
    void size(size_type s)
    {
        if(s > len)
        {
            // FIX: issue some kind of warning
        }

        base_t::lock(s) = 0;
        base_t::unlock();
    }

    // needed because above overload hides underlying size()
    size_type size() const { return base_t::size(); }
    // ---


    struct InitParam
    {
        const TBuffer& b;
        bool is_initialized;

        InitParam(const TBuffer& b, bool is_initialized = false) :
                b(b),
                is_initialized(is_initialized) {}
    };


    dynamic_array_helper(const InitParam& p) : base_t(p.b)
    {
        if(!p.is_initialized) size(0);
    }

    dynamic_array_helper(const TBuffer& b) : base_t(b)
    {
        // NOTE: Only should arrive here with non const* since specializations should be
        // taking over now in those cases.  However, we'd still like the feature of not
        // brute forcing initialization (there are cases in which a passed in RW buffer
        // is already initialized)
        size(0);
    }

    dynamic_array_helper()
    {
        size(0);
    }
};

// attempt to specialize for const T* scenarios
// for now, seems to be necessary in parallel with the following more-specialized version
// debugger doesn't pick up construction, but size() is invoked
template <class T, size_t len>
class dynamic_array_helper<single_fixedbuf_allocator<T, len, true, const T*> >
        : public dynamic_array_fixedbuf_helper_base<T, len, true, const T*>
{
    typedef dynamic_array_fixedbuf_helper_base<T, len, true, const T*> base_t;

public:
    typedef typename base_t::size_type size_type;
    typedef typename base_t::allocator_traits allocator_traits;

    dynamic_array_helper(const T* buf) : base_t(buf) {}
};


// specialize for const T* scenarios
// would like to merge this with above one if possible
template <class T, size_t len>
class dynamic_array_helper<single_fixedbuf_allocator<const T, len, true, const T*> >
        : public dynamic_array_fixedbuf_helper_base<const T, len, true, const T*>
{
    typedef dynamic_array_fixedbuf_helper_base<const T, len, true, const T*> base_t;

public:
    dynamic_array_helper(const T* buf) : base_t(buf) {}
};

}

}
