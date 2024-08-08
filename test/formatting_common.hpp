//
// Copyright (c) 2024 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/formatting.hpp>
#include <boost/locale/generator.hpp>
#include <cstdint>
#include <limits>
#include <sstream>

#include "boostLocale/test/tools.hpp"
#include "boostLocale/test/unit_test.hpp"


template<typename CharType>
void test_format_large_number_by_char(const std::locale& locale)
{
    std::basic_ostringstream<CharType> output;
    output.imbue(locale);
    output << boost::locale::as::number;

    constexpr int64_t high_signed64 = 9223372036854775807;
    static_assert(high_signed64 == std::numeric_limits<int64_t>::max());

    empty_stream(output) << high_signed64;
    TEST_EQ(output.str(), ascii_to<CharType>("9,223,372,036,854,775,807"));
    empty_stream(output) << static_cast<uint64_t>(high_signed64);
    TEST_EQ(output.str(), ascii_to<CharType>("9,223,372,036,854,775,807"));
    empty_stream(output) << (static_cast<uint64_t>(high_signed64) + 1u);
    TEST_EQ(output.str(), ascii_to<CharType>("9,223,372,036,854,775,808"));
}

void test_format_large_number()
{
    const auto locale = boost::locale::generator{}("en_US.UTF-8");

    std::cout << "Testing char" << std::endl;
    test_format_large_number_by_char<char>(locale);

    std::cout << "Testing wchar_t" << std::endl;
    test_format_large_number_by_char<wchar_t>(locale);

#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    std::cout << "Testing char16_t" << std::endl;
    test_format_large_number_by_char<char16_t>(locale);
#endif
}