//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "boost/locale/util/locale_data.hpp"
#include "boost/locale/encoding/conv.hpp"
#include <string>

namespace boost { namespace locale { namespace util {
    void locale_data::parse(const std::string& locale_name)
    {
        language_ = "C";
        country_.clear();
        variant_.clear();
        encoding_ = "us-ascii";
        utf8_ = false;
        parse_from_lang(locale_name);
    }

    void locale_data::parse_from_lang(const std::string& locale_name)
    {
        size_t end = locale_name.find_first_of("-_@.");
        std::string tmp = locale_name.substr(0, end);
        if(tmp.empty())
            return;
        for(unsigned i = 0; i < tmp.size(); i++) {
            if('A' <= tmp[i] && tmp[i] <= 'Z')
                tmp[i] = tmp[i] - 'A' + 'a';
            else if(tmp[i] < 'a' || 'z' < tmp[i])
                return;
        }
        language_ = tmp;
        if(end >= locale_name.size())
            return;

        if(locale_name[end] == '-' || locale_name[end] == '_') {
            parse_from_country(locale_name.substr(end + 1));
        } else if(locale_name[end] == '.') {
            parse_from_encoding(locale_name.substr(end + 1));
        } else if(locale_name[end] == '@') {
            parse_from_variant(locale_name.substr(end + 1));
        }
    }

    void locale_data::parse_from_country(const std::string& locale_name)
    {
        size_t end = locale_name.find_first_of("@.");
        std::string tmp = locale_name.substr(0, end);
        if(tmp.empty())
            return;
        for(unsigned i = 0; i < tmp.size(); i++) {
            if('a' <= tmp[i] && tmp[i] <= 'z')
                tmp[i] = tmp[i] - 'a' + 'A';
            else if(tmp[i] < 'A' || 'Z' < tmp[i])
                return;
        }

        country_ = tmp;

        if(end >= locale_name.size())
            return;
        else if(locale_name[end] == '.') {
            parse_from_encoding(locale_name.substr(end + 1));
        } else if(locale_name[end] == '@') {
            parse_from_variant(locale_name.substr(end + 1));
        }
    }

    void locale_data::parse_from_encoding(const std::string& locale_name)
    {
        size_t end = locale_name.find_first_of('@');
        std::string tmp = locale_name.substr(0, end);
        if(tmp.empty())
            return;
        for(unsigned i = 0; i < tmp.size(); i++) {
            if('A' <= tmp[i] && tmp[i] <= 'Z')
                tmp[i] = tmp[i] - 'A' + 'a';
        }
        encoding_ = tmp;

        utf8_ = conv::impl::normalize_encoding(encoding_.c_str()) == "utf8";

        if(end >= locale_name.size())
            return;

        if(locale_name[end] == '@') {
            parse_from_variant(locale_name.substr(end + 1));
        }
    }

    void locale_data::parse_from_variant(const std::string& locale_name)
    {
        variant_ = locale_name;
        for(unsigned i = 0; i < variant_.size(); i++) {
            if('A' <= variant_[i] && variant_[i] <= 'Z')
                variant_[i] = variant_[i] - 'A' + 'a';
        }
    }

}}} // namespace boost::locale::util
