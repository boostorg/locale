//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
// Copyright (c) 2022-2023 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/encoding.hpp>

#include "boost/locale/encoding/conv.hpp"
#if BOOST_LOCALE_USE_WIN32_API
#    define BOOST_LOCALE_WITH_WCONV
#endif
#ifdef BOOST_LOCALE_WITH_ICONV
#    include "boost/locale/encoding/iconv_codepage.ipp"
#endif
#ifdef BOOST_LOCALE_WITH_ICU
#    include "boost/locale/encoding/uconv_codepage.ipp"
#endif
#ifdef BOOST_LOCALE_WITH_WCONV
#    include "boost/locale/encoding/wconv_codepage.ipp"
#endif

namespace boost { namespace locale { namespace conv {

    std::string between(const char* begin,
                        const char* end,
                        const std::string& to_charset,
                        const std::string& from_charset,
                        method_type how)
    {
#ifdef BOOST_LOCALE_WITH_ICONV
        {
            impl::iconv_between cvt;
            if(cvt.open(to_charset, from_charset, how))
                return cvt.convert(begin, end);
        }
#endif
#ifdef BOOST_LOCALE_WITH_ICU
        {
            impl::uconv_between cvt;
            if(cvt.open(to_charset, from_charset, how))
                return cvt.convert(begin, end);
        }
#endif
#ifdef BOOST_LOCALE_WITH_WCONV
        {
            impl::wconv_between cvt;
            if(cvt.open(to_charset, from_charset, how))
                return cvt.convert(begin, end);
        }
#endif
        throw invalid_charset_error(std::string(to_charset) + " or " + from_charset);
    }

    template<typename CharType>
    std::basic_string<CharType> to_utf(const char* begin, const char* end, const std::string& charset, method_type how)
    {
#ifdef BOOST_LOCALE_WITH_ICONV
        {
            impl::iconv_to_utf<CharType> cvt;
            if(cvt.open(charset, how))
                return cvt.convert(begin, end);
        }
#endif
#ifdef BOOST_LOCALE_WITH_ICU
        {
            impl::uconv_to_utf<CharType> cvt;
            if(cvt.open(charset, how))
                return cvt.convert(begin, end);
        }
#endif
#ifdef BOOST_LOCALE_WITH_WCONV
        {
            impl::wconv_to_utf<CharType> cvt;
            if(cvt.open(charset, how))
                return cvt.convert(begin, end);
        }
#endif
        throw invalid_charset_error(charset);
    }

    template<typename CharType>
    std::string from_utf(const CharType* begin, const CharType* end, const std::string& charset, method_type how)
    {
#ifdef BOOST_LOCALE_WITH_ICONV
        {
            impl::iconv_from_utf<CharType> cvt;
            if(cvt.open(charset, how))
                return cvt.convert(begin, end);
        }
#endif
#ifdef BOOST_LOCALE_WITH_ICU
        {
            impl::uconv_from_utf<CharType> cvt;
            if(cvt.open(charset, how))
                return cvt.convert(begin, end);
        }
#endif
#ifdef BOOST_LOCALE_WITH_WCONV
        {
            impl::wconv_from_utf<CharType> cvt;
            if(cvt.open(charset, how))
                return cvt.convert(begin, end);
        }
#endif
        throw invalid_charset_error(charset);
    }

#define BOOST_LOCALE_INSTANTIATE(CHARTYPE)                                                              \
    template BOOST_LOCALE_DECL std::basic_string<CHARTYPE> to_utf<CHARTYPE>(const char* begin,          \
                                                                            const char* end,            \
                                                                            const std::string& charset, \
                                                                            method_type how);           \
    template BOOST_LOCALE_DECL std::string from_utf<CHARTYPE>(const CHARTYPE* begin,                    \
                                                              const CHARTYPE* end,                      \
                                                              const std::string& charset,               \
                                                              method_type how)

    BOOST_LOCALE_INSTANTIATE(char);
    BOOST_LOCALE_INSTANTIATE(wchar_t);

#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    BOOST_LOCALE_INSTANTIATE(char16_t);
#endif

#ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    BOOST_LOCALE_INSTANTIATE(char32_t);
#endif

}}} // namespace boost::locale::conv
