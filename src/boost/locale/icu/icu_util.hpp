//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
// Copyright (c) 2021-2022 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_SRC_ICU_UTIL_HPP
#define BOOST_SRC_ICU_UTIL_HPP

#include <boost/locale/config.hpp>
#ifdef BOOST_HAS_STDINT_H
#include <stdint.h> // Avoid ICU defining e.g. INT8_MIN causing macro redefinition warnings
#endif
#include <unicode/utypes.h>
#include <unicode/uversion.h>
#include <stdexcept>

#define BOOST_LOCALE_ICU_VERSION (U_ICU_VERSION_MAJOR_NUM * 100 + U_ICU_VERSION_MINOR_NUM)

namespace boost {
namespace locale {
namespace impl_icu {

    inline void throw_icu_error(UErrorCode err)
    {
        throw std::runtime_error(u_errorName(err));
    }

    inline void check_and_throw_icu_error(UErrorCode err)
    {
        if(U_FAILURE(err))
            throw_icu_error(err);
    }

    /// Cast a pointer to an ICU object to a pointer to TargetType
    /// using RTTI or ICUs "poor man's RTTI" to make it work with e.g. libc++ and hidden visibility
    template<class TargetType, class SourceType>
    TargetType* icu_cast(SourceType* p)
    {
        TargetType* result = dynamic_cast<TargetType*>(p);
        if(!result && p && p->getDynamicClassID() == TargetType::getStaticClassID())
            result = static_cast<TargetType*>(p);
        return result;
    }
} // impl
} // locale
} // boost

#endif
