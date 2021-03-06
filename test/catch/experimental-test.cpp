#include <catch.hpp>

#include "estd/array.h"
//#include "estd/exp/buffer.h"
#include "mem.h"
#include "test-data.h"
#include <estd/string.h>
#include <estd/exp/memory_pool.h>
#include <estd/memory.h>
#include <estd/functional.h>
#include "estd/streambuf.h"
#include <estd/charconv.h>
#include <estd/cctype.h>
//#include <estd/locale.h>


struct TestA {};

struct Test
{
    int a;
    float b;

    //std::string s;
    TestA& t;

    constexpr Test(int a, float b, TestA& t) :
        a(a), b(b), t(t) {}
};

TestA t;

template <class TBase>
struct provider_test : TBase
{
    typedef TBase value_provider;
    typedef typename value_provider::value_type value_type;

    template <class T>
    void do_require(const T& value)
    {
        const value_type& v = value_provider::value();

        REQUIRE(v == value);
    }

    provider_test() {}

    provider_test(int v) : value_provider (v) {}
};

int global_provider_test_value = 6;

estd::layer1::string<128> provider_string;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

// trying this
// https://stackoverflow.com/questions/2831934/how-to-use-if-inside-define-in-the-c-preprocessor
#define _STATIC_ASSERT(expr) STATIC_ASSERT_##expr
#define STATIC_ASSERT(expr) _STATIC_ASSERT(expr)
#define STATIC_ASSERT_true
#define STATIC_ASSERT_false #error Assert Failed

// UNTESTED, UNFINISHED
#if !defined(__cpp_static_assert)
#define static_assert(expr, message) STATIC_ASSERT(expr)
#endif



TEST_CASE("experimental tests")
{
    SECTION("A")
    {
        //estd::experimental::layer0
        constexpr Test test[] =
        {
            { 1,2, t },
            { 2, 3, t}
        };
        //constexpr Test test1(1, 2, t);
    }
    SECTION("accessor")
    {
        _allocator<int> a;
        int* val = a.allocate(1);

        *val = 5;

        estd::experimental::stateful_locking_accessor<_allocator<int>> acc(a, val);

        int& val2 = acc;

        REQUIRE(val2 == 5);

        a.deallocate(val, 1);
    }
    SECTION("bitness size_t deducer")
    {
        SECTION("8-bit")
        {
            {
                typedef estd::internal::deduce_fixed_size_t<4> deducer_t;
                REQUIRE(sizeof(deducer_t::size_type) == 1);
            }
            {
                typedef estd::internal::deduce_fixed_size_t<10> deducer1_t;
                REQUIRE(sizeof(deducer1_t::size_type) == 1);
            }
            {
                typedef estd::internal::deduce_fixed_size_t<100> deducer_t;
                REQUIRE(sizeof(deducer_t::size_type) == 1);
            }
        }
        SECTION("16-bit")
        {
            typedef estd::internal::deduce_fixed_size_t<1000> deducer_t;
            REQUIRE(sizeof(deducer_t::size_type) == 2);
        }
    }
    SECTION("providers")
    {
        using namespace estd::experimental;

        SECTION("temporary")
        {
            provider_test<temporary_provider<int> > pt;

            REQUIRE(pt.value() == 0);

            pt.do_require(0);
        }
        SECTION("instanced")
        {
            provider_test<instance_provider<int> > pt(5);

            REQUIRE(pt.value() == 5);

            pt.do_require(5);
        }
        SECTION("global")
        {
            provider_test<global_provider<int&, global_provider_test_value> > pt;

            REQUIRE(pt.value() == 6);

            pt.do_require(6);

            provider_test<global_provider<
                    estd::layer1::string<128>&,
                            provider_string> > pt2;

            estd::layer1::string<64> f = "hi2u";

            provider_string = "hi2u";

            REQUIRE(pt2.value() == f);
            REQUIRE(pt2.value() == "hi2u");

            pt2.do_require(f);
        }
        SECTION("global")
        {
            provider_test<literal_provider<int, 7> > pt;

            REQUIRE(pt.value() == 7);

            pt.do_require(7);
        }
        SECTION("pointer from value")
        {
            provider_test<pointer_from_instance_provider<int> > pt(5);

            REQUIRE(*pt.value() == 5);
        }
    }
    SECTION("memory pool")
    {
        using namespace estd;
        using namespace estd::experimental;

        SECTION("simple integer pool")
        {
            typedef estd::experimental::memory_pool_1<int, 10> memory_pool_type;
            memory_pool_type pool;

            int* i = pool.allocate();
            REQUIRE(pool.count_free() == 9);
            int& i2 = pool.construct(3);
            REQUIRE(pool.count_free() == 8);

            pool.deallocate(i);
            REQUIRE(pool.count_free() == 9);

            REQUIRE(i2 == 3);

            pool.destroy(i2);
            REQUIRE(pool.count_free() == 10);

            constexpr int sz = sizeof(pool);
            constexpr int sz_item = sizeof(memory_pool_type::item);
            constexpr int sz_size_type = sizeof(memory_pool_type::size_type);

            // TODO: We actually want to autodeduce this size to uint8_t, etc. eventually
            REQUIRE(sz_size_type == sizeof(uint8_t));
            REQUIRE(sz_item == sizeof(int) + sz_size_type);
            REQUIRE(sz == sz_item * 10 + sz_size_type);
        }
        SECTION("low level access")
        {
            typedef memory_pool_1<int, 10> pool_type;
            pool_type pool;

            auto& item = pool.allocate_item();
            auto& item2 = pool.allocate_item();

            auto& _item2 = pool.lock(item._next);

            REQUIRE(&item2 == &_item2);

            SECTION("peering in with linked list")
            {
                // this linked list relies completely on the storage from 'pool',
                // including the intrusive portion (the 'next' handle).  The only
                // memory allocated for ext_list_type therefore should be a pointer
                // to the original storage (for lock resolution) and the current head
                pool_type::item_ext_node_traits traits(pool.node_traits().storage);
                pool_type::ext_list_type list(traits);

                list.push_front(item);
                list.push_front(item2);

                // odd, looks like my iterators are cross wired
                int sz = estd::distance(list.begin(), list.end());

                REQUIRE(sz == 2);

                pool_type::item_ext_node_traits::node_type& f = list.front();

                REQUIRE(&f == &item2);
                list.pop_front();
                REQUIRE(&list.front() == &item);
            }
        }
        SECTION("alignment testing")
        {
            struct
                    //alignas(8)
                    test1 { int val; };

            typedef estd::experimental::memory_pool_1<test1, 10> memory_pool_type;
            memory_pool_type pool;

            constexpr int sz = sizeof(pool);
            constexpr int sz_item = sizeof(memory_pool_type::item);
            constexpr int sz_size_type = sizeof(memory_pool_type::size_type);

            // TODO: We actually want to autodeduce this size to uint8_t, etc. eventually
            REQUIRE(sz_size_type == sizeof(uint8_t));

            // Not ready yet, still experimenting
            //REQUIRE(sz_item == sizeof(int*));

            // was expecting '81' but got '84'.  didn't expect alignof/alignas to affect this,
            // but that's why we experiment
            //REQUIRE(sz == sz_item * 10 + sz_size_type);
        }
        SECTION("advanced shared_ptr pool")
        {
            typedef void (*deleter_fn)(test::Dummy* to_delete, void* context);
            deleter_fn F = [](test::Dummy* to_delete, void* context)
            {

            };

            // NOTE: Can't easily get our destructor in here for shared_ptr - damn
            // I see now why they passed in their Destructor type in shared_ptr constructor
            // - so instead, memory_pool_1 has s specialized behavior when it
            // encounters layer1 shared ptrs
            memory_pool_1<layer1::shared_ptr<test::Dummy>, 10> pool;
            typedef typename decltype (pool)::value_type shared_ptr;

            // NOTE: pool construct will call shared_ptr constructor,
            // but shared_ptr themselves don't auto construct their
            // managed pointer
            /*
            layer1::shared_ptr<test::Dummy>& p = pool.construct();
            layer1::shared_ptr<test::Dummy>& p3 = pool.construct();
            */
#ifdef FEATURE_ESTD_EXP_AUTOCONSTRUCT
            shared_ptr& p = pool.construct(7, "hi2u");
            shared_ptr& p3 = pool.construct(8, "hi2u!");
#else
            auto& p = pool.construct();
            auto& p3 = pool.construct();
#endif

            REQUIRE(pool.count_free() == 8);

            {
                layer3::shared_ptr<test::Dummy> p2(p);

#ifdef FEATURE_ESTD_EXP_AUTOCONSTRUCT
                REQUIRE(p.use_count() == 2);
                REQUIRE(p3.use_count() == 1);
#else
                REQUIRE(p.use_count() == 0);
                REQUIRE(p3.use_count() == 0);

                p.construct(7, "hi2u");
                p3.construct(8, "hi2u!");

                REQUIRE(p.use_count() == 1);
#endif

                p2 = p;

                REQUIRE(p.use_count() == 2);

                REQUIRE(p3.use_count() == 1);

                layer3::shared_ptr<test::Dummy> p4(p3);

                REQUIRE(p3.use_count() == 2);
                REQUIRE(p4.use_count() == 2);

                p3.reset();

                // superfluous - p4 going out of scope achieves the same affect
                //p4.reset();
            }

            REQUIRE(pool.count_free() == 9);

            REQUIRE(p.use_count() == 1);

            // now that shared_ptr's own destructor is linked in to pool.destory,
            // you must never call pool.destroy manually - otherwise it will cascade
            // out through shared_ptr's destructor and call itself again (via pool.destroy_internal)
            //pool.destroy(p);
            p.reset();  // do this instead of direct pool.destroy

            REQUIRE(pool.count_free() == 10);

            SECTION("layer3 conversions")
            {
                shared_ptr& _p = pool.construct();

                // there's a slight oddness (but maybe necessary) in that a
                // freshly constructed shared_ptr has no use count, but obviously
                // counts as allocated in the pool.  this means that you have to
                // actually use the shared_ptr if you ever expect it to be freed,
                // since its pool removal is kicked off by shared_ptr's auto destruction
                // at the moment if you want to bypass that, then in this condition
                // only you can call pool.destroy
#ifndef FEATURE_ESTD_EXP_AUTOCONSTRUCT
                _p.construct();
#endif
                REQUIRE(_p.use_count() == 1);

                /*
                 * this doesn't do what you expect because what really needs to happen
                 * is a pool.construct().construct() to activate allocated shared_ptr,
                 * otherwise the layer3 initialization ends up empty.
                 * trouble continues because even if you DO construct().construct(),
                 * this code doesn't have easy access to the initial shared_ptr which
                 * one still has to call reset() on
                layer3::shared_ptr<test::Dummy> p(pool.construct());

                REQUIRE(p.use_count() == 1); */

                REQUIRE(pool.count_free() == 9);

                int counter = 0;
                auto F2 = [&](layer3::shared_ptr<test::Dummy> p)
                {
                    counter++;
                    REQUIRE(p.use_count() == 3);
                };

                layer3::shared_ptr<test::Dummy> p = _p;

                REQUIRE(_p.use_count() == 2);

                F2(p);

                REQUIRE(counter == 1);

                F2(_p);

                REQUIRE(p.use_count() == 2);

                REQUIRE(counter == 2);

                _p.reset();

                REQUIRE(p.use_count() == 1);
            }

            REQUIRE(pool.count_free() == 10);
        }
        SECTION("memory-pool specific make_shared")
        {
            memory_pool_1<layer1::shared_ptr<test::Dummy>, 10> pool;
            typedef typename decltype (pool)::value_type shared_ptr;

            // would be better to do this kind of in reverse, where make_shared can take any allocator,
            // including a memory pool
            shared_ptr& p = experimental::make_shared(pool);

            REQUIRE(p.use_count() == 1);
            REQUIRE(pool.count_free() == 9);

            p.reset();  // will auto-destroy Dummy and free from pool

            REQUIRE(pool.count_free() == 10);
        }
        SECTION("ll pool")
        {
            // almost there, just some lingering pointer vs non pointer descrepency for handling
            // '_next'
            typedef memory_pool_ll<int, 10> pool_type;
            intrusive_forward_list<pool_type::item> list;
            pool_type pool;

            int sz = sizeof(pool_type::item);
            int expected_sz = sizeof(int) + sizeof(void*) + 4; // extra 4 because of padding on 64-bit gnu

            REQUIRE(sz == expected_sz);

            sz = sizeof(pool);

            REQUIRE(sz == (expected_sz * 10) + sizeof(void*));

            int* val1 = pool.allocate();

            REQUIRE(pool.count_free() == 9);

            *val1 = 123;

            int& val2 = pool.construct(456);

            REQUIRE(pool.count_free() == 8);

            REQUIRE(*val1 == 123);
            REQUIRE(val2 == 456);

            pool_type::item& item = pool.allocate_item();

            REQUIRE(pool.count_free() == 7);

            // NOTE: almost works - as expected, traits are different for this particular
            // node type.  I think we can specialize here
            //list.push_front(item);
        }
    }
    SECTION("instance wrapper")
    {
        using namespace estd;
        using namespace estd::experimental;

        SECTION("simplistic")
        {
            instance_wrapper<int> a;

            a.construct(5);

            REQUIRE(a == 5);
        }
        SECTION("Dummy class")
        {
            instance_wrapper<test::Dummy> a;

            a.construct(7, "hi2u");

            REQUIRE(a.value().val1 == 7);
        }
        SECTION("non copyable")
        {
            instance_wrapper<test::NonCopyable> a;

            a.construct();
        }
    }
    SECTION("streambuf")
    {
        SECTION("streambuf-traits")
        {
            char buf[] = "Hello";
            estd::span<char> span = buf;
            typedef char char_type;
            typedef estd::internal::streambuf<
                    estd::internal::impl::in_span_streambuf<char_type >> streambuf_type;
            typedef estd::experimental::streambuf_traits<streambuf_type> streambuf_traits;

            streambuf_type sb(span);

            estd::span<char> same_span = streambuf_traits::gdata(sb);

            REQUIRE(span.size() == same_span.size());
            REQUIRE(same_span[0] == buf[0]);
        }
    }
    SECTION("internal from_char prototypes")
    {
        // TODO: Move out of experimental area, concept is proven
        using namespace estd::internal;

        SECTION("base 2")
        {
            const char *src = "1010";

            short value = 0;
            from_chars_integer<char_base_traits<2> >(src, src + 4, value, 2);

            REQUIRE(value == 10);
        }
        SECTION("base 10")
        {
            SECTION("positive")
            {
                const char* src = "1234";
                int value = 0;
                from_chars_integer<char_base_traits<10> >(src, src + 4, value, 10);

                REQUIRE(value == 1234);
            }
            SECTION("negative")
            {
                const char* src = "-1234";
                int value = 0;
                from_chars_integer<char_base_traits<10> >(src, src + 5, value);

                REQUIRE(value == -1234);
            }
            SECTION("extra stuff")
            {
                estd::layer2::const_string src = "1234 hello";

                int value = 0;
                estd::from_chars_result result =
                        from_chars_integer<char_base_traits<10> >(
                                src.data(),
                          src.data() + src.size(), value);

                REQUIRE(result.ec == 0);
                REQUIRE(value == 1234);
                REQUIRE(estd::layer2::const_string(result.ptr) == " hello");
            }
        }
        SECTION("base 16")
        {
            const char *src = "FF";
            int value = 0;
            from_chars_integer<char_base_traits<16> >(src, src + 4, value);

            REQUIRE(value == 255);
        }
    }
    SECTION("from_char prototypes")
    {
        // TODO: Move out of experimental area, concept is proven

        SECTION("long")
        {
            estd::layer2::const_string s = "1234";
            long value;

            estd::from_chars_result result =
                    estd::from_chars(s.data(), s.data() + s.size(), value);

            REQUIRE(result.ec == 0);
            REQUIRE(value == 1234);
        }
        SECTION("unsigned")
        {
            estd::layer2::const_string s = "1234";
            unsigned value;

            estd::from_chars_result result =
                    estd::from_chars(s.data(), s.data() + s.size(), value);

            REQUIRE(result.ec == 0);
            REQUIRE(value == 1234);
        }
        SECTION("lazy")
        {
            const char s[128] = "1234";

            unsigned value;

            estd::from_chars_result result =
                    estd::from_chars(s, &s[0] + sizeof(s), value);

            REQUIRE(result.ec == 0);
            REQUIRE(value == 1234);
        }
    }
    SECTION("STATIC_ASSERT")
    {
        STATIC_ASSERT(true);
        //STATIC_ASSERT(false); // does indeed halt compilation, clunky though
    }
    SECTION("char_base_traits")
    {
        // TODO: Move out of experimental area, concept is proven
        using namespace estd::internal;

        SECTION("decimal")
        {
            typedef char_base_traits<10> cbt;

            REQUIRE(cbt::is_in_base('9') == true);
            REQUIRE(cbt::is_in_base('F') == false);

            REQUIRE(cbt::from_char('9') == 9);
        }
        SECTION("hexadecimal")
        {
            typedef char_base_traits<16> cbt;

            REQUIRE(cbt::is_in_base('F') == true);
            REQUIRE(cbt::is_in_base('G') == false);

            REQUIRE(cbt::from_char('9') == 9);
            REQUIRE(cbt::from_char('B') == 11);
        }
    }
}

#pragma GCC diagnostic pop
