//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
// Copyright (c) 2021-2022 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define BOOST_LOCALE_SOURCE
#include "boost/locale/icu/formatter.hpp"
#include <boost/locale/formatting.hpp>
#include <boost/locale/hold_ptr.hpp>
#include <boost/locale/info.hpp>
#include "boost/locale/icu/formatters_cache.hpp"
#include "boost/locale/icu/icu_util.hpp"
#include "boost/locale/icu/time_zone.hpp"
#include "boost/locale/icu/uconv.hpp"
#include <boost/core/ignore_unused.hpp>
#include <limits>
#include <memory>
#include <unicode/datefmt.h>
#include <unicode/decimfmt.h>
#include <unicode/numfmt.h>
#include <unicode/rbnf.h>
#include <unicode/smpdtfmt.h>

#ifdef BOOST_MSVC
#    pragma warning(disable : 4244) // lose data
#endif

namespace boost { namespace locale { namespace impl_icu {

    namespace {
        // Set the min/max fraction digits for the NumberFormat
        void set_fraction_digits(icu::NumberFormat& nf, const std::ios_base::fmtflags how, std::streamsize precision)
        {
#if BOOST_LOCALE_ICU_VERSION >= 5601
            // Since ICU 56.1 the integer part counts to the fraction part
            if(how == std::ios_base::scientific)
                precision += nf.getMaximumIntegerDigits();
#endif
            nf.setMaximumFractionDigits(precision);
            if(how == std::ios_base::scientific || how == std::ios_base::fixed) {
                nf.setMinimumFractionDigits(precision);
            } else {
                nf.setMinimumFractionDigits(0);
            }
        }
    } // namespace

    template<typename CharType>
    class number_format : public formatter<CharType> {
    public:
        typedef CharType char_type;
        typedef std::basic_string<CharType> string_type;

        number_format(icu::NumberFormat* fmt, std::string codepage): cvt_(codepage), icu_fmt_(fmt) {}

        string_type format(double value, size_t& code_points) const override
        {
            icu::UnicodeString tmp;
            icu_fmt_->format(value, tmp);
            code_points = tmp.countChar32();
            return cvt_.std(tmp);
        }
        string_type format(int64_t value, size_t& code_points) const override
        {
            icu::UnicodeString tmp;
            icu_fmt_->format(value, tmp);
            code_points = tmp.countChar32();
            return cvt_.std(tmp);
        }

        string_type format(int32_t value, size_t& code_points) const override
        {
            icu::UnicodeString tmp;
            icu_fmt_->format(value, tmp);
            code_points = tmp.countChar32();
            return cvt_.std(tmp);
        }

        size_t parse(const string_type& str, double& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int64_t& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int32_t& value) const override { return do_parse(str, value); }

    private:
        bool get_value(double& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            v = fmt.getDouble(err);
            if(U_FAILURE(err))
                return false;
            return true;
        }

        bool get_value(int64_t& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            v = fmt.getInt64(err);
            if(U_FAILURE(err))
                return false;
            return true;
        }

        bool get_value(int32_t& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            v = fmt.getLong(err);
            if(U_FAILURE(err))
                return false;
            return true;
        }

        template<typename ValueType>
        size_t do_parse(const string_type& str, ValueType& v) const
        {
            icu::Formattable val;
            icu::ParsePosition pp;
            icu::UnicodeString tmp = cvt_.icu(str.data(), str.data() + str.size());

            icu_fmt_->parse(tmp, val, pp);

            ValueType tmp_v;

            if(pp.getIndex() == 0 || !get_value(tmp_v, val))
                return 0;
            size_t cut = cvt_.cut(tmp, str.data(), str.data() + str.size(), pp.getIndex());
            if(cut == 0)
                return 0;
            v = tmp_v;
            return cut;
        }

        icu_std_converter<CharType> cvt_;
        icu::NumberFormat* icu_fmt_;
    };

    template<typename CharType>
    class date_format : public formatter<CharType> {
    public:
        typedef CharType char_type;
        typedef std::basic_string<CharType> string_type;

        string_type format(double value, size_t& code_points) const override { return do_format(value, code_points); }
        string_type format(int64_t value, size_t& code_points) const override { return do_format(value, code_points); }

        string_type format(int32_t value, size_t& code_points) const override { return do_format(value, code_points); }

        size_t parse(const string_type& str, double& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int64_t& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int32_t& value) const override { return do_parse(str, value); }

        date_format(icu::DateFormat* fmt, bool transfer_owneship, std::string codepage) : cvt_(codepage)
        {
            if(transfer_owneship) {
                aicu_fmt_.reset(fmt);
                icu_fmt_ = aicu_fmt_.get();
            } else {
                icu_fmt_ = fmt;
            }
        }

    private:
        template<typename ValueType>
        size_t do_parse(const string_type& str, ValueType& value) const
        {
            icu::ParsePosition pp;
            icu::UnicodeString tmp = cvt_.icu(str.data(), str.data() + str.size());

            UDate udate = icu_fmt_->parse(tmp, pp);
            if(pp.getIndex() == 0)
                return 0;
            double date = udate / 1000.0;
            typedef std::numeric_limits<ValueType> limits_type;
            // Explicit cast to double to avoid warnings changing value (e.g. for INT64_MAX -> double)
            if(date > static_cast<double>(limits_type::max()) || date < static_cast<double>(limits_type::min()))
                return 0;
            size_t cut = cvt_.cut(tmp, str.data(), str.data() + str.size(), pp.getIndex());
            if(cut == 0)
                return 0;
            // Handle the edge case where the double is slightly out of range and hence the cast would be UB
            // by rounding to the min/max values
            if(date == static_cast<double>(limits_type::max()))
                value = limits_type::max();
            else if(date == static_cast<double>(limits_type::min()))
                value = limits_type::min();
            else
                value = static_cast<ValueType>(date);
            return cut;
        }

        string_type do_format(double value, size_t& codepoints) const
        {
            UDate date = value * 1000.0; /// UDate is time_t in milliseconds
            icu::UnicodeString tmp;
            icu_fmt_->format(date, tmp);
            codepoints = tmp.countChar32();
            return cvt_.std(tmp);
        }

        icu_std_converter<CharType> cvt_;
        hold_ptr<icu::DateFormat> aicu_fmt_;
        icu::DateFormat* icu_fmt_;
    };

    icu::UnicodeString strftime_to_icu_full(icu::DateFormat* dfin, const char* alt)
    {
        std::unique_ptr<icu::DateFormat> df(dfin);
        icu::SimpleDateFormat* sdf = icu_cast<icu::SimpleDateFormat>(df.get());
        icu::UnicodeString result;
        if(sdf)
            sdf->toPattern(result);
        else
            result = alt;
        return result;
    }

    icu::UnicodeString strftime_to_icu_symbol(char c, const icu::Locale& locale, const formatters_cache* cache = 0)
    {
        switch(c) {
            case 'a': // Abbr Weekday
                return "EE";
            case 'A': // Full Weekday
                return "EEEE";
            case 'b': // Abbr Month
                return "MMM";
            case 'B': // Full Month
                return "MMMM";
            case 'c': // DateTime Full
            {
                if(cache)
                    return cache->date_time_format(format_len::Full, format_len::Full);
                return strftime_to_icu_full(
                  icu::DateFormat::createDateTimeInstance(icu::DateFormat::kFull, icu::DateFormat::kFull, locale),
                  "yyyy-MM-dd HH:mm:ss");
            }
            // not supported by ICU ;(
            //  case 'C': // Century -> 1980 -> 19
            case 'd': // Day of Month [01,31]
                return "dd";
            case 'D': // %m/%d/%y
                return "MM/dd/yy";
            case 'e': // Day of Month [1,31]
                return "d";
            case 'h': // == b
                return "MMM";
            case 'H': // 24 clock hour 00,23
                return "HH";
            case 'I': // 12 clock hour 01,12
                return "hh";
            case 'j': // day of year 001,366
                return "D";
            case 'm': // month as [01,12]
                return "MM";
            case 'M': // minute [00,59]
                return "mm";
            case 'n': // \n
                return "\n";
            case 'p': // am-pm
                return "a";
            case 'r': // time with AM/PM %I:%M:%S %p
                return "hh:mm:ss a";
            case 'R': // %H:%M
                return "HH:mm";
            case 'S': // second [00,61]
                return "ss";
            case 't': // \t
                return "\t";
            case 'T': // %H:%M:%S
                return "HH:mm:ss";
                /*          case 'u': // weekday 1,7 1=Monday
                            case 'U': // week number of year [00,53] Sunday first
                            case 'V': // week number of year [01,53] Moday first
                            case 'w': // weekday 0,7 0=Sunday
                            case 'W': // week number of year [00,53] Moday first, */
            case 'x': // Date
            {
                if(cache)
                    return cache->date_format(format_len::Medium);
                return strftime_to_icu_full(icu::DateFormat::createDateInstance(icu::DateFormat::kMedium, locale),
                                            "yyyy-MM-dd");
            }
            case 'X': // Time
            {
                if(cache)
                    return cache->time_format(format_len::Medium);
                return strftime_to_icu_full(icu::DateFormat::createTimeInstance(icu::DateFormat::kMedium, locale),
                                            "HH:mm:ss");
            }
            case 'y': // Year [00-99]
                return "yy";
            case 'Y': // Year 1998
                return "yyyy";
            case 'Z': // timezone
                return "vvvv";
            case '%': // %
                return "%";
            default: return "";
        }
    }

    icu::UnicodeString strftime_to_icu(const icu::UnicodeString& ftime, const icu::Locale& locale)
    {
        const unsigned len = ftime.length();
        icu::UnicodeString result;
        bool escaped = false;
        for(unsigned i = 0; i < len; i++) {
            UChar c = ftime[i];
            if(c == '%') {
                i++;
                c = ftime[i];
                if(c == 'E' || c == 'O') {
                    i++;
                    c = ftime[i];
                }
                if(escaped) {
                    result += "'";
                    escaped = false;
                }
                result += strftime_to_icu_symbol(c, locale);
            } else if(c == '\'') {
                result += "''";
            } else {
                if(!escaped) {
                    result += "'";
                    escaped = true;
                }
                result += c;
            }
        }
        if(escaped)
            result += "'";
        return result;
    }

    template<typename CharType>
    formatter<CharType>* generate_formatter(std::ios_base& ios, const icu::Locale& locale, const std::string& encoding)
    {
        using namespace boost::locale::flags;

        std::unique_ptr<formatter<CharType>> fmt;
        ios_info& info = ios_info::get(ios);
        uint64_t disp = info.display_flags();

        const formatters_cache& cache = std::use_facet<formatters_cache>(ios.getloc());

        if(disp == posix)
            return fmt.release();

        UErrorCode err = U_ZERO_ERROR;

        switch(disp) {
            case number: {
                std::ios_base::fmtflags how = (ios.flags() & std::ios_base::floatfield);
                icu::NumberFormat* nf = 0;

                if(how == std::ios_base::scientific)
                    nf = cache.number_format(num_fmt_type::sci);
                else
                    nf = cache.number_format(num_fmt_type::number);

                set_fraction_digits(*nf, how, ios.precision());
                fmt.reset(new number_format<CharType>(nf, encoding));
            } break;
            case currency: {
                icu::NumberFormat* nf;

                uint64_t curr = info.currency_flags();

                if(curr == currency_default || curr == currency_national)
                    nf = cache.number_format(num_fmt_type::curr_nat);
                else
                    nf = cache.number_format(num_fmt_type::curr_iso);

                fmt.reset(new number_format<CharType>(nf, encoding));
            } break;
            case percent: {
                icu::NumberFormat* nf = cache.number_format(num_fmt_type::percent);
                set_fraction_digits(*nf, ios.flags() & std::ios_base::floatfield, ios.precision());
                fmt.reset(new number_format<CharType>(nf, encoding));

            } break;
            case spellout:
                fmt.reset(new number_format<CharType>(cache.number_format(num_fmt_type::spell), encoding));
                break;
            case ordinal:
                fmt.reset(new number_format<CharType>(cache.number_format(num_fmt_type::ordinal), encoding));
                break;
            case date:
            case time:
            case datetime:
            case strftime: {
                using namespace flags;
                std::unique_ptr<icu::DateFormat> adf;
                icu::DateFormat* df = 0;
                icu::SimpleDateFormat* sdf = cache.date_formatter();
                // try to use cached first
                if(sdf) {
                    format_len tmf;
                    switch(info.time_flags()) {
                        case time_short: tmf = format_len::Short; break;
                        case time_long: tmf = format_len::Long; break;
                        case time_full: tmf = format_len::Full; break;
                        case time_default:
                        case time_medium:
                        default: tmf = format_len::Medium;
                    }
                    format_len dtf;
                    switch(info.date_flags()) {
                        case date_short: dtf = format_len::Short; break;
                        case date_long: dtf = format_len::Long; break;
                        case date_full: dtf = format_len::Full; break;
                        case date_default:
                        case date_medium:
                        default: dtf = format_len::Medium;
                    }

                    icu::UnicodeString pattern;
                    switch(disp) {
                        case date: pattern = cache.date_format(dtf); break;
                        case time: pattern = cache.time_format(tmf); break;
                        case datetime: pattern = cache.date_time_format(dtf, tmf); break;
                        case strftime: {
                            if(!cache.date_format(format_len::Medium).isEmpty()
                               && !cache.time_format(format_len::Medium).isEmpty()
                               && !cache.date_time_format(format_len::Medium, format_len::Medium).isEmpty())
                            {
                                icu_std_converter<CharType> cvt_(encoding);
                                const std::basic_string<CharType>& f = info.date_time_pattern<CharType>();
                                pattern = strftime_to_icu(cvt_.icu(f.c_str(), f.c_str() + f.size()), locale);
                            }
                        } break;
                    }
                    if(!pattern.isEmpty()) {
                        sdf->applyPattern(pattern);
                        df = sdf;
                    }
                    sdf = nullptr;
                }

                if(!df) {
                    icu::DateFormat::EStyle dstyle = icu::DateFormat::kDefault;
                    icu::DateFormat::EStyle tstyle = icu::DateFormat::kDefault;

                    switch(info.time_flags()) {
                        case time_short: tstyle = icu::DateFormat::kShort; break;
                        case time_medium: tstyle = icu::DateFormat::kMedium; break;
                        case time_long: tstyle = icu::DateFormat::kLong; break;
                        case time_full: tstyle = icu::DateFormat::kFull; break;
                    }
                    switch(info.date_flags()) {
                        case date_short: dstyle = icu::DateFormat::kShort; break;
                        case date_medium: dstyle = icu::DateFormat::kMedium; break;
                        case date_long: dstyle = icu::DateFormat::kLong; break;
                        case date_full: dstyle = icu::DateFormat::kFull; break;
                    }

                    if(disp == date)
                        adf.reset(icu::DateFormat::createDateInstance(dstyle, locale));
                    else if(disp == time)
                        adf.reset(icu::DateFormat::createTimeInstance(tstyle, locale));
                    else if(disp == datetime)
                        adf.reset(icu::DateFormat::createDateTimeInstance(dstyle, tstyle, locale));
                    else { // strftime
                        icu_std_converter<CharType> cvt_(encoding);
                        const std::basic_string<CharType>& f = info.date_time_pattern<CharType>();
                        icu::UnicodeString pattern = strftime_to_icu(cvt_.icu(f.data(), f.data() + f.size()), locale);
                        adf.reset(new icu::SimpleDateFormat(pattern, locale, err));
                    }
                    if(U_FAILURE(err))
                        return fmt.release();
                    df = adf.get();
                }

                df->adoptTimeZone(get_time_zone(info.time_zone()));

                // Depending if we own formatter or not
                if(adf.get())
                    fmt.reset(new date_format<CharType>(adf.release(), true, encoding));
                else
                    fmt.reset(new date_format<CharType>(df, false, encoding));
            } break;
        }

        return fmt.release();
    }

    template<>
    formatter<char>* formatter<char>::create(std::ios_base& ios, const icu::Locale& l, const std::string& e)
    {
        return generate_formatter<char>(ios, l, e);
    }

    template<>
    formatter<wchar_t>* formatter<wchar_t>::create(std::ios_base& ios, const icu::Locale& l, const std::string& e)
    {
        return generate_formatter<wchar_t>(ios, l, e);
    }

#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    template<>
    formatter<char16_t>* formatter<char16_t>::create(std::ios_base& ios, const icu::Locale& l, const std::string& e)
    {
        return generate_formatter<char16_t>(ios, l, e);
    }

#endif

#ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    template<>
    formatter<char32_t>* formatter<char32_t>::create(std::ios_base& ios, const icu::Locale& l, const std::string& e)
    {
        return generate_formatter<char32_t>(ios, l, e);
    }

#endif

}}} // namespace boost::locale::impl_icu

// boostinspect:nominmax
