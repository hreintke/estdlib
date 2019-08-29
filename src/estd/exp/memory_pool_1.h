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
template <class T, std::size_t N>
struct memory_pool
{
    // instance provider being first base class means:
    // 1 - tracked data precedes control data
    // 2 - it's presumed 'next' is memory aligned, but that might not always be true
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

    estd::intrusive_forward_list<item> free_nodes;
    estd::array<item, N> pool;

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
        return free_nodes.pop_front();
    }

    // internal deallocate call
    // NOTE: behavior undefined if 'i' wasn't reserved from this memory pool in the first place
    void release(item& i)
    {
        free_nodes.push_front(i);
    }
};

}}