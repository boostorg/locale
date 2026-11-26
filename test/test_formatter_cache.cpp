//
// Copyright (c) 2026 Alexander Grund
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/formatting.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/localization_backend.hpp>
#include "boostLocale/test/tools.hpp"
#include "boostLocale/test/unit_test.hpp"
#include <ctime>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

namespace {

// Some facet a user may install into their application locale, e.g. application settings
struct custom_facet : std::locale::facet {
    static std::locale::id id;
};
std::locale::id custom_facet::id;

// 2020-05-06 15:33:13 UTC
const std::time_t test_time = 1588779193;

template<typename Manipulator>
std::string format_time(const std::locale& loc, const Manipulator manipulator, const std::time_t time)
{
    std::ostringstream ss;
    ss.imbue(loc);
    ss << boost::locale::as::gmt << manipulator << time;
    return ss.str();
}

template<typename ValueType>
std::string format_number(const std::locale& loc, const ValueType value)
{
    std::ostringstream ss;
    ss.imbue(loc);
    ss << boost::locale::as::number << value;
    return ss.str();
}

} // namespace

void test_main(int /*argc*/, char** /*argv*/)
{
#ifndef BOOST_LOCALE_WITH_ICU
    std::cout << "ICU is not built... Skipping\n";
    return;
#else
    // The formatters_cache used for (some) date/time formatting is only used by the ICU backend
    const auto origBackendManager = boost::locale::localization_backend_manager::global();
    auto backendManager = origBackendManager;
    backendManager.select("icu");
    boost::locale::localization_backend_manager::global(backendManager);

    boost::locale::generator gen;
    // Typical application locale: en_US plus an application-specific facet
    const std::locale base = std::locale(gen("en_US.UTF-8"), new custom_facet);
    const std::locale expected = gen("de_DE.UTF-8");
    // Documented way to extend an existing locale: "Use base as a locale to which all facets
    // are added". This keeps the custom facet of the application locale.
    const std::locale extended = gen.generate(base, "de_DE.UTF-8");

    TEST(std::has_facet<custom_facet>(extended));

    // The extended locale must format dates and times like a locale generated from scratch
    // with the same ID and not like the base locale. Otherwise a stale formatters_cache
    // (containing the date/time patterns of the base locale) is used.
    {
        TEST_CONTEXT("as::date");
        TEST_EQ(format_time(extended, boost::locale::as::date, test_time),
                format_time(expected, boost::locale::as::date, test_time));
    }
    {
        TEST_CONTEXT("as::time");
        TEST_EQ(format_time(extended, boost::locale::as::time, test_time),
                format_time(expected, boost::locale::as::time, test_time));
    }
    {
        TEST_CONTEXT("as::datetime");
        TEST_EQ(format_time(extended, boost::locale::as::datetime, test_time),
                format_time(expected, boost::locale::as::datetime, test_time));
    }
    {
        TEST_CONTEXT("as::ftime(%x)");
        TEST_EQ(format_time(extended, boost::locale::as::ftime(std::string("%x")), test_time),
                format_time(expected, boost::locale::as::ftime(std::string("%x")), test_time));
    }
    {
        TEST_CONTEXT("as::ftime(%X)");
        TEST_EQ(format_time(extended, boost::locale::as::ftime(std::string("%X")), test_time),
                format_time(expected, boost::locale::as::ftime(std::string("%X")), test_time));
    }
    {
        TEST_CONTEXT("as::ftime(%c)");
        TEST_EQ(format_time(extended, boost::locale::as::ftime(std::string("%c")), test_time),
                format_time(expected, boost::locale::as::ftime(std::string("%c")), test_time));
    }

    // Combining facets of different locales is another way to get a cache that doesn't match
    // the locale of the formatting facet: numeric/formatting facets from `de`, rest from `en`.
    {
        const std::locale combined(base, expected, std::locale::numeric);
        TEST_CONTEXT("combined numeric locale");
        TEST_EQ(format_number(combined, 1234567.89), format_number(expected, 1234567.89));
        TEST_EQ(format_time(combined, boost::locale::as::date, test_time),
                format_time(expected, boost::locale::as::date, test_time));
        TEST_EQ(format_time(combined, boost::locale::as::ftime(std::string("%x")), test_time),
                format_time(expected, boost::locale::as::ftime(std::string("%x")), test_time));
    }

    boost::locale::localization_backend_manager::global(origBackendManager);
#endif
}
