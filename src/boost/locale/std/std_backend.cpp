//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "boost/locale/std/std_backend.hpp"
#include <boost/locale/gnu_gettext.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/locale/util.hpp>
#include <boost/locale/util/locale_data.hpp>
#include <algorithm>
#include <iterator>
#include <vector>

#if BOOST_LOCALE_USE_WIN32_API
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include "boost/locale/util/encoding.hpp"
#    include "boost/locale/win32/lcid.hpp"
#    include <windows.h>
#endif
#include "boost/locale/std/all_generator.hpp"
#include "boost/locale/util/gregorian.hpp"
#include "boost/locale/util/numeric.hpp"

namespace {
#if BOOST_LOCALE_USE_WIN32_API
struct windows_name {
    windows_name() : name("C"), codepage("0") {}
    std::string name, codepage;
};

windows_name to_windows_name(const std::string& l)
{
    windows_name res;
    const unsigned lcid = boost::locale::impl_win::locale_to_lcid(l);
    char win_lang[256]{};
    if(lcid == 0 || GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE, win_lang, sizeof(win_lang)) == 0)
        return res;
    res.name = win_lang;
    char win_country[256]{};
    if(GetLocaleInfoA(lcid, LOCALE_SENGCOUNTRY, win_country, sizeof(win_country)) != 0) {
        res.name += "_";
        res.name += win_country;
    }

    char win_codepage[10]{};
    if(GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, win_codepage, sizeof(win_codepage)) != 0)
        res.codepage = win_codepage;
    return res;
}
#endif

bool loadable(const std::string& name)
{
    try {
        std::locale l(name);
        return true;
    } catch(const std::exception&) {
        return false;
    }
}
} // namespace

namespace boost { namespace locale { namespace impl_std {

    class std_localization_backend : public localization_backend {
    public:
        std_localization_backend() : invalid_(true), use_ansi_encoding_(false) {}
        std_localization_backend(const std_localization_backend& other) :
            localization_backend(), paths_(other.paths_), domains_(other.domains_), locale_id_(other.locale_id_),
            invalid_(true), use_ansi_encoding_(other.use_ansi_encoding_)
        {}
        std_localization_backend* clone() const override { return new std_localization_backend(*this); }

        void set_option(const std::string& name, const std::string& value) override
        {
            invalid_ = true;
            if(name == "locale")
                locale_id_ = value;
            else if(name == "message_path")
                paths_.push_back(value);
            else if(name == "message_application")
                domains_.push_back(value);
            else if(name == "use_ansi_encoding")
                use_ansi_encoding_ = value == "true";
        }
        void clear_options() override
        {
            invalid_ = true;
            use_ansi_encoding_ = false;
            locale_id_.clear();
            paths_.clear();
            domains_.clear();
        }

        void prepare_data()
        {
            if(!invalid_)
                return;
            invalid_ = false;
            std::string lid = locale_id_;
            if(lid.empty()) {
                bool use_utf8 = !use_ansi_encoding_;
                lid = util::get_system_locale(use_utf8);
            }
            in_use_id_ = lid;
            data_.parse(lid);

#if BOOST_LOCALE_USE_WIN32_API
            const auto l_win = to_windows_name(lid);
#endif

            if(!data_.is_utf8()) {
                utf_mode_ = utf8_support::none;
                if(loadable(lid))
                    name_ = lid;
#if BOOST_LOCALE_USE_WIN32_API
                else if(loadable(l_win.name)) {
                    if(util::are_encodings_equal(l_win.codepage, data_.encoding()))
                        name_ = l_win.name;
                    else {
                        int codepage_int;
                        if(util::try_to_int(l_win.codepage, codepage_int)
                           && codepage_int == util::encoding_to_windows_codepage(data_.encoding()))
                        {
                            name_ = l_win.name;
                        } else
                            name_ = "C";
                    }
                }
#endif
                else
                    name_ = "C";
            } else {
                if(loadable(lid)) {
                    name_ = lid;
                    utf_mode_ = utf8_support::native_with_wide;
#if defined(BOOST_WINDOWS)
                    // This isn't fully correct:
                    // It will treat the 2-Byte wchar_t as UTF-16 encoded while it may be UCS-2
                    // std::basic_filebuf explicitely disallows using suche multi-byte codecvts
                    // but it works in practice so far, so use it instead of failing for codepoints above U+FFFF
                    utf_mode_ = utf8_support::from_wide;
#endif
                }
#if BOOST_LOCALE_USE_WIN32_API
                else if(loadable(l_win.name))
                {
                    name_ = l_win.name;
                    utf_mode_ = utf8_support::from_wide;
                }
#endif
                else
                {
                    const std::string non_utf8 = util::locale_data(data_).encoding("").to_string();
                    name_ = loadable(non_utf8) ? non_utf8 : "C";
                    utf_mode_ = utf8_support::from_wide;
                }
            }
        }

        std::locale install(const std::locale& base, category_t category, char_facet_t type) override
        {
            prepare_data();

            switch(category) {
                case category_t::convert: return create_convert(base, name_, type, utf_mode_);
                case category_t::collation: return create_collate(base, name_, type, utf_mode_);
                case category_t::formatting: return create_formatting(base, name_, type, utf_mode_);
                case category_t::parsing: return create_parsing(base, name_, type, utf_mode_);
                case category_t::codepage: return create_codecvt(base, name_, type, utf_mode_);
                case category_t::calendar: return util::install_gregorian_calendar(base, data_.country());
                case category_t::message: {
                    gnu_gettext::messages_info minf;
                    minf.language = data_.language();
                    minf.country = data_.country();
                    minf.variant = data_.variant();
                    minf.encoding = data_.encoding();
                    std::copy(domains_.begin(),
                              domains_.end(),
                              std::back_inserter<gnu_gettext::messages_info::domains_type>(minf.domains));
                    minf.paths = paths_;
                    switch(type) {
                        case char_facet_t::nochar: break;
                        case char_facet_t::char_f:
                            return std::locale(base, gnu_gettext::create_messages_facet<char>(minf));
                        case char_facet_t::wchar_f:
                            return std::locale(base, gnu_gettext::create_messages_facet<wchar_t>(minf));
#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
                        case char_facet_t::char16_f:
                            return std::locale(base, gnu_gettext::create_messages_facet<char16_t>(minf));
#endif
#ifdef BOOST_LOCALE_ENABLE_CHAR32_T
                        case char_facet_t::char32_f:
                            return std::locale(base, gnu_gettext::create_messages_facet<char32_t>(minf));
#endif
                    }
                    return base;
                }
                case category_t::information: return util::create_info(base, in_use_id_);
                case category_t::boundary: break; // Not implemented
            }
            return base;
        }

    private:
        std::vector<std::string> paths_;
        std::vector<std::string> domains_;
        std::string locale_id_;

        util::locale_data data_;
        std::string name_;
        std::string in_use_id_;
        utf8_support utf_mode_;
        bool invalid_;
        bool use_ansi_encoding_;
    };

    localization_backend* create_localization_backend()
    {
        return new std_localization_backend();
    }

}}} // namespace boost::locale::impl_std
