/**
 * @file
 */
#pragma once

#include "../array.h"
#include "../memory.h"
#include "../type_traits.h"
#include "../cstdint.h"
#include "../forward_list.h"
#include "../algorithm.h"
#include "../limits.h"

// memory pools continually turn out to be kinda tricky, so numbering my attempts
// This attempt was initiated 8/29/2019 after existing memory_pool.h turned out to be too unweildy,
// even if it was impressively powerful
namespace estd { namespace experimental1 {

// FIX: Need explicit control over memory alignment *OR* comment as to why we don't
template<class T>
struct memory_pool_item
{
    // Type 1 item node* comes first in memory
    struct type_1 :
            estd::experimental::forward_node_base_base<type_1*>,
            estd::experimental::raw_instance_provider<T>
    {
        typedef estd::experimental::forward_node_base_base<type_1*> node_base_type;

        // DEBT: forward_node_base_base demands some kind of initializing constructor, so indulge it
        // with a NULLPTR.  Optimize this out, and don't leverage its "pre initialization" side effect
        type_1() : node_base_type(NULLPTR) {}

        // DEBT: Traits is the paradigm for doing this, so put it in a traits mechanism
        static type_1* get_item_from_tracked(T* tracked)
        {
            // NOTE: Due to https://software.intel.com/en-us/blogs/2015/04/20/null-pointer-dereferencing-causes-undefined-behavior
            // we don't dereference a null to find offsets.  This means to be safe we'll need a runtime evaluation
            // on a dummy stack variable
            // Future optimization would be to invoke 'offsetof' when available, including making our own 'offsetof' in
            // scenarios where we could be sure it would not be undefined
            type_1 for_offset_only;
            std::ptrdiff_t diff = for_offset_only.buf - (byte*)&for_offset_only;
            byte* adjuster = (byte*)tracked;
            adjuster -= diff;
            return reinterpret_cast<type_1*>(adjuster);
        }
    };

    // Type 2 item inline data comes first in memory
    // instance provider being first base class means:
    // 1 - tracked data precedes control data
    // 2 - it's presumed 'next' is memory aligned, but that might not always be true
    struct type_2 :
            estd::experimental::raw_instance_provider<T>,
            estd::experimental::forward_node_base_base<type_2*>
    {
        typedef estd::experimental::forward_node_base_base<type_2*> node_base_type;

        // DEBT: forward_node_base_base demands some kind of initializing constructor, so indulge it
        // with a NULLPTR.  Optimize this out, and don't leverage its "pre initialization" side effect
        type_2() : node_base_type(NULLPTR) {}

        // DEBT: Traits is the paradigm for doing this, so put it in a traits mechanism
        static type_2* get_item_from_tracked(T* tracked)
        {
            return reinterpret_cast<type_2*>(tracked);
        }
    };
};

// No attempt to conform to 'allocator' pattern, do that with an owning class with a 'has a'
// relationship
template <class T, std::size_t N, class TItem = typename memory_pool_item<T>::type_2>
struct memory_pool
{
    typedef TItem item_type;

private:
    // DEBT: forward_list has a traits/allocator empty struct gobbling up space
    estd::intrusive_forward_list<item_type> free_nodes;
    estd::array<item_type, N> pool;

public:
    memory_pool()
    {
        free_nodes.push_front(pool[0]);

        // DEBT: Use last/insert_after instead of this
        for(std::size_t j = 0; j < N - 1; j++)
        {
            item_type& i = pool[j];
            i.next(&pool[j + 1]);
        }

        pool[N - 1].next(NULLPTR);
    }

    // internal allocate call
    item_type& reserve()
    {
        item_type& front = free_nodes.front();
        free_nodes.pop_front();
        return front;
    }

    // internal deallocate call
    // NOTE: behavior undefined if 'i' wasn't reserved from this memory pool in the first place
    void release(item_type& i)
    {
        free_nodes.push_front(i);
    }

    // given a pointer to the 'tracked data', acquire the related 'item'
    // DEBT: Put this into traits
    static item_type* get_item_from_tracked(T* tracked)
    {
        return item_type::get_item_from_tracked(tracked);
    }

    // internal call counting how many 'free_nodes' are left
    std::size_t free_slots() const
    {
        // DEBT: Create and utilize 'estd::count' in algorithm.h.  Not doing so now to keep
        // experimental changes isolated to just this area
        return estd::distance(free_nodes.begin(), free_nodes.end());
    }
};

}}