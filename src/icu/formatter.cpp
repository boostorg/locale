//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
// Copyright (c) 2021-2024 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "formatter.hpp"
#include <boost/locale/formatting.hpp>
#include <boost/locale/info.hpp>
#include "../util/foreach_char.hpp"
#include "formatters_cache.hpp"
#include "icu_util.hpp"
#include "time_zone.hpp"
#include "uconv.hpp"
#include <boost/assert.hpp>
#include <boost/charconv/detail/parser.hpp>
#include <boost/charconv/to_chars.hpp>
#include <array>
#include <limits>
#include <memory>
#include <sstream>
#ifdef BOOST_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4251) // "identifier" : class "type" needs to have dll-interface...
#endif
#include <unicode/datefmt.h>
#include <unicode/decimfmt.h>
#include <unicode/numfmt.h>
#include <unicode/rbnf.h>
#include <unicode/smpdtfmt.h>

#ifdef BOOST_MSVC
#    pragma warning(pop)
#    pragma warning(disable : 4244) // lose data
#endif

namespace boost { namespace locale { namespace impl_icu {

    namespace {

        // Create lookup table where: powers_of_10[i] == 10**i
        constexpr uint64_t pow10(unsigned exponent)
        {
            return (exponent == 0) ? 1 : pow10(exponent - 1) * 10u;
        }
        template<bool condition, std::size_t Length>
        using array_if_true = typename std::enable_if<condition, std::array<uint64_t, Length>>::type;

        template<std::size_t Length, typename... Values>
        constexpr array_if_true<sizeof...(Values) == Length, Length> make_powers_of_10(Values... values)
        {
            return {values...};
        }
        template<std::size_t Length, typename... Values>
        constexpr array_if_true<sizeof...(Values) < Length, Length> make_powers_of_10(Values... values)
        {
            return make_powers_of_10<Length>(values..., pow10(sizeof...(Values)));
        }
        constexpr auto powers_of_10 = make_powers_of_10<std::numeric_limits<uint64_t>::digits10 + 1>();
        static_assert(powers_of_10[0] == 1u, "!");
        static_assert(powers_of_10[1] == 10u, "!");
        static_assert(powers_of_10[5] == 100000u, "!");

        // Set the min/max fraction digits for the NumberFormat
        void set_fraction_digits(icu::NumberFormat& nf, const std::ios_base::fmtflags how, std::streamsize precision)
        {
#if BOOST_LOCALE_ICU_VERSION >= 5601
            // Since ICU 56.1 the integer part counts to the fraction part
            if(how == std::ios_base::scientific)
                precision += nf.getMaximumIntegerDigits();
#endif
            nf.setMaximumFractionDigits(precision);
            if(how == std::ios_base::scientific || how == std::ios_base::fixed)
                nf.setMinimumFractionDigits(precision);
            else
                nf.setMinimumFractionDigits(0);
        }
    } // namespace

    template<typename CharType>
    class number_format : public formatter<CharType> {
    public:
        typedef std::basic_string<CharType> string_type;

        number_format(icu::NumberFormat& fmt, const std::string& codepage, bool isNumberOnly = false) :
            cvt_(codepage), icu_fmt_(fmt), isNumberOnly_(isNumberOnly)
        {}

        string_type format(double value, size_t& code_points) const override { return do_format(value, code_points); }
        string_type format(int64_t value, size_t& code_points) const override { return do_format(value, code_points); }
        string_type format(int32_t value, size_t& code_points) const override { return do_format(value, code_points); }
        size_t parse(const string_type& str, double& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, uint64_t& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int64_t& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int32_t& value) const override { return do_parse(str, value); }

        string_type format(const uint64_t value, size_t& code_points) const override
        {
            // ICU only supports int64_t as the largest integer type
            if(value <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                return format(static_cast<int64_t>(value), code_points);

            // Fallback to using a StringPiece (decimal number) as input
            char buffer[std::numeric_limits<uint64_t>::digits10 + 2];
            auto res = boost::charconv::to_chars(buffer, std::end(buffer), value);
            BOOST_ASSERT(res);
            BOOST_ASSERT(res.ptr < std::end(buffer));
            *res.ptr = '\0'; // ICU expects a NULL-terminated string even for the StringPiece
            icu::UnicodeString tmp;
            UErrorCode err = U_ZERO_ERROR;
            icu_fmt_.format(icu::StringPiece(buffer, res.ptr - buffer), tmp, nullptr, err);
            check_and_throw_icu_error(err);
            code_points = tmp.countChar32();
            return cvt_.std(tmp);
        }

    private:
        bool get_value(double& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            v = fmt.getDouble(err);
            return U_SUCCESS(err);
        }

        bool get_value(int64_t& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            v = fmt.getInt64(err);
            return U_SUCCESS(err);
        }

        bool get_value(uint64_t& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            // ICU only supports int64_t as the largest integer type
            const int64_t tmp = fmt.getInt64(err);
            if(U_SUCCESS(err)) {
                if(tmp < 0)
                    return false;
                v = static_cast<uint64_t>(tmp);
                return true;
            }
            // Get value as a decimal number and parse that
            err = U_ZERO_ERROR;
            const auto decimals = fmt.getDecimalNumber(err);
            if(U_FAILURE(err))
                return false; // Not a number

            // Parse raw value as ICU might return 9223372036854775810 as 9.22337203685477581E+18
            uint64_t sig;
            int32_t exp;
            bool sign;
            const char* end_decimal = decimals.data() + decimals.length();
            const auto res = boost::charconv::detail::parser(decimals.data(), end_decimal, sign, sig, exp);
            // The ICU value should always be valid
            BOOST_ASSERT_MSG(res, "Failed to parse integer representation of ICU");
            BOOST_ASSERT_MSG(res.ptr == end_decimal, "Did not fully parse integer representation of ICU");
            BOOST_ASSERT_MSG(exp >= 0, "ICU truncated parsed integer");
            // Gracefully handle if it is not the case, i.e. likely a bug in ICU
            if(BOOST_UNLIKELY(!res || res.ptr != end_decimal || exp < 0))
                return false; // LCOV_EXCL_LINE
            if(sign)          // Negative value
                return false;
            if(exp >= powers_of_10.size())
                return false; // Out of range
            v = sig * powers_of_10[exp];
            return true;
        }

        bool get_value(int32_t& v, icu::Formattable& fmt) const
        {
            UErrorCode err = U_ZERO_ERROR;
            v = fmt.getLong(err);
            return U_SUCCESS(err);
        }

        template<typename ValueType>
        string_type do_format(ValueType value, size_t& code_points) const
        {
            icu::UnicodeString tmp;
            icu_fmt_.format(value, tmp);
            code_points = tmp.countChar32();
            return cvt_.std(tmp);
        }

        template<typename ValueType>
        size_t do_parse(const string_type& str, ValueType& v) const
        {
            icu::Formattable val;
            icu::ParsePosition pp;
            icu::UnicodeString tmp = cvt_.icu(str.data(), str.data() + str.size());

            // For the plain number parsing (no currency etc) parse "123.456" as 2 ints
            // not a float later converted to int
            icu_fmt_.setParseIntegerOnly(std::is_integral<ValueType>::value && isNumberOnly_);
            icu_fmt_.parse(tmp, val, pp);

            if(pp.getIndex() == 0 || !get_value(v, val))
                return 0;
            size_t cut = cvt_.cut(tmp, str.data(), str.data() + str.size(), pp.getIndex());
            if(cut == 0)
                return 0;
            return cut;
        }

        icu_std_converter<CharType> cvt_;
        icu::NumberFormat& icu_fmt_;
        const bool isNumberOnly_;
    };

    template<typename CharType>
    class date_format : public formatter<CharType> {
    public:
        typedef std::basic_string<CharType> string_type;

        string_type format(double value, size_t& code_points) const override { return do_format(value, code_points); }
        string_type format(uint64_t value, size_t& code_points) const override { return do_format(value, code_points); }
        string_type format(int64_t value, size_t& code_points) const override { return do_format(value, code_points); }
        string_type format(int32_t value, size_t& code_points) const override { return do_format(value, code_points); }
        size_t parse(const string_type& str, double& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, uint64_t& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int64_t& value) const override { return do_parse(str, value); }
        size_t parse(const string_type& str, int32_t& value) const override { return do_parse(str, value); }

        date_format(icu::DateFormat& fmt, const std::string& encoding) : cvt_(encoding), icu_fmt_(fmt) {}
        date_format(std::unique_ptr<icu::DateFormat> fmt, const std::string& encoding) :
            cvt_(encoding), icu_fmt_holder_(std::move(fmt)), icu_fmt_(*icu_fmt_holder_)
        {}

    private:
        template<typename ValueType>
        size_t do_parse(const string_type& str, ValueType& value) const
        {
            icu::ParsePosition pp;
            icu::UnicodeString tmp = cvt_.icu(str.data(), str.data() + str.size());

            UDate udate = icu_fmt_.parse(tmp, pp);
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
            UDate date = value * 1000.0; // UDate is time_t in milliseconds
            icu::UnicodeString tmp;
            icu_fmt_.format(date, tmp);
            codepoints = tmp.countChar32();
            return cvt_.std(tmp);
        }

        icu_std_converter<CharType> cvt_;
        std::unique_ptr<icu::DateFormat> icu_fmt_holder_;
        icu::DateFormat& icu_fmt_;
    };

    icu::UnicodeString strftime_symbol_to_icu(const char c, const formatters_cache& cache)
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
            case 'c': // DateTime
                return cache.default_date_time_format();
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
                            case 'V': // week number of year [01,53] Monday first
                            case 'w': // weekday 0,7 0=Sunday
                            case 'W': // week number of year [00,53] Monday first, */
            case 'x': // Date
                return cache.default_date_format();
            case 'X': // Time
                return cache.default_time_format();
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

    icu::UnicodeString strftime_to_icu(const icu::UnicodeString& ftime, const formatters_cache& cache)
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
                result += strftime_symbol_to_icu(c, cache);
            } else if(c == '\'')
                result += "''";
            else {
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

    format_len time_flags_to_len(const uint64_t time_flags)
    {
        switch(time_flags) {
            using namespace boost::locale::flags;
            case time_short: return format_len::Short;
            case time_medium: return format_len::Medium;
            case time_long: return format_len::Long;
            case time_full: return format_len::Full;
            default: return format_len::Medium;
        }
    }
    format_len date_flags_to_len(const uint64_t date_flags)
    {
        switch(date_flags) {
            using namespace boost::locale::flags;
            case date_short: return format_len::Short;
            case date_medium: return format_len::Medium;
            case date_long: return format_len::Long;
            case date_full: return format_len::Full;
            default: return format_len::Medium;
        }
    }
    icu::DateFormat::EStyle time_flags_to_icu_len(const uint64_t time_flags)
    {
        switch(time_flags) {
            using namespace boost::locale::flags;
            case time_short: return icu::DateFormat::kShort;
            case time_medium: return icu::DateFormat::kMedium;
            case time_long: return icu::DateFormat::kLong;
            case time_full: return icu::DateFormat::kFull;
            case time_default:
            default: return icu::DateFormat::kDefault;
        }
    }
    icu::DateFormat::EStyle date_flags_to_icu_len(const uint64_t date_flags)
    {
        switch(date_flags) {
            using namespace boost::locale::flags;
            case date_short: return icu::DateFormat::kShort;
            case date_medium: return icu::DateFormat::kMedium;
            case date_long: return icu::DateFormat::kLong;
            case date_full: return icu::DateFormat::kFull;
            case date_default:
            default: return icu::DateFormat::kDefault;
        }
    }

    template<typename CharType>
    std::unique_ptr<formatter<CharType>>
    formatter<CharType>::create(std::ios_base& ios, const icu::Locale& locale, const std::string& encoding)
    {
        using ptr_type = std::unique_ptr<formatter<CharType>>;

        const ios_info& info = ios_info::get(ios);
        const formatters_cache& cache = std::use_facet<formatters_cache>(ios.getloc());

        const uint64_t disp = info.display_flags();
        switch(disp) {
            using namespace boost::locale::flags;
            case posix:
                BOOST_ASSERT_MSG(false, "Shouldn't try to create a posix formatter"); // LCOV_EXCL_LINE
                break;                                                                // LCOV_EXCL_LINE
            case number: {
                const std::ios_base::fmtflags how = (ios.flags() & std::ios_base::floatfield);
                icu::NumberFormat& nf =
                  cache.number_format((how == std::ios_base::scientific) ? num_fmt_type::sci : num_fmt_type::number);
                set_fraction_digits(nf, how, ios.precision());
                return ptr_type(new number_format<CharType>(nf, encoding, true));
            }
            case currency: {
                icu::NumberFormat& nf = cache.number_format(
                  (info.currency_flags() == currency_iso) ? num_fmt_type::curr_iso : num_fmt_type::curr_nat);
                return ptr_type(new number_format<CharType>(nf, encoding));
            }
            case percent: {
                icu::NumberFormat& nf = cache.number_format(num_fmt_type::percent);
                set_fraction_digits(nf, ios.flags() & std::ios_base::floatfield, ios.precision());
                return ptr_type(new number_format<CharType>(nf, encoding));
            }
            case spellout:
                return ptr_type(new number_format<CharType>(cache.number_format(num_fmt_type::spell), encoding));
            case ordinal:
                return ptr_type(new number_format<CharType>(cache.number_format(num_fmt_type::ordinal), encoding));
            case date:
            case time:
            case datetime:
            case strftime: {
                using namespace flags;
                std::unique_ptr<icu::DateFormat> new_df;
                icu::DateFormat* df = nullptr;
                // try to use cached first
                {
                    icu::SimpleDateFormat* sdf = cache.date_formatter();
                    if(sdf) {
                        icu::UnicodeString pattern;
                        switch(disp) {
                            case date: pattern = cache.date_format(date_flags_to_len(info.date_flags())); break;
                            case time: pattern = cache.time_format(time_flags_to_len(info.time_flags())); break;
                            case datetime:
                                pattern = cache.date_time_format(date_flags_to_len(info.date_flags()),
                                                                 time_flags_to_len(info.time_flags()));
                                break;
                            case strftime: {
                                icu_std_converter<CharType> cvt_(encoding);
                                const std::basic_string<CharType> f = info.date_time_pattern<CharType>();
                                pattern = strftime_to_icu(cvt_.icu(f.c_str(), f.c_str() + f.size()), locale);
                            } break;
                        }
                        if(!pattern.isEmpty()) {
                            sdf->applyPattern(pattern);
                            df = sdf;
                        }
                    }
                }

                if(!df) {
                    switch(disp) {
                        case date:
                            new_df.reset(
                              icu::DateFormat::createDateInstance(date_flags_to_icu_len(info.date_flags()), locale));
                            break;
                        case time:
                            new_df.reset(
                              icu::DateFormat::createTimeInstance(time_flags_to_icu_len(info.time_flags()), locale));
                            break;
                        case datetime:
                            new_df.reset(
                              icu::DateFormat::createDateTimeInstance(date_flags_to_icu_len(info.date_flags()),
                                                                      time_flags_to_icu_len(info.time_flags()),
                                                                      locale));
                            break;
                        case strftime: {
                            icu_std_converter<CharType> cvt_(encoding);
                            const std::basic_string<CharType> f = info.date_time_pattern<CharType>();
                            icu::UnicodeString pattern =
                              strftime_to_icu(cvt_.icu(f.data(), f.data() + f.size()), locale);
                            UErrorCode err = U_ZERO_ERROR;
                            new_df.reset(new icu::SimpleDateFormat(pattern, locale, err));
                            if(U_FAILURE(err))
                                return nullptr;
                        } break;
                    }
                    df = new_df.get();
                    BOOST_ASSERT_MSG(df, "Failed to create date/time formatter");
                }

                df->adoptTimeZone(get_time_zone(info.time_zone()));

                // Depending if we own formatter or not
                if(new_df)
                    return ptr_type(new date_format<CharType>(std::move(new_df), encoding));
                else
                    return ptr_type(new date_format<CharType>(*df, encoding));
            } break;
        }

        return nullptr; // LCOV_EXCL_LINE
    }

#define BOOST_LOCALE_INSTANTIATE(CHAR) template class formatter<CHAR>;
    BOOST_LOCALE_FOREACH_CHAR_STRING(BOOST_LOCALE_INSTANTIATE)

}}} // namespace boost::locale::impl_icu

// boostinspect:nominmax
