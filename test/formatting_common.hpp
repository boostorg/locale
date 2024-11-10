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

#include "../src/boost/locale/util/foreach_char.hpp"
#include "boostLocale/test/tools.hpp"
#include "boostLocale/test/unit_test.hpp"

template<typename CharType, typename IntType>
void test_parse_multi_number_by_char(const std::locale& locale)
{
    std::basic_istringstream<CharType> stream;
    stream.imbue(locale);
    stream.str(ascii_to<CharType>("42.12,345"));
    stream >> boost::locale::as::number;

    IntType value;
    TEST_REQUIRE(stream >> value);
    TEST_EQ(value, IntType(42));
    TEST_EQ(static_cast<char>(stream.get()), '.');
    TEST_REQUIRE(stream >> value);
    TEST_EQ(value, IntType(12345));
    TEST_REQUIRE(!(stream >> value));
    TEST(stream.eof());

    stream.str(ascii_to<CharType>("42.25,678"));
    stream.clear();
    float fValue;
    TEST_REQUIRE(stream >> fValue);
    TEST_EQ(fValue, 42.25);
    TEST_EQ(static_cast<char>(stream.get()), ',');
    TEST_REQUIRE(stream >> value);
    TEST_EQ(value, IntType(678));
    TEST_REQUIRE(!(stream >> value));
    TEST(stream.eof());

    // Parsing a floating point currency to integer truncates the floating point value but fully parses it
    stream.str(ascii_to<CharType>("USD1,234.55,67.89"));
    stream.clear();
    TEST_REQUIRE(!(stream >> value));
    stream.clear();
    stream >> boost::locale::as::currency >> boost::locale::as::currency_iso;
    if(stream >> value) { // Parsing currencies not fully supported by WinAPI backend
        TEST_EQ(value, IntType(1234));
        TEST_EQ(static_cast<char>(stream.get()), ',');
        TEST_REQUIRE(stream >> boost::locale::as::number >> value);
        TEST_EQ(value, IntType(67));
        TEST(!stream.eof());
    }
}

/// Test that parsing multiple numbers without any spaces works as expected
void test_parse_multi_number()
{
    const auto locale = boost::locale::generator{}("en_US.UTF-8");

#define BOOST_LOCALE_CALL_I(T, I)      \
    std::cout << "\t" #I << std::endl; \
    test_parse_multi_number_by_char<T, I>(locale);

#define BOOST_LOCALE_CALL(T)                                 \
    std::cout << "test_parse_multi_number " #T << std::endl; \
    BOOST_LOCALE_CALL_I(T, int16_t);                         \
    BOOST_LOCALE_CALL_I(T, uint16_t);                        \
    BOOST_LOCALE_CALL_I(T, int32_t);                         \
    BOOST_LOCALE_CALL_I(T, uint32_t);                        \
    BOOST_LOCALE_CALL_I(T, int64_t);                         \
    BOOST_LOCALE_CALL_I(T, uint64_t);

    BOOST_LOCALE_CALL(char);
    BOOST_LOCALE_CALL(wchar_t);
#undef BOOST_LOCALE_CALL
#undef BOOST_LOCALE_CALL_I
}