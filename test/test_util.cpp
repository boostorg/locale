//
// Copyright (c) 2022 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/util.hpp>
#include <boost/locale/util/locale_data.hpp>
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
#if !BOOST_LOCALE_USE_WIN32_API
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

void test_locale_data()
{
    boost::locale::util::locale_data data;
    // Default is C.US-ASCII
    TEST_EQ(data.language(), "C");
    TEST_EQ(data.country(), "");
    TEST_EQ(data.encoding(), "US-ASCII");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("en_US.UTF-8");
    TEST_EQ(data.language(), "en");
    TEST_EQ(data.country(), "US");
    TEST_EQ(data.encoding(), "UTF-8");
    TEST(data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("C");
    TEST_EQ(data.language(), "C");
    TEST_EQ(data.country(), "");
    TEST_EQ(data.encoding(), "US-ASCII");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("ku_TR.UTF-8@sorani");
    TEST_EQ(data.language(), "ku");
    TEST_EQ(data.country(), "TR");
    TEST_EQ(data.encoding(), "UTF-8");
    TEST(data.is_utf8());
    TEST_EQ(data.variant(), "sorani");

    data.parse("POSIX");
    TEST_EQ(data.language(), "C");
    TEST_EQ(data.country(), "");
    TEST_EQ(data.encoding(), "US-ASCII");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("da_DK.ISO8859-15@euro");
    TEST_EQ(data.language(), "da");
    TEST_EQ(data.country(), "DK");
    TEST_EQ(data.encoding(), "ISO8859-15");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "euro");

    data.parse("de_DE.ISO8859-1");
    TEST_EQ(data.language(), "de");
    TEST_EQ(data.country(), "DE");
    TEST_EQ(data.encoding(), "ISO8859-1");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("ja_JP.eucJP");
    TEST_EQ(data.language(), "ja");
    TEST_EQ(data.country(), "JP");
    TEST_EQ(data.encoding(), "EUCJP");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("ko_KR.EUC@dict");
    TEST_EQ(data.language(), "ko");
    TEST_EQ(data.country(), "KR");
    TEST_EQ(data.encoding(), "EUC");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "dict");

    data.parse("th_TH.TIS620");
    TEST_EQ(data.language(), "th");
    TEST_EQ(data.country(), "TH");
    TEST_EQ(data.encoding(), "TIS620");
    TEST(!data.is_utf8());
    TEST_EQ(data.variant(), "");

    data.parse("zh_TW.UTF-8@radical");
    TEST_EQ(data.language(), "zh");
    TEST_EQ(data.country(), "TW");
    TEST_EQ(data.encoding(), "UTF-8");
    TEST(data.is_utf8());
    TEST_EQ(data.variant(), "radical");

    // to_string yields the input (if format is correct already)
    for(const std::string name : {"C",
                                  "en_US.UTF-8",
                                  "ku_TR.UTF-8@sorani",
                                  "da_DK.ISO8859-15@euro",
                                  "de_DE.ISO8859-1",
                                  "en_US",
                                  "ko_KR.EUC@dict",
                                  "th_TH.TIS620",
                                  "zh_TW.UTF-8@radical"})
    {
        data.parse(name);
        TEST_EQ(data.to_string(), name);
    }

    // Unify casing:
    // - language: lowercase
    // - region: uppercase
    // - encoding: uppercase
    // - variant: lowercase
    data.parse("EN_us.utf-8@EUro");
    TEST_EQ(data.language(), "en");
    TEST_EQ(data.country(), "US");
    TEST_EQ(data.encoding(), "UTF-8");
    TEST(data.is_utf8());
    TEST_EQ(data.variant(), "euro");
    TEST_EQ(data.to_string(), "en_US.UTF-8@euro");
    data.parse("lAnGUagE_cOunTRy.eNCo-d123inG@Va-r1_Ant");
    TEST_EQ(data.to_string(), "language_COUNTRY.ENCO-D123ING@va-r1_ant");
}

void test_main(int /*argc*/, char** /*argv*/)
{
    test_get_system_locale();
    test_locale_data();
}
