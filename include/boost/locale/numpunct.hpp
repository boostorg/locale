//
// Copyright (c) 2021-2021 Salvo Miosi
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

    template<typename CharType>
    class numpunct_base : public std::numpunct<CharType> {
        typedef std::basic_string<CharType> string_type;

    public:
        numpunct_base(size_t refs = 0) : std::numpunct<CharType>(refs) {}

        string_type decimal_point_str() const { return do_decimal_point_str(); }

        string_type thousands_sep_str() const { return do_thousands_sep_str(); }

    protected:
        CharType do_decimal_point() const BOOST_OVERRIDE
        {
            string_type dec = do_decimal_point_str();
            if(dec.size() > 1) {
                return '.';
            } else {
                return dec[0];
            }
        }

        virtual string_type do_decimal_point_str() const
        {
            static const char t[] = ".";
            return string_type(t, t + sizeof(t) - 1);
        }

        CharType do_thousands_sep() const BOOST_OVERRIDE
        {
            string_type thous = do_thousands_sep_str();
            if(thous.size() > 1) {
                return ',';
            } else {
                return thous[0];
            }
        }

        virtual string_type do_thousands_sep_str() const
        {
            static const char t[] = ",";
            return string_type(t, t + sizeof(t) - 1);
        }

        virtual string_type do_truename() const BOOST_OVERRIDE
        {
            static const char t[] = "true";
            return string_type(t, t + sizeof(t) - 1);
        }

        virtual string_type do_falsename() const BOOST_OVERRIDE
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