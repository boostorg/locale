//
// Copyright (c) 2023 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale.hpp>
#include <boost/locale/util/string.hpp>
#include "boostLocale/test/tools.hpp"
#include "boostLocale/test/unit_test.hpp"
#include <limits>
#include <map>
#include <sstream>
#include <string>

namespace bl = boost::locale;

template<typename Char>
void as_if_std_numpunct(const std::locale& l)
{
    const std::numpunct<Char>& std_facet = std::use_facet<std::numpunct<Char>>(l);
    const bl::numpunct<Char>& boost_facet = std::use_facet<bl::numpunct<Char>>(l);
    // All functions present in std::numpunct are also present in boost::locale::numpunct and yield the same results
    TEST_REQUIRE(dynamic_cast<const bl::numpunct<Char>*>(&std_facet)); // In fact they are equal
    TEST_EQ(std_facet.decimal_point(), boost_facet.decimal_point());
    TEST_EQ(std_facet.thousands_sep(), boost_facet.thousands_sep());
    TEST_EQ(std_facet.grouping(), boost_facet.grouping());
    TEST_EQ(std_facet.truename(), boost_facet.truename());
    TEST_EQ(std_facet.falsename(), boost_facet.falsename());
}

namespace {
template<typename Char = char>
struct Punctuation {
    std::basic_string<Char> decimal;
    std::basic_string<Char> thousand;
};

const std::map<std::string, Punctuation<>> expected_punctuations = {
  {"en", {".", ","}},
  {"de", {",", "."}},
  {"he", {".", ","}},
  {"ja", {".", ","}},
  {"ru", {",", "\xC2\xA0"}},
  {"it", {".", ","}},
};

template<typename Char, template<typename> class Res>
Res<Char> get_expected(const std::map<std::string, Res<char>>& from, const std::locale& l)
{
    const auto& src = from.at(std::use_facet<bl::info>(l).language());
    return {to_correct_string<Char>(src.decimal, l), to_correct_string<Char>(src.thousand, l)};
}

template<typename Char>
struct split_result {
    std::basic_string<Char> digits, others;
};

template<typename Char>
split_result<Char> split_number(const std::basic_string<Char>& s)
{
    split_result<Char> res;
    for(Char c : s) {
        if(c >= std::numeric_limits<char>::min() && c <= std::numeric_limits<char>::max()
           && boost::locale::util::is_numeric_ascii(static_cast<char>(c)))
            res.digits += c;
        else
            res.others += c;
    }
    return res;
}
} // namespace

template<typename Char>
void test_for_char(const std::locale& l)
{
    using string_type = std::basic_string<Char>;
    as_if_std_numpunct<Char>(l);
    {
        const auto& expected = get_expected<Char>(expected_punctuations, l);
        const auto& boost_facet = std::use_facet<bl::numpunct<Char>>(l);
        TEST_EQ(boost_facet.decimal_point_str(), expected.decimal);
        TEST_EQ(boost_facet.thousands_sep_str(), expected.thousand);
    }
    std::basic_ostringstream<Char> s;
    s.imbue(l);
    // Formatting not using the Boost.Locale modifiers uses (only) the std::numpunct values
    {
        const auto& facet = std::use_facet<std::numpunct<Char>>(l);
        empty_stream(s) << 1234567890;
        auto actual = split_number(s.str());
        TEST_EQ(actual.digits, ascii_to<Char>("1234567890"));
        TEST_EQ(actual.others, string_type(actual.others.size(), facet.thousands_sep()));

        empty_stream(s) << 12.25;
        actual = split_number(s.str());
        TEST_EQ(actual.digits, ascii_to<Char>("1225"));
        TEST_EQ(actual.others, string_type(actual.others.size(), facet.decimal_point()));
    }
    // Formatting using the Boost.Locale modifiers uses the boost::locale::numpunct values
    s << bl::as::number;
    {
        const auto& facet = std::use_facet<bl::numpunct<Char>>(l);
        empty_stream(s) << 1234567890;
        auto actual = split_number(s.str());
        TEST_EQ(actual.digits, ascii_to<Char>("1234567890"));
        TEST_EQ(actual.others, string_type(actual.others.size(), facet.thousands_sep()));

        empty_stream(s) << 12.25;
        actual = split_number(s.str());
        TEST_EQ(actual.digits, ascii_to<Char>("1225"));
        TEST_EQ(actual.others, string_type(actual.others.size(), facet.decimal_point()));
    }
}

void test_for_locale(const std::string& name)
{
    std::cout << "-- Locale: " << name << '\n';
    const std::locale l = bl::generator{}(name);
    std::cout << "---- char\n";
    test_for_char<char>(l);
    std::cout << "---- wchar_t\n";
    test_for_char<wchar_t>(l);
#ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    std::cout << "---- char16_t\n";
    test_for_char<char16_t>(l);
#endif
#ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    std::cout << "---- char32_t\n";
    test_for_char<char32_t>(l);
#endif
}

void test_main(int /*argc*/, char** /*argv*/)
{
    const bl::localization_backend_manager orig_backend = bl::localization_backend_manager::global();
    for(const std::string& backendName : orig_backend.get_all_backends()) {
        std::cout << "Backend: " << backendName << std::endl;
        bl::localization_backend_manager tmp_backend = bl::localization_backend_manager::global();
        tmp_backend.select(backendName);
        bl::localization_backend_manager::global(tmp_backend);
        test_for_locale("en_US.UTF-8");
        test_for_locale("de_DE.UTF-8");
        test_for_locale("he_IL.UTF-8");
        test_for_locale("ja_JP.UTF-8");
        test_for_locale("ru_RU.UTF-8");
        test_for_locale("it_IT");
    }
}
