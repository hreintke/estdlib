#include <catch.hpp>

#include "estd/array.h"
//#include "estd/exp/buffer.h"
#include "mem.h"
#include "test-data.h"
#include <estd/string.h>
#include <estd/exp/memory_pool.h>
#include <estd/memory.h>
#include <estd/functional.h>

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
            typedef estd::internal::deduce_fixed_size_t<10> deducer_t;
            REQUIRE(sizeof(deducer_t::size_type) == 1);
        }
        {
            typedef estd::internal::deduce_fixed_size_t<100> deducer_t;
            REQUIRE(sizeof(deducer_t::size_type) == 1);
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
            estd::experimental::memory_pool_1<int, 10> pool;

            int* i = pool.allocate();
            REQUIRE(pool.count_free() == 9);
            int& i2 = pool.construct(3);
            REQUIRE(pool.count_free() == 8);

            pool.deallocate(i);
            REQUIRE(pool.count_free() == 9);

            REQUIRE(i2 == 3);

            pool.destroy(i2);
            REQUIRE(pool.count_free() == 10);
        }
        SECTION("advanced shared_ptr pool")
        {
            typedef void (*deleter_fn)(test::Dummy* to_delete, void* context);
            deleter_fn F = [](test::Dummy* to_delete, void* context)
            {

            };

            // NOTE: Can't easily get our destructor in here for shared_ptr - damn
            // I see now why they passed in their Destructor type in shared_ptr constructor
            memory_pool_1<layer1::shared_ptr<test::Dummy>, 10> pool;

            // NOTE: pool construct will call shared_ptr constructor,
            // but shared_ptr themselves don't auto construct their
            // managed pointer
            /*
            layer1::shared_ptr<test::Dummy>& p = pool.construct();
            layer1::shared_ptr<test::Dummy>& p3 = pool.construct();
            */
            auto& p = pool.construct();
            auto& p3 = pool.construct();

            REQUIRE(pool.count_free() == 8);

            {
                layer3::shared_ptr<test::Dummy> p2(p);

                REQUIRE(p.use_count() == 0);

                p.construct(7, "hi2u");
                p3.construct(8, "hi2u!");

                REQUIRE(p.use_count() == 1);

                p2 = p;

                REQUIRE(p.use_count() == 2);

                layer3::shared_ptr<test::Dummy> p4(p3);

                REQUIRE(p4.use_count() == 2);

                p3.reset();
            }

            REQUIRE(p.use_count() == 1);

            pool.destroy(p);

            REQUIRE(pool.count_free() == 9);
        }
    }
}
