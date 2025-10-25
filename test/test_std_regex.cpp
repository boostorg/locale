//
// Copyright (c) 2025 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/generator.hpp>
#include <boost/locale/localization_backend.hpp>
#include "boostLocale/test/tools.hpp"
#include <boost/core/detail/string_view.hpp>
#include <regex>

/// Smoke test that std::regex works with generated locales
template<typename CharType>
void test_by_char_impl(const std::locale& l)
{
    using string_type = std::basic_string<CharType>;

    // Needs at least a '\s' and '=' to reproduce issue #249
    const string_type s_pattern = ascii_to<CharType>(R"([[:alnum:]]+\s*= \d+)");
    const string_type text = ascii_to<CharType>("a2b2 = 42");
    std::match_results<typename string_type::const_iterator> pieces;

    // Sanity check using default locale
    std::basic_regex<CharType> pattern{s_pattern};
    if TEST(std::regex_match(text, pieces, pattern)) {
        TEST_EQ(pieces.size(), 1u);
        TEST_EQ(pieces[0].str(), text);

        pattern.imbue(l);
        pattern = s_pattern;
        if TEST(std::regex_match(text, pieces, pattern)) {
            TEST_EQ(pieces.size(), 1u);
            TEST_EQ(pieces[0].str(), text);
        }

        // Set via global locale
        const std::locale oldLoc = std::locale::global(l);
        std::basic_regex<CharType> globalPattern{s_pattern};
        if TEST(std::regex_match(text, pieces, globalPattern)) {
            TEST_EQ(pieces.size(), 1u);
            TEST_EQ(pieces[0].str(), text);
        }
        std::locale::global(oldLoc);
    }
}

template<typename CharType>
void test_by_char(const std::locale& loc, const std::locale& loc_collation, const std::locale& loc_no_collation)
{
    test_by_char_impl<CharType>(loc);
    {
        TEST_CONTEXT("without collation");
        test_by_char_impl<CharType>(loc_no_collation);
    }
    {
        TEST_CONTEXT("just collation");
        test_by_char_impl<CharType>(loc_collation);
    }
}

void test_main(int /*argc*/, char** /*argv*/)
{
    for(const std::string& backend_name : boost::locale::localization_backend_manager::global().get_all_backends()) {
        TEST_CONTEXT("Backend: " << backend_name);
        boost::locale::localization_backend_manager tmp_backend = boost::locale::localization_backend_manager::global();
        tmp_backend.select(backend_name);
        boost::locale::localization_backend_manager::global(tmp_backend);

        boost::locale::generator gen;
        const std::locale loc = gen("en_US.UTF-8");
        gen.categories(gen.categories() ^ boost::locale::category_t::collation);
        const std::locale loc_no_collation = gen("en_US.UTF-8");
        gen.categories(boost::locale::category_t::collation);
        const std::locale loc_collation = gen("en_US.UTF-8");
        {
            TEST_CONTEXT("char");
            test_by_char<char>(loc, loc_collation, loc_no_collation);
        }
        {
            TEST_CONTEXT("wchar_t");
            test_by_char<wchar_t>(loc, loc_collation, loc_no_collation);
        }
    }
}
