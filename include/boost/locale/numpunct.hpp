//
// Copyright (c) 2021-2021 Salvo Miosi
// Copyright (c) 2023-2023 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_LOCALE_NUMPUNCT_HPP_INCLUDED
#define BOOST_LOCALE_NUMPUNCT_HPP_INCLUDED

#include <boost/locale/config.hpp>
#include <locale>
#include <string>

namespace boost { namespace locale {

    /// \brief Extension of `std::numpunct` providing possibly encoded values of decimal point and thousands separator.
    ///
    /// To achieve interface compatibility with `std::numpunct` for the case where the separators are encoded using
    /// multiple chars the functions `do_decimal_point` and `do_thousands_sep` will fall back to the values used by the
    /// "C" locale.
    ///
    /// \note
    ///
    /// - Not all backends support encoded separators, so \ref decimal_point_str & \ref thousands_sep_str may return
    ///   strings of length 1.
    /// - Some backends may provide single char replacements of the encoded separators instead of falling back to the
    ///   "C" locale.
    template<typename CharType>
    class numpunct_base : public std::numpunct<CharType> {
    public:
        using string_type = std::numpunct<CharType>::string_type;

        numpunct_base(size_t refs = 0) : std::numpunct<CharType>(refs) {}

        /// Provides the character to use as decimal point possibly encoded into multiple code units
        string_type decimal_point_str() const { return do_decimal_point_str(); }
        /// Provides the character to use as thousands separator possibly encoded into multiple code units
        string_type thousands_sep_str() const { return do_thousands_sep_str(); }

    protected:
        CharType do_decimal_point() const override
        {
            const string_type dec = do_decimal_point_str();
            return (dec.size() > 1) ? '.' : dec[0];
        }

        /// Provides the character to use as decimal point possibly encoded into multiple code units
        virtual string_type do_decimal_point_str() const
        {
            static const char t[] = ".";
            return string_type(t, t + sizeof(t) - 1);
        }

        CharType do_thousands_sep() const override
        {
            const string_type sep = do_thousands_sep_str();
            return (sep.size() > 1) ? '.' : sep[0];
        }

        /// Provides the character to use as thousands separator possibly encoded into multiple code units
        virtual string_type do_thousands_sep_str() const
        {
            static const char t[] = ",";
            return string_type(t, t + sizeof(t) - 1);
        }

        string_type do_truename() const override
        {
            static const char t[] = "true";
            return string_type(t, t + sizeof(t) - 1);
        }

        string_type do_falsename() const override
        {
            static const char t[] = "false";
            return string_type(t, t + sizeof(t) - 1);
        }
    };

    template<typename CharType>
    struct numpunct;

    template<>
    struct numpunct<char> : numpunct_base<char> {
        numpunct(size_t refs = 0) : numpunct_base<char>(refs) {}
    };

    template<>
    struct numpunct<wchar_t> : numpunct_base<wchar_t> {
        numpunct(size_t refs = 0) : numpunct_base<wchar_t>(refs) {}
    };

#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    template<>
    struct numpunct<char16_t> : numpunct_base<char16_t> {
        numpunct(size_t refs = 0) : numpunct_base<char16_t>(refs) {}
    };
#endif
#ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    template<>
    struct numpunct<char32_t> : numpunct_base<char32_t> {
        numpunct(size_t refs = 0) : numpunct_base<char32_t>(refs) {}
    };
#endif
}} // namespace boost::locale

#endif