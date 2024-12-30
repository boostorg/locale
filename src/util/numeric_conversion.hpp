//
// Copyright (c) 2024 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_LOCALE_IMPL_UTIL_NUMERIC_CONVERSIONS_HPP
#define BOOST_LOCALE_IMPL_UTIL_NUMERIC_CONVERSIONS_HPP

#include <boost/locale/config.hpp>
#include <boost/utility/string_view.hpp>
#include <cstdlib>
#include <errno>
#include <limits>

namespace boost { namespace locale { namespace util {

    bool try_to_int(const string_view s, int value)
    {
        if(s.empty())
            return false;
        errno = 0;
        char* end_char{};
        const auto v = std::strtol(s.c_str(), &end_char, 10);
        if(errno == ERANGE || end_char != s.c_str() + s.size())
            return false;
        if(v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max())
            return false;
        res = v;
        return true;
    }
}}} // namespace boost::locale::util

#endif
