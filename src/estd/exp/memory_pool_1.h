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

// No attempt to conform to 'allocator' pattern, do that with an owning class with a 'has a'
// relationship
// TODO: Consider passing in TItem so that we can do 'type 1' item
template <class T, std::size_t N>
struct memory_pool
{
    // instance provider being first base class means:
    // 1 - tracked data precedes control data
    // 2 - it's presumed 'next' is memory aligned, but that might not always be true
    // this is a 'type 2' item
    struct item :
            estd::experimental::raw_instance_provider<T>,
            // FIX: Need explicit control over memory alignment *OR* comment as to why we don't
            estd::experimental::forward_node_base_base<item*>
    {
        typedef estd::experimental::forward_node_base_base<item*> node_base_type;

        // DEBT: forward_node_base_base demands some kind of initializing constructor, so indulge it
        // with a NULLPTR.  Optimize this out, and don't leverage its "pre initialization" side effect
        item() : node_base_type(NULLPTR) {}
    };

private:
    // DEBT: forward_list has a traits/allocator empty struct gobbling up space
    estd::intrusive_forward_list<item> free_nodes;
    estd::array<item, N> pool;

public:
    memory_pool()
    {
        free_nodes.push_front(pool[0]);

        // DEBT: Use last/insert_after instead of this
        for(std::size_t j = 0; j < N - 1; j++)
        {
            item& i = pool[j];
            i.next(&pool[j + 1]);
        }

        pool[N - 1].next(NULLPTR);
    }

    // internal allocate call
    item& reserve()
    {
        item& front = free_nodes.front();
        free_nodes.pop_front();
        return front;
    }

    // internal deallocate call
    // NOTE: behavior undefined if 'i' wasn't reserved from this memory pool in the first place
    void release(item& i)
    {
        free_nodes.push_front(i);
    }

    // given a pointer to the 'tracked data', acquire the related 'item'
    // DEBT: Put this into traits
    item* get_item_from_tracked(T* tracked) const
    {
        return reinterpret_cast<item*>(tracked);
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