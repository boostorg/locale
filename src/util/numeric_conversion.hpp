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
}}} // namespace boost::locale::util

#endif
