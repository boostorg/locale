//
// Copyright (c) 2024-2025 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_LOCALE_IMPL_UTIL_NUMERIC_CONVERSIONS_HPP
#define BOOST_LOCALE_IMPL_UTIL_NUMERIC_CONVERSIONS_HPP

#include <boost/locale/config.hpp>
#include <boost/charconv/from_chars.hpp>
#include <boost/core/detail/string_view.hpp>
#include <algorithm>
#include <array>
#include <type_traits>
#ifdef BOOST_LOCALE_WITH_ICU
#    include <unicode/fmtable.h>
#endif

namespace boost { namespace locale { namespace util {

    template<typename Integer>
    bool try_to_int(core::string_view s, Integer& value)
    {
        if(s.size() >= 2 && s[0] == '+') {
            if(s[1] == '-') // "+-" is not allowed, invalid "+<number>" is detected by parser
                return false;
            s.remove_prefix(1);
        }
        const auto res = boost::charconv::from_chars(s, value);
        return res && res.ptr == (s.data() + s.size());
    }

    /// Parse a string in scientific format to an integer.
    /// In particular the "E notation" is used.
    /// I.e. "\d.\d+E\d+", e.g. 5.12E3 == 5120; 5E2 == 500; 2E+1 == 20)
    /// Additionally plain integers are recognized.
    template<typename Integer>
    bool try_scientific_to_int(const core::string_view s, Integer& value)
    {
        static_assert(std::is_integral<Integer>::value && std::is_unsigned<Integer>::value,
                      "Must be an  unsigned integer");
        if(s.size() < 3) // At least: iEj for E notation
            return try_to_int(s, value);
        if(s[0] == '-')
            return false;
        constexpr auto maxDigits = std::numeric_limits<Integer>::digits10 + 1;
        std::array<char, maxDigits> buffer;
        // Convert to a regular integer string without exponent or fractional
        core::string_view string_value;

        const auto expPos = s.find('E', 1);
        if(s[1] == '.') { // "Shift" the dot to right according to exponent
            int8_t exponent;
            if(BOOST_UNLIKELY(expPos == core::string_view::npos || !try_to_int(s.substr(expPos + 1), exponent)))
                return false;
            const auto numSignificantDigits = expPos - 1; // Exclude dot
            const auto numDigits = exponent + 1u;         // E0 -> 1 digit
            if(BOOST_UNLIKELY(exponent < 0 || numDigits < numSignificantDigits))
                return false; // Fractional
            else if(BOOST_UNLIKELY(numDigits > maxDigits))
                return false; // Too large

            // Copy to buffer excluding dot
            buffer[0] = s[0];
            const auto bufPos = std::copy(s.begin() + 2, s.begin() + expPos, buffer.begin() + 1);
            std::fill(bufPos, buffer.begin() + numDigits, '0');
            string_value = core::string_view(buffer.data(), numDigits);
        } else { // Pad with zeros according to exponent
            if(expPos == core::string_view::npos)
                return try_to_int(s, value); // Shortcut: Regular integer
            const core::string_view significant = s.substr(0, expPos);
            int8_t exponent;
            if(BOOST_UNLIKELY(!try_to_int(s.substr(expPos + 1), exponent)))
                return false;
            else if(BOOST_UNLIKELY(exponent < 0))
                return false; // Fractional
            else if(BOOST_UNLIKELY(exponent == 0))
                string_value = significant;
            else {
                const auto numDigits = significant.size() + exponent;
                if(BOOST_UNLIKELY(numDigits > maxDigits))
                    return false; // Too large
                else {
                    const auto bufPos = std::copy(significant.begin(), significant.end(), buffer.begin());
                    std::fill(bufPos, buffer.begin() + numDigits, '0');
                    string_value = core::string_view(buffer.data(), numDigits);
                }
            }
        }
        return try_to_int(string_value, value);
    }

#ifdef BOOST_LOCALE_WITH_ICU
    template<typename Integer>
    bool try_parse_icu(icu::Formattable& fmt, Integer& value)
    {
        // Get value as a decimal number and parse that
        UErrorCode err = U_ZERO_ERROR;
        const auto decimals = fmt.getDecimalNumber(err);
        if(U_FAILURE(err))
            return false; // Not a number
        const core::string_view s(decimals.data(), decimals.length());
        return try_scientific_to_int(s, value);
    }
#endif
}}} // namespace boost::locale::util

#endif
