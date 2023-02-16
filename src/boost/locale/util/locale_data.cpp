//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "boost/locale/util/locale_data.hpp"
#include "boost/locale/encoding/conv.hpp"
#include <string>

static bool is_upper_ascii(const char c)
{
    return 'A' <= c && c <= 'Z';
}

static bool is_lower_ascii(const char c)
{
    return 'a' <= c && c <= 'z';
}

namespace boost { namespace locale { namespace util {
    locale_data::locale_data()
    {
        reset();
    }

    void locale_data::reset()
    {
        language_ = "C";
        country_.clear();
        encoding_ = "US-ASCII";
        variant_.clear();
        utf8_ = false;
    }

    void locale_data::parse(const std::string& locale_name)
    {
        reset();
        parse_from_lang(locale_name);
    }

    void locale_data::parse_from_lang(const std::string& input)
    {
        const auto end = input.find_first_of("-_@.");
        std::string tmp = input.substr(0, end);
        if(tmp.empty())
            return;
        // lowercase ASCII
        for(char& c : tmp) {
            if(is_upper_ascii(c))
                c += 'a' - 'A';
            else if(!is_lower_ascii(c))
                return;
        }
        if(tmp == "c" || tmp == "posix")
            return;
        language_ = tmp;
        if(end >= input.size())
            return;

        if(input[end] == '-' || input[end] == '_') {
            parse_from_country(input.substr(end + 1));
        } else if(input[end] == '.') {
            parse_from_encoding(input.substr(end + 1));
        } else if(input[end] == '@') {
            parse_from_variant(input.substr(end + 1));
        }
    }

    void locale_data::parse_from_country(const std::string& input)
    {
        const auto end = input.find_first_of("@.");
        std::string tmp = input.substr(0, end);
        if(tmp.empty())
            return;
        // uppercase ASCII
        for(char& c : tmp) {
            if(is_lower_ascii(c))
                c += 'A' - 'a';
            else if(!is_upper_ascii(c))
                return;
        }

        country_ = tmp;
        if(end >= input.size())
            return;

        if(input[end] == '.') {
            parse_from_encoding(input.substr(end + 1));
        } else if(input[end] == '@') {
            parse_from_variant(input.substr(end + 1));
        }
    }

    void locale_data::parse_from_encoding(const std::string& input)
    {
        const auto end = input.find_first_of('@');
        std::string tmp = input.substr(0, end);
        if(tmp.empty())
            return;
        // No assumptions
        encoding_ = tmp;

        utf8_ = conv::impl::normalize_encoding(encoding_.c_str()) == "utf8";

        if(end >= input.size())
            return;

        if(input[end] == '@') {
            parse_from_variant(input.substr(end + 1));
        }
    }

    void locale_data::parse_from_variant(const std::string& input)
    {
        variant_ = input;
        // No assumptions, just make it lowercase
        for(char& c : variant_) {
            if(is_upper_ascii(c))
                c += 'a' - 'A';
        }
    }

}}} // namespace boost::locale::util
