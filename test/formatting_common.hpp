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

template<typename CharType, typename IntType>
void test_parse_multi_number_by_char(const std::locale& locale)
{
    // thousandsNum will mostly be 12,345 but some systems
    // don't have the thousand separator for the POSIX locale.
    // So use the formatted output.
    const IntType expectedInt = 12345;
    std::basic_ostringstream<CharType> thousandsNum;
    thousandsNum.imbue(locale);
    thousandsNum << boost::locale::as::number << expectedInt;

    std::basic_istringstream<CharType> stream;
    stream.imbue(locale);
    stream.str(ascii_to<CharType>("42.") + thousandsNum.str());
    stream >> boost::locale::as::number;

    IntType value;
    if TEST(stream >> value) {
        TEST_EQ(value, IntType(42));
        TEST_EQ(static_cast<char>(stream.get()), '.');
        if TEST(stream >> value) {
            TEST_EQ(value, expectedInt);
            if TEST(!(stream >> value))
                TEST(stream.eof());
        }
    }

    stream.str(ascii_to<CharType>("42.25,678"));
    stream.clear();
    float fValue;
    if TEST(stream >> fValue) {
        TEST_EQ(fValue, 42.25);
        TEST_EQ(static_cast<char>(stream.get()), ',');
        if TEST(stream >> value) {
            TEST_EQ(value, IntType(678));
            if TEST(!(stream >> value))
                TEST(stream.eof());
        }
    }

    // Parsing a floating point currency to integer truncates the floating point value but fully parses it
    stream.str(ascii_to<CharType>("USD1,234.55,67.89"));
    stream.clear();
    if TEST(!(stream >> value)) {
        stream.clear();
        stream >> boost::locale::as::currency >> boost::locale::as::currency_iso;
        if(stream >> value) { // Parsing currencies not fully supported by WinAPI backend
            TEST_EQ(value, IntType(1234));
            TEST_EQ(static_cast<char>(stream.get()), ',');
            if TEST(stream >> boost::locale::as::number >> value) {
                TEST_EQ(value, IntType(67));
                TEST(!stream.eof());
            }
        }
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