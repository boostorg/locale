//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/encoding.hpp>
#include "boost/locale/std/all_generator.hpp"
#include <ios>
#include <locale>
#include <string>

namespace boost { namespace locale { namespace impl_std {

    class utf8_collator_from_wide : public std::collate<char> {
    public:
        typedef std::collate<wchar_t> wfacet;
        utf8_collator_from_wide(const std::string& locale_name) :
            base_(std::locale::classic(), new std::collate_byname<wchar_t>(locale_name))
        {}
        int do_compare(const char* lb, const char* le, const char* rb, const char* re) const override
        {
            const std::wstring l = conv::utf_to_utf<wchar_t>(lb, le);
            const std::wstring r = conv::utf_to_utf<wchar_t>(rb, re);
            return std::use_facet<wfacet>(base_).compare(l.c_str(),
                                                         l.c_str() + l.size(),
                                                         r.c_str(),
                                                         r.c_str() + r.size());
        }
        long do_hash(const char* b, const char* e) const override
        {
            const std::wstring tmp = conv::utf_to_utf<wchar_t>(b, e);
            return std::use_facet<wfacet>(base_).hash(tmp.c_str(), tmp.c_str() + tmp.size());
        }
        std::string do_transform(const char* b, const char* e) const override
        {
            const std::wstring tmp = conv::utf_to_utf<wchar_t>(b, e);
            const std::wstring wkey = std::use_facet<wfacet>(base_).transform(tmp.c_str(), tmp.c_str() + tmp.size());
            return conv::utf_to_utf<char>(wkey);
        }

    private:
        std::locale base_;
    };

    std::locale
    create_collate(const std::locale& in, const std::string& locale_name, char_facet_t type, utf8_support utf)
    {
        switch(type) {
            case char_facet_t::nochar: break;
            case char_facet_t::char_f:
                if(utf == utf8_support::from_wide)
                    return std::locale(in, new utf8_collator_from_wide(locale_name));
                else
                    return std::locale(in, new std::collate_byname<char>(locale_name));

            case char_facet_t::wchar_f: return std::locale(in, new std::collate_byname<wchar_t>(locale_name));

#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
            case char_facet_t::char16_f: return std::locale(in, new std::collate_byname<char16_t>(locale_name));
#endif

#ifdef BOOST_LOCALE_ENABLE_CHAR32_T
            case char_facet_t::char32_f: return std::locale(in, new std::collate_byname<char32_t>(locale_name));
#endif
        }
        return in;
    }

}}} // namespace boost::locale::impl_std
