//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
// Copyright (c) 2021-2022 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "boost/locale/icu/formatters_cache.hpp"
#include <boost/assert.hpp>
#include <boost/core/ignore_unused.hpp>
#include <memory>
#include <type_traits>
#ifdef BOOST_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4251) // "identifier" : class "type" needs to have dll-interface...
#endif
#include <unicode/datefmt.h>
#include <unicode/numfmt.h>
#include <unicode/rbnf.h>
#include <unicode/smpdtfmt.h>
#ifdef BOOST_MSVC
#    pragma warning(pop)
#endif

namespace boost { namespace locale { namespace impl_icu {

    std::locale::id formatters_cache::id;

    namespace {

        struct init {
            init() { ignore_unused(std::has_facet<formatters_cache>(std::locale::classic())); }
        } instance;

    } // namespace

    formatters_cache::formatters_cache(const icu::Locale& locale) : locale_(locale)
    {
        icu::DateFormat::EStyle styles[4]{};
        styles[int(format_len::Short)] = icu::DateFormat::kShort;
        styles[int(format_len::Medium)] = icu::DateFormat::kMedium;
        styles[int(format_len::Long)] = icu::DateFormat::kLong;
        styles[int(format_len::Full)] = icu::DateFormat::kFull;

        for(int i = 0; i < 4; i++) {
            std::unique_ptr<icu::DateFormat> fmt(icu::DateFormat::createDateInstance(styles[i], locale));
            icu::SimpleDateFormat* sfmt = icu_cast<icu::SimpleDateFormat>(fmt.get());
            if(sfmt)
                sfmt->toPattern(date_format_[i]);
        }

        for(int i = 0; i < 4; i++) {
            std::unique_ptr<icu::DateFormat> fmt(icu::DateFormat::createTimeInstance(styles[i], locale));
            icu::SimpleDateFormat* sfmt = icu_cast<icu::SimpleDateFormat>(fmt.get());
            if(sfmt)
                sfmt->toPattern(time_format_[i]);
        }

        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                std::unique_ptr<icu::DateFormat> fmt(
                  icu::DateFormat::createDateTimeInstance(styles[i], styles[j], locale));
                icu::SimpleDateFormat* sfmt = icu_cast<icu::SimpleDateFormat>(fmt.get());
                if(sfmt)
                    sfmt->toPattern(date_time_format_[i][j]);
            }
        }
    }

    icu::NumberFormat* formatters_cache::number_format(num_fmt_type type) const
    {
        icu::NumberFormat* ptr = number_format_[int(type)].get();
        if(ptr)
            return ptr;
        UErrorCode err = U_ZERO_ERROR;
        std::unique_ptr<icu::NumberFormat> ap;

        switch(type) {
            case num_fmt_type::number: ap.reset(icu::NumberFormat::createInstance(locale_, err)); break;
            case num_fmt_type::sci: ap.reset(icu::NumberFormat::createScientificInstance(locale_, err)); break;
#if BOOST_LOCALE_ICU_VERSION >= 408
            case num_fmt_type::curr_nat:
                ap.reset(icu::NumberFormat::createInstance(locale_, UNUM_CURRENCY, err));
                break;
            case num_fmt_type::curr_iso:
                ap.reset(icu::NumberFormat::createInstance(locale_, UNUM_CURRENCY_ISO, err));
                break;
#elif BOOST_LOCALE_ICU_VERSION >= 402
            case num_fmt_type::curr_nat:
                ap.reset(icu::NumberFormat::createInstance(locale_, icu::NumberFormat::kCurrencyStyle, err));
                break;
            case num_fmt_type::curr_iso:
                ap.reset(icu::NumberFormat::createInstance(locale_, icu::NumberFormat::kIsoCurrencyStyle, err));
                break;
#else
            case num_fmt_type::curr_nat:
            case num_fmt_type::curr_iso: ap.reset(icu::NumberFormat::createCurrencyInstance(locale_, err)); break;
#endif
            case num_fmt_type::percent: ap.reset(icu::NumberFormat::createPercentInstance(locale_, err)); break;
            case num_fmt_type::spell:
                ap.reset(new icu::RuleBasedNumberFormat(icu::URBNF_SPELLOUT, locale_, err));
                break;
            case num_fmt_type::ordinal:
                ap.reset(new icu::RuleBasedNumberFormat(icu::URBNF_ORDINAL, locale_, err));
                break;
            default: throw std::runtime_error("locale::internal error should not get there");
        }

        check_and_throw_icu_error(err, "Failed to create a formatter");
        ptr = ap.get();
        number_format_[int(type)].reset(ap.release());
        return ptr;
    }

    icu::SimpleDateFormat* formatters_cache::date_formatter() const
    {
        icu::SimpleDateFormat* p = date_formatter_.get();
        if(p)
            return p;

        std::unique_ptr<icu::DateFormat> fmt(
          icu::DateFormat::createDateTimeInstance(icu::DateFormat::kMedium, icu::DateFormat::kMedium, locale_));

        p = icu_cast<icu::SimpleDateFormat>(fmt.get());
        if(p) {
            fmt.release();
            date_formatter_.reset(p);
        }
        return p;
    }

}}} // namespace boost::locale::impl_icu
