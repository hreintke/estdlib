#pragma once

#include "iterators/list.h"
#include "traits/list_node.h"
#include "forward_list.h"

namespace estd {

template<class T, class TNode = T,
         class TAllocator = experimental_std_allocator<TNode>,
         class TNodeTraits = node_traits<TNode, TAllocator, nothing_allocator<T> > >
class list : public internal::linkedlist_base<T, TNodeTraits, internal::list::BidirectionalIterator<T, TNodeTraits> >
{
    typedef internal::linkedlist_base<T, TNodeTraits, internal::list::BidirectionalIterator<T, TNodeTraits> > base_t;
    typedef typename base_t::allocator_t allocator_t;
    typedef typename base_t::value_type value_type;
    typedef value_type& reference;
    typedef TNodeTraits node_traits_t;
    typedef typename base_t::node_handle node_handle;

    node_handle m_back;

public:
    list(allocator_t* allocator = NULLPTR) : base_t(allocator) {}

    reference back()
    {
        reference back_value = base_t::alloc_lock(m_back);

        base_t::alloc_unlock(m_back);

        return back_value;
    }
};

}
