//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_LOCALE_UNIT_TEST_HPP
#define BOOST_LOCALE_UNIT_TEST_HPP

#include <boost/locale/config.hpp>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <string>

int error_counter=0;
int test_counter=0;

#ifndef BOOST_LOCALE_ERROR_LIMIT
#define BOOST_LOCALE_ERROR_LIMIT 20
#endif


#define THROW_IF_TOO_BIG(X)                                                     \
    if((X) > BOOST_LOCALE_ERROR_LIMIT)                                          \
        throw std::runtime_error("Error limits reached, stopping unit test")

#define TEST(X)                                                           \
    do {                                                                  \
        test_counter++;                                                   \
        if(X) break;                                                      \
        std::cerr << "Error in line:" << __LINE__ << " "#X  << std::endl; \
        THROW_IF_TOO_BIG(error_counter++);                                \
        BOOST_LOCALE_START_CONST_CONDITION                                \
    }while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_REQUIRE(X)                                                    \
    do {                                                                   \
        test_counter++;                                                    \
        if(X) break;                                                       \
        std::cerr << "Error in line " << __LINE__ << ": "#X  << std::endl; \
        throw std::runtime_error("Critical test " #X " failed");           \
        BOOST_LOCALE_START_CONST_CONDITION                                 \
    }while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_THROWS(X,E)                                                   \
    do {                                                                   \
        test_counter++;                                                    \
        try { X; } catch(E const &/*e*/ ) {break;} catch(...){}            \
        std::cerr << "Error in line " << __LINE__ << ": "#X  << std::endl; \
        THROW_IF_TOO_BIG(error_counter++);                                 \
        BOOST_LOCALE_START_CONST_CONDITION                                 \
    }while(0) BOOST_LOCALE_END_CONST_CONDITION

void test_main(int argc, char **argv);

int main(int argc,char **argv) {
    try {
        test_main(argc, argv);
    } catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl; // LCOV_EXCL_LINE
        return EXIT_FAILURE;                             // LCOV_EXCL_LINE
    }
    int passed = test_counter - error_counter; 
    std::cout << std::endl;
    std::cout << "Passed " << passed << " tests\n";
    if(error_counter > 0 ) { 
        std::cout << "Failed " << error_counter << " tests\n"; // LCOV_EXCL_LINE
    }
    std::cout << " " << std::fixed << std::setprecision(1)
              << std::setw(5) << 100.0 * passed / test_counter <<
              "% of tests completed successfully\n";
    return error_counter == 0 ? EXIT_SUCCESS : EXIT_FAILURE ;
}

inline unsigned utf8_next(std::string const &s,unsigned &pos)
{
    unsigned c=(unsigned char)s[pos++];
    if( (unsigned char)(c - 0xc0) >= 0x35)
        return c;
    unsigned l;
    if(c < 192)
        l = 0;
    else if(c < 224)
        l = 1;
    else if(c < 240)
        l = 2;
    else
        l = 3;

    c &= (1 << (6-l)) - 1;

    switch(l) {
    case 3:
        c = (c << 6) | (((unsigned char)s[pos++]) & 0x3F);
        BOOST_FALLTHROUGH;
    case 2:
        c = (c << 6) | (((unsigned char)s[pos++]) & 0x3F);
        BOOST_FALLTHROUGH;
    case 1:
        c = (c << 6) | (((unsigned char)s[pos++]) & 0x3F);
    }
    return c;
}

template<typename Char>
std::basic_string<Char> to(std::string const &utf8)
{
    std::basic_string<Char> out;
    unsigned i=0;
    while(i<utf8.size()) {
        unsigned point;
        unsigned prev=i;
        point = utf8_next(utf8,i);
BOOST_LOCALE_START_CONST_CONDITION
        if(sizeof(Char)==1 && point > 255) {
            std::ostringstream ss;
            ss << "Can't convert codepoint U"
               << std::hex << point << "(" << std::string(utf8.begin()+prev,utf8.begin()+i)
               << ") to Latin1";
            throw std::runtime_error(ss.str());
        }
        else if(sizeof(Char)==2 && point >0xFFFF) { // Deal with surrogates
            point-=0x10000;
            out+=static_cast<Char>(0xD800 | (point>>10));
            out+=static_cast<Char>(0xDC00 | (point & 0x3FF));
            continue;
        }
BOOST_LOCALE_END_CONST_CONDITION
        out+=static_cast<Char>(point);
    }
    return out;
}

template<typename T>
std::string to_string(T const& s)
{
    std::stringstream ss;
    ss << s;
    return ss.str();
}

std::string const& to_string(std::string const& s)
{
    return s;
}

/// Put the char into the stream making sure it is readable
/// Fallback to the unicode representation of it (e.g. U+00A0)
template<typename Char>
void stream_char(std::ostream& s, const Char c)
{
    if((c >= '!' && c <= '~') || c == ' ')
        s << static_cast<char>(c);
    else
        s << "U+" << std::hex << std::uppercase << std::setw(sizeof(Char)) << static_cast<unsigned>(c);
}

template<typename Char>
std::string to_string(std::basic_string<Char> const& s)
{
    std::stringstream ss;
    for(size_t i = 0; i < s.size(); ++i)
        stream_char(ss, s[i]);
    return ss.str();
}

// Unicode chars cannot be streamed directly (deleted overloads in C++20)
template<typename Char>
std::string to_string_char_impl(const Char c)
{
    std::stringstream ss;
    stream_char(ss, c);
    return ss.str();
}

std::string to_string(const wchar_t c) { return to_string_char_impl(c); }
std::string to_string(const char16_t c) { return to_string_char_impl(c); }
std::string to_string(const char32_t c) { return to_string_char_impl(c); }

template<typename T, typename U>
void test_eq_impl(T const& l, U const& r, const char* expr, int line)
{
    test_counter++;
    if (l != r) {
        std::cerr << "Error in line " << line << ": " << expr << std::endl;
        std::cerr << "---- [" << to_string(l) << "] != [" << to_string(r) << "]" << std::endl;
        THROW_IF_TOO_BIG(error_counter++);
    }
}
#define TEST_EQ_IMPL(x, y, expr) test_eq_impl(x, y, #expr, __LINE__)
#define TEST_EQ(x,y) TEST_EQ_IMPL(x, y, x == y)

#endif
