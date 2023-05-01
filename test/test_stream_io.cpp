//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
// Copyright (c) 2022-2023 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/locale/generator.hpp>
#include <boost/locale/localization_backend.hpp>
#include <fstream>

#include "boostLocale/test/tools.hpp"
#include "boostLocale/test/unit_test.hpp"

bool test_iso;
const bool test_iso_8859_8 =
#if defined(BOOST_LOCALE_WITH_ICU) || defined(BOOST_LOCALE_WITH_ICONV)
  true;
#else
  hasWinCodepage(28598);
#endif
bool test_utf;
bool test_sjis;

std::string he_il_8bit;
std::string en_us_8bit;
std::string ja_jp_shiftjis;

template<typename Char>
std::basic_string<Char> read_file(std::basic_istream<Char>& in)
{
    std::basic_string<Char> res;
    Char c;
    while(in.get(c))
        res += c;
    return res;
}

template<typename Char>
void test_ok(const std::string& content, const std::locale& l, std::basic_string<Char> cmp = std::basic_string<Char>())
{
    typedef std::basic_fstream<Char> stream_type;
    if(cmp.empty())
        cmp = to<Char>(content);

    {
        const std::string file_path = boost::locale::test::exe_name + "-test_read.txt";
        remove_file_on_exit _(file_path);
        {
            std::ofstream out_file(file_path);
            out_file << content;
        }
        stream_type in_file(file_path, stream_type::in);
        in_file.imbue(l);
        TEST_EQ(read_file<Char>(in_file), cmp);
    }

    {
        const std::string file_path = boost::locale::test::exe_name + "-test_write.txt";
        remove_file_on_exit _(file_path);
        {
            stream_type out_file(file_path, stream_type::out);
            out_file.imbue(l);
            out_file << cmp;
        }
        std::ifstream in_file(file_path);
        TEST_EQ(read_file<char>(in_file), content);
    }
}

template<typename Char>
void test_read_fail(const std::string& content, const std::locale& l, int pos)
{
    const std::string file_path = boost::locale::test::exe_name + "-test.txt";
    remove_file_on_exit _(file_path);
    {
        std::ofstream f(file_path);
        f << content;
    }

    typedef std::basic_fstream<Char> stream_type;

    stream_type f(file_path, stream_type::in);
    f.imbue(l);
    // Read up until the position
    for(int i = 0; i < pos; i++) {
        Char c;
        f.get(c);
        if(f.fail()) // failed before the position,
            return;  // e.g. when errors are detected reading more than the current char
        TEST(f);
    }
    // if the loop above succeeded, then it must fail now
    Char c;
    TEST(f.get(c).fail());
}

template<typename Char>
void test_write_fail(const std::string& content, const std::locale& l, int pos)
{
    const std::string file_path = boost::locale::test::exe_name + "-test.txt";
    remove_file_on_exit _(file_path);

    typedef std::basic_fstream<Char> stream_type;
    stream_type f(file_path, stream_type::out);
    f.imbue(l);
    std::basic_string<Char> out = to<Char>(content);
    for(int i = 0; i < pos; i++) {
        f << out.at(i) << std::flush;
        TEST(f);
    }
    f << out.at(pos);
    TEST(f.fail() || (f << std::flush).fail());
}

template<typename Char>
void test_for_char()
{
    boost::locale::generator g;
    if(test_utf) {
        std::cout << "    UTF-8" << std::endl;
        test_ok<Char>("grüße\nn i", g("en_US.UTF-8"));
        test_read_fail<Char>("abc\xFF\xFF", g("en_US.UTF-8"), 3);
        std::cout << "    Testing codepoints above 0xFFFF" << std::endl;
        std::cout << "      Single U+2008A" << std::endl;
        test_ok<Char>("\xf0\xa0\x82\x8a", g("en_US.UTF-8")); // U+2008A
        std::cout << "      Single U+2008A within text" << std::endl;
        test_ok<Char>("abc\"\xf0\xa0\x82\x8a\"", g("en_US.UTF-8")); // U+2008A
        std::string one = "\xf0\xa0\x82\x8a";
        std::string res;
        for(unsigned i = 0; i < 1000; i++)
            res += one;
        std::cout << "      U+2008A x 1000" << std::endl;
        test_ok<Char>(res.c_str(), g("en_US.UTF-8")); // U+2008A
    } else {
        std::cout << "    UTF-8 Not supported \n";
    }

    if(test_iso) {
        if(test_iso_8859_8) {
            std::cout << "    ISO8859-8" << std::endl;
            test_ok<Char>("hello \xf9\xec\xe5\xed", g(he_il_8bit), to<Char>("hello שלום"));
        }
        std::cout << "    ISO8859-1" << std::endl;
        test_ok<Char>(to<char>("grüße\nn i"), g(en_us_8bit), to<Char>("grüße\nn i"));
        test_write_fail<Char>("grüßen שלום", g(en_us_8bit), 7);
    }

    if(test_sjis) {
        std::cout << "    Shift-JIS" << std::endl;
        test_ok<Char>("\x93\xfa\x96\x7b",
                      g(ja_jp_shiftjis),
                      boost::locale::conv::to_utf<Char>("\xe6\x97\xa5\xe6\x9c\xac", "UTF-8")); // Japan
    }
}
void test_wide_io()
{
    std::cout << "  wchar_t" << std::endl;
    test_for_char<wchar_t>();

#if defined BOOST_LOCALE_ENABLE_CHAR16_T
    std::cout << "  char16_t" << std::endl;
    test_for_char<char16_t>();
#endif
#if defined BOOST_LOCALE_ENABLE_CHAR32_T
    std::cout << "  char32_t" << std::endl;
    test_for_char<char32_t>();
#endif
}

void test_main(int /*argc*/, char** /*argv*/)
{
    for(const std::string& backendName : boost::locale::localization_backend_manager::global().get_all_backends()) {
        boost::locale::localization_backend_manager tmp_backend = boost::locale::localization_backend_manager::global();
        tmp_backend.select(backendName);
        boost::locale::localization_backend_manager::global(tmp_backend);

        if(backendName == "std") {
            en_us_8bit = get_std_name("en_US.ISO8859-1");
            he_il_8bit = get_std_name("he_IL.ISO8859-8");
            ja_jp_shiftjis = get_std_name("ja_JP.SJIS");
            if(!ja_jp_shiftjis.empty() && !test_std_supports_SJIS_codecvt(ja_jp_shiftjis)) {
                std::cout << "Warning: detected unproper support of " << ja_jp_shiftjis << " locale, disabling it"
                          << std::endl;
                ja_jp_shiftjis = "";
            }
        } else {
            en_us_8bit = "en_US.ISO8859-1";
            he_il_8bit = "he_IL.ISO8859-8";
            ja_jp_shiftjis = "ja_JP.SJIS";
        }

        std::cout << "Testing for backend " << backendName << std::endl;

        test_iso = true;
        if(backendName == "std" && (he_il_8bit.empty() || en_us_8bit.empty())) {
            std::cout << "no ISO locales available, passing" << std::endl;
            test_iso = false;
        }
        test_sjis = true;
        if(backendName == "std" && ja_jp_shiftjis.empty()) {
            test_sjis = false;
        }
        if(backendName == "winapi") {
            test_iso = false;
            test_sjis = false;
        }
        test_utf = true;
#ifndef BOOST_LOCALE_NO_POSIX_BACKEND
        if(backendName == "posix") {
            if(!has_posix_locale(he_il_8bit))
                test_iso = false;
            if(!has_posix_locale(en_us_8bit))
                test_iso = false;
            if(!has_posix_locale("en_US.UTF-8"))
                test_utf = false;
#    ifdef BOOST_LOCALE_WITH_ICONV
            if(!has_posix_locale(ja_jp_shiftjis))
                test_sjis = false;
#    else
            test_sjis = false;
#    endif
        }
#endif

        if(backendName == "std" && (get_std_name("en_US.UTF-8").empty() || get_std_name("he_IL.UTF-8").empty())) {
            test_utf = false;
        }

        test_wide_io();
    }
}

// boostinspect:noascii
