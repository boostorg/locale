//
// Copyright (c) 2022 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/util.hpp>
#include "boostLocale/test/test_helpers.hpp"
#include "boostLocale/test/unit_test.hpp"
#include <cstdlib>

void test_get_system_locale()
{
    // Clear all -> Default to C
    {
        using boost::locale::test::unsetenv;
        unsetenv("LC_CTYPE");
        unsetenv("LC_ALL");
        unsetenv("LANG");
    }

    using boost::locale::util::get_system_locale;
#if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
    TEST_EQ(get_system_locale(false), "C");
#else
    // On Windows the user default name is used, so we can only test the encoding
    TEST(get_system_locale(true).find(".UTF-8") != std::string::npos);
    {
        const std::string loc = get_system_locale(false);
        const std::string enc = loc.substr(loc.find_last_of('.'));
        // encoding should be a windows codepage, but in the error case can be UTF-8
        if(enc.find(".windows-") != 0u)
            TEST_EQ(enc, ".UTF-8"); // LCOV_EXCL_LINE
    }
#endif
    // LC_ALL, LC_CTYPE and LANG variables used in this order
    using boost::locale::test::setenv;
    setenv("LANG", "mylang.foo");
    TEST_EQ(get_system_locale(true), "mylang.foo");
    setenv("LC_CTYPE", "this.lang");
    TEST_EQ(get_system_locale(true), "this.lang");
    setenv("LC_ALL", "barlang.bar");
    TEST_EQ(get_system_locale(true), "barlang.bar");
}

void test_main(int /*argc*/, char** /*argv*/)
{
    test_get_system_locale();
}
