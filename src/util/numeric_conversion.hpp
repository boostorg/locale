//
// Copyright (c) 2024 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_LOCALE_IMPL_UTIL_NUMERIC_CONVERSIONS_HPP
#define BOOST_LOCALE_IMPL_UTIL_NUMERIC_CONVERSIONS_HPP

#include <boost/locale/config.hpp>
#include <boost/charconv/from_chars.hpp>
#include <boost/utility/string_view.hpp>

namespace boost { namespace locale { namespace util {

    bool try_to_int(const string_view s, int value)
    {
        if(s.empty())
            return false;
        const auto res = boost::charconv::from_chars(s, value);
        return res && res.ptr == (s.data() + s.size());
    }
}}} // namespace boost::locale::util

#endif
