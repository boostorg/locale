//
// Copyright (c) 2022-2025 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "../src/util/numeric_conversion.hpp"
#include "boostLocale/test/tools.hpp"
#include <limits>
#include <locale>
#include <sstream>

void test_try_to_int()
{
    using boost::locale::util::try_to_int;

    int v = 1337;
    if TEST(try_to_int("0", v))
        TEST_EQ(v, 0);

    if TEST(try_to_int("42", v))
        TEST_EQ(v, 42);

    if TEST(try_to_int("-1337", v))
        TEST_EQ(v, -1337);

    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    empty_stream(ss) << std::numeric_limits<int>::min();
    if TEST(try_to_int(ss.str(), v))
        TEST_EQ(v, std::numeric_limits<int>::min());
    empty_stream(ss) << std::numeric_limits<int>::max();
    if TEST(try_to_int(ss.str(), v))
        TEST_EQ(v, std::numeric_limits<int>::max());

    TEST(!try_to_int("", v));
    TEST(!try_to_int("a", v));
    TEST(!try_to_int("1.", v));
    TEST(!try_to_int("1a", v));
    TEST(!try_to_int("a1", v));
    static_assert(sizeof(long long) > sizeof(int), "Value below under/overflows!");
    empty_stream(ss) << static_cast<long long>(std::numeric_limits<int>::min()) - 1;
    TEST(!try_to_int(ss.str(), v));
    empty_stream(ss) << static_cast<long long>(std::numeric_limits<int>::max()) + 1;
    TEST(!try_to_int(ss.str(), v));
}

void test_main(int /*argc*/, char** /*argv*/)
{
    test_try_to_int();
}
