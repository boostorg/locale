//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_LOCALE_WITH_ICU
#include <iostream>
int main()
{
        std::cout << "ICU is not build... Skipping\n";
}
#else

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <boost/locale/date_time.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/locale/format.hpp>
#include <boost/locale/formatting.hpp>
#include <boost/locale/generator.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <unicode/uversion.h>
#include <unicode/numfmt.h>

#include "../src/boost/locale/icu/time_zone.hpp"
#include "boostLocale/test/unit_test.hpp"
#include "boostLocale/test/tools.hpp"

#define BOOST_LOCALE_ICU_VERSION (U_ICU_VERSION_MAJOR_NUM * 100 + U_ICU_VERSION_MINOR_NUM)
#define BOOST_LOCALE_ICU_VERSION_EXACT (BOOST_LOCALE_ICU_VERSION  * 100 + U_ICU_VERSION_PATCHLEVEL_NUM)

const std::string test_locale_name = "en_US";

const icu::Locale& get_icu_test_locale()
{
    static icu::Locale locale = icu::Locale::createCanonical(test_locale_name.c_str());
    return locale;
}

std::string from_icu_string(const icu::UnicodeString& str)
{
    return boost::locale::conv::utf_to_utf<char>(str.getBuffer(), str.getBuffer() + str.length());
}

// Currency style changes between ICU versions, so get "real" value from ICU
#if BOOST_LOCALE_ICU_VERSION >= 402

std::string get_icu_currency_iso(const double value)
{
#if BOOST_LOCALE_ICU_VERSION >= 408
    auto styleIso = UNUM_CURRENCY_ISO;
#else
    auto styleIso = icu::NumberFormat::kIsoCurrencyStyle;
#endif
    UErrorCode err = U_ZERO_ERROR;
    std::unique_ptr<icu::NumberFormat> fmt(icu::NumberFormat::createInstance(get_icu_test_locale(), styleIso, err));
    TEST_REQUIRE(U_SUCCESS(err) && fmt.get());

    icu::UnicodeString tmp;
    return from_icu_string(fmt->format(value, tmp));
}

#endif

std::string get_icu_full_gmt_name()
{
    icu::UnicodeString tmp;
    return from_icu_string(icu::TimeZone::getGMT()->getDisplayName(get_icu_test_locale(), tmp));
}

// This changes between ICU versions, i.e. "GMT" or "Greenwich Mean Time"
const std::string icu_full_gmt_name = get_icu_full_gmt_name();

using namespace boost::locale;

template <typename CharType, typename T>
void test_fmt_impl(std::basic_ostringstream<CharType>& ss, const T& value, const std::basic_string<CharType>& expected, int line)
{
    ss << value;
    test_eq_impl(ss.str(), expected, "", line);
}

template <typename T, typename CharType>
void test_parse_impl(std::basic_istringstream<CharType>& ss, const T& expected, int line)
{
    T v;
    ss >> v >> std::ws;
    test_eq_impl(v, expected, "v == expected", line);
    test_eq_impl(ss.eof(), true, "ss.eof()", line);
}

template <typename T, typename CharType>
void test_parse_at_impl(std::basic_istringstream<CharType>& ss, const T& expected, int line)
{
    T v;
    CharType c_at;
    ss >> v >> std::skipws >> c_at;
    test_eq_impl(v, expected, "v == expected", line);
    test_eq_impl(c_at, '@', "c_at == @", line);
}

template <typename T, typename CharType>
void test_parse_fail_impl(std::basic_istringstream<CharType>& ss, int line)
{
    T v;
    ss >> v;
    test_eq_impl(ss.fail(), true, "ss.fail()", line);
}

#define TEST_FMT(manip,value,expected) \
do{ \
    std::basic_ostringstream<CharType> ss; \
    ss.imbue(loc); \
    ss << manip; \
    test_fmt_impl(ss, (value), to_correct_string<CharType>(expected,loc), __LINE__); \
BOOST_LOCALE_START_CONST_CONDITION                              \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_PARSE_FAILS(manip,actual,type)          \
do{                                                  \
    std::basic_istringstream<CharType> ss;           \
    ss.imbue(loc);                                   \
    ss.str(to_correct_string<CharType>(actual,loc)); \
    ss >> manip;                                     \
    test_parse_fail_impl<type>(ss, __LINE__);        \
BOOST_LOCALE_START_CONST_CONDITION                   \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_PARSE(manip,value,expected) \
do{ \
    const auto str_value = to_correct_string<CharType>(value,loc); \
    {std::basic_istringstream<CharType> ss; \
    ss.imbue(loc); \
    ss.str(str_value); \
    ss >> manip; \
    test_parse_impl(ss, expected, __LINE__); } \
    {std::basic_istringstream<CharType> ss; \
    ss.imbue(loc); \
    ss.str(str_value + CharType('@')); \
    ss >> manip; \
    test_parse_at_impl(ss, expected, __LINE__); } \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_1(manip,value_in,value_str) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  manip,value_in,value_str_); \
    TEST_PARSE(manip,value_str_,value_in); \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_2(m1,m2,value_in,value_str) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  m1 << m2,value_in,value_str_); \
    TEST_PARSE(m1 >> m2,value_str_,value_in);  \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_2_2(m1,m2,value_in,value_str,value_parsed) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  m1 << m2,value_in,value_str_); \
    TEST_PARSE(m1 >> m2,value_str_,value_parsed);  \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_3(m1,m2,m3,value_in,value_str) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  m1 << m2 << m3,value_in,value_str_); \
    TEST_PARSE(m1 >> m2 >> m3,value_str_,value_in); \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_3_2(m1,m2,m3,value_in,value_str,value_parsed) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  m1 << m2 << m3,value_in,value_str_); \
    TEST_PARSE(m1 >> m2 >> m3,value_str_,value_parsed); \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_4(m1,m2,m3,m4,value_in,value_str) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  m1 << m2 << m3 << m4,value_in,value_str_); \
    TEST_PARSE(m1 >> m2 >> m3 >> m4,value_str_,value_in); \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FMT_PARSE_4_2(m1,m2,m3,m4,value_in,value_str,value_parsed) \
do { \
    const std::string value_str_ = value_str; \
    TEST_FMT(  m1 << m2 << m3 << m4,value_in,value_str_); \
    TEST_PARSE(m1 >> m2 >> m3 >> m4,value_str_,value_parsed); \
BOOST_LOCALE_START_CONST_CONDITION                  \
}while(0) BOOST_LOCALE_END_CONST_CONDITION

#define TEST_FORMAT_CLS(fmt_string,fmt_expression,expected_str) \
    do{\
        std::basic_ostringstream<CharType> ss; \
        ss.imbue(loc);  \
        const std::basic_string<CharType> fmt = to_correct_string<CharType>(fmt_string,loc); \
        const std::basic_string<CharType> expected = to_correct_string<CharType>(expected_str,loc); \
        ss << boost::locale::basic_format<CharType>(fmt) % fmt_expression; \
        TEST_EQ(ss.str(), expected); \
        ss.str(std::basic_string<CharType>()); \
        ss << boost::locale::basic_format<CharType>(boost::locale::translate(fmt.c_str())) % fmt_expression; \
        TEST_EQ(ss.str(), expected); \
        TEST_EQ( (boost::locale::basic_format<CharType>(fmt) % fmt_expression).str(loc), expected); \
    BOOST_LOCALE_START_CONST_CONDITION                  \
    }while(0) BOOST_LOCALE_END_CONST_CONDITION


#define TEST_MIN_MAX_FMT(type,minval,maxval)    \
    TEST_FMT(as::number,std::numeric_limits<type>::min(),minval); \
    TEST_FMT(as::number,std::numeric_limits<type>::max(),maxval)

#define TEST_MIN_MAX_PARSE(type,minval,maxval)    \
    TEST_PARSE(as::number,minval,std::numeric_limits<type>::min()); \
    TEST_PARSE(as::number,maxval,std::numeric_limits<type>::max())

#define TEST_MIN_MAX(type,minval,maxval)    \
    TEST_MIN_MAX_FMT(type,minval,maxval); \
    TEST_MIN_MAX_PARSE(type,minval,maxval)


bool short_parsing_fails()
{
    static bool fails = false;
    static bool get_result = false;
    if(get_result)
        return fails;
    std::stringstream ss("65000");
    ss.imbue(std::locale::classic());
    short v=0;
    ss >> v;
    fails = ss.fail();
    get_result = true;
    return fails;
}

template<typename CharType>
void test_manip(std::string e_charset="UTF-8")
{
    boost::locale::generator g;
    std::locale loc=g(test_locale_name + "." + e_charset);

    TEST_FMT_PARSE_1(as::posix,1200.1,"1200.1");
    TEST_FMT_PARSE_1(as::number,1200.1,"1,200.1");
    TEST_FMT(as::number << std::setfill(CharType('_')) << std::setw(6),1534,"_1,534");
    TEST_FMT(as::number << std::left << std::setfill(CharType('_')) << std::setw(6),1534,"1,534_");

    // Ranges
BOOST_LOCALE_START_CONST_CONDITION
    if(sizeof(short) == 2) {
        TEST_MIN_MAX(short,"-32,768","32,767");
        TEST_MIN_MAX(unsigned short,"0","65,535");
        TEST_PARSE_FAILS(as::number,"-1",unsigned short);
        if(short_parsing_fails()) {
            TEST_PARSE_FAILS(as::number,"65,535",short);
        }
    }
    if(sizeof(int)==4) {
        TEST_MIN_MAX(int,"-2,147,483,648","2,147,483,647");
        TEST_MIN_MAX(unsigned int,"0","4,294,967,295");
        TEST_PARSE_FAILS(as::number,"-1",unsigned int);
        TEST_PARSE_FAILS(as::number,"4,294,967,295",int);
    }
    if(sizeof(long)==4) {
        TEST_MIN_MAX(long,"-2,147,483,648","2,147,483,647");
        TEST_MIN_MAX(unsigned long,"0","4,294,967,295");
        TEST_PARSE_FAILS(as::number,"-1",unsigned long);
        TEST_PARSE_FAILS(as::number,"4,294,967,295",long);
    }
    if(sizeof(long)==8) {
        TEST_MIN_MAX(long,"-9,223,372,036,854,775,808","9,223,372,036,854,775,807");
        TEST_MIN_MAX_FMT(unsigned long,"0","18446744073709551615"); // Unsupported range by icu - ensure fallback
        TEST_PARSE_FAILS(as::number,"-1",unsigned long);
    }
    if(sizeof(long long)==8) {
        TEST_MIN_MAX(long long,"-9,223,372,036,854,775,808","9,223,372,036,854,775,807");
        // we can't really parse this as ICU does not support this range, only format
        TEST_MIN_MAX_FMT(unsigned long long,"0","18446744073709551615"); // Unsupported range by icu - ensure fallback
        TEST_FMT(as::number,9223372036854775807ULL,"9,223,372,036,854,775,807");
        TEST_FMT(as::number,9223372036854775808ULL,"9223372036854775808"); // Unsupported range by icu - ensure fallback
        TEST_PARSE_FAILS(as::number,"-1",unsigned long long);
    }
BOOST_LOCALE_END_CONST_CONDITION



    TEST_FMT_PARSE_3(as::number,std::left,std::setw(3),15,"15 ");
    TEST_FMT_PARSE_3(as::number,std::right,std::setw(3),15," 15");
    TEST_FMT_PARSE_3(as::number,std::setprecision(3),std::fixed,13.1,"13.100");
    #if BOOST_LOCALE_ICU_VERSION < 5601
    // bug #13276
    TEST_FMT_PARSE_3(as::number,std::setprecision(3),std::scientific,13.1,"1.310E1");
    #endif

    TEST_PARSE_FAILS(as::number,"",int);
    TEST_PARSE_FAILS(as::number,"--3",int);
    TEST_PARSE_FAILS(as::number,"y",int);

    TEST_FMT_PARSE_1(as::percent,0.1,"10%");
    TEST_FMT_PARSE_3(as::percent,std::fixed,std::setprecision(1),0.10,"10.0%");

    TEST_PARSE_FAILS(as::percent,"1",double);

    TEST_FMT_PARSE_1(as::currency,1345,"$1,345.00");
    TEST_FMT_PARSE_1(as::currency,1345.34,"$1,345.34");

    TEST_PARSE_FAILS(as::currency,"$",double);


    #if BOOST_LOCALE_ICU_VERSION >= 402
    TEST_FMT_PARSE_2(as::currency,as::currency_national,1345,"$1,345.00");
    TEST_FMT_PARSE_2(as::currency,as::currency_national,1345.34,"$1,345.34");
    TEST_FMT_PARSE_2(as::currency,as::currency_iso,1345,get_icu_currency_iso(1345));
    TEST_FMT_PARSE_2(as::currency,as::currency_iso,1345.34,get_icu_currency_iso(1345.34));
    #endif
    TEST_FMT_PARSE_1(as::spellout,10,"ten");
    #if 402 <= BOOST_LOCALE_ICU_VERSION && BOOST_LOCALE_ICU_VERSION < 408
    if(e_charset=="UTF-8") {
        TEST_FMT(as::ordinal,1,"1\xcb\xa2\xe1\xb5\x97"); // 1st with st as ligatures
    }
    #else
        TEST_FMT(as::ordinal,1,"1st");
    #endif

    time_t a_date = 3600*24*(31+4); // Feb 5th
    time_t a_time = 3600*15+60*33; // 15:33:05
    time_t a_timesec = 13;
    time_t a_datetime = a_date + a_time + a_timesec;

    TEST_FMT_PARSE_2_2(as::date,                as::gmt,a_datetime,"Feb 5, 1970",a_date);
    TEST_FMT_PARSE_3_2(as::date,as::date_short ,as::gmt,a_datetime,"2/5/70",a_date);
    TEST_FMT_PARSE_3_2(as::date,as::date_medium,as::gmt,a_datetime,"Feb 5, 1970",a_date);
    TEST_FMT_PARSE_3_2(as::date,as::date_long  ,as::gmt,a_datetime,"February 5, 1970",a_date);
    TEST_FMT_PARSE_3_2(as::date,as::date_full  ,as::gmt,a_datetime,"Thursday, February 5, 1970",a_date);

    TEST_PARSE_FAILS(as::date>>as::date_short,"aa/bb/cc",double);

    TEST_FMT_PARSE_2_2(as::time,                as::gmt,a_datetime,"3:33:13 PM",a_time+a_timesec);
    TEST_FMT_PARSE_3_2(as::time,as::time_short ,as::gmt,a_datetime,"3:33 PM",a_time);
    TEST_FMT_PARSE_3_2(as::time,as::time_medium,as::gmt,a_datetime,"3:33:13 PM",a_time+a_timesec);
    #if BOOST_LOCALE_ICU_VERSION >= 408
        TEST_FMT_PARSE_3_2(as::time,as::time_long  ,as::gmt,a_datetime,"3:33:13 PM GMT",a_time+a_timesec);
        #if BOOST_LOCALE_ICU_VERSION_EXACT != 40800
            // know bug #8675
            TEST_FMT_PARSE_3_2(as::time,as::time_full  ,as::gmt,a_datetime,"3:33:13 PM " + icu_full_gmt_name,a_time+a_timesec);
        #endif
    #else
        TEST_FMT_PARSE_3_2(as::time,as::time_long  ,as::gmt,a_datetime,"3:33:13 PM GMT+00:00",a_time+a_timesec);
        TEST_FMT_PARSE_3_2(as::time,as::time_full  ,as::gmt,a_datetime,"3:33:13 PM GMT+00:00",a_time+a_timesec);
    #endif

    TEST_PARSE_FAILS(as::time,"AM",double);

    TEST_FMT_PARSE_2_2(as::time,                as::time_zone("GMT+01:00"),a_datetime,"4:33:13 PM",a_time+a_timesec);
    TEST_FMT_PARSE_3_2(as::time,as::time_short ,as::time_zone("GMT+01:00"),a_datetime,"4:33 PM",a_time);
    TEST_FMT_PARSE_3_2(as::time,as::time_medium,as::time_zone("GMT+01:00"),a_datetime,"4:33:13 PM",a_time+a_timesec);

#if U_ICU_VERSION_MAJOR_NUM >= 51
#define GMT_P100 "GMT+1"
#else
#define GMT_P100 "GMT+01:00"
#endif


#if U_ICU_VERSION_MAJOR_NUM >= 50
#define PERIOD ","
#define ICUAT " at"
#else
#define PERIOD ""
#define ICUAT ""
#endif

    TEST_FMT_PARSE_3_2(as::time,as::time_long  ,as::time_zone("GMT+01:00"),a_datetime,"4:33:13 PM "  GMT_P100,a_time+a_timesec);
    #if BOOST_LOCALE_ICU_VERSION == 308 && defined(__CYGWIN__)
    // Known failure ICU issue
    #else
    TEST_FMT_PARSE_3_2(as::time,as::time_full  ,as::time_zone("GMT+01:00"),a_datetime,"4:33:13 PM GMT+01:00",a_time+a_timesec);
    #endif

    TEST_FMT_PARSE_2(as::datetime,                                as::gmt,a_datetime,"Feb 5, 1970" PERIOD  " 3:33:13 PM");
    TEST_FMT_PARSE_4_2(as::datetime,as::date_short ,as::time_short ,as::gmt,a_datetime,"2/5/70" PERIOD " 3:33 PM",a_date+a_time);
    TEST_FMT_PARSE_4(as::datetime,as::date_medium,as::time_medium,as::gmt,a_datetime,"Feb 5, 1970" PERIOD " 3:33:13 PM");
    #if BOOST_LOCALE_ICU_VERSION >= 408
    TEST_FMT_PARSE_4(as::datetime,as::date_long  ,as::time_long  ,as::gmt,a_datetime,"February 5, 1970" ICUAT " 3:33:13 PM GMT");
        #if BOOST_LOCALE_ICU_VERSION_EXACT != 40800
            // know bug #8675
            TEST_FMT_PARSE_4(as::datetime,as::date_full  ,as::time_full  ,as::gmt,a_datetime,"Thursday, February 5, 1970" ICUAT " 3:33:13 PM " + icu_full_gmt_name);
        #endif
    #else
    TEST_FMT_PARSE_4(as::datetime,as::date_long  ,as::time_long  ,as::gmt,a_datetime,"February 5, 1970" PERIOD " 3:33:13 PM GMT+00:00");
    TEST_FMT_PARSE_4(as::datetime,as::date_full  ,as::time_full  ,as::gmt,a_datetime,"Thursday, February 5, 1970" PERIOD " 3:33:13 PM GMT+00:00");
    #endif

    time_t now=time(0);
    time_t lnow = now + 3600 * 4;
    char local_time_str[256];
    std::string format="%H:%M:%S";
    std::basic_string<CharType> format_string(format.begin(),format.end());
    strftime(local_time_str,sizeof(local_time_str),format.c_str(),gmtime(&lnow));
    TEST_FMT(as::ftime(format_string),now,local_time_str);
    TEST_FMT(as::ftime(format_string) << as::gmt << as::local_time,now,local_time_str);

    std::string marks =
        "aAbB"
        "cdeh"
        "HIjm"
        "Mnpr"
        "RStT"
        "xXyY"
        "Z%";

    std::string result[]= {
        "Thu","Thursday","Feb","February",  // aAbB
        "Thursday, February 5, 1970" ICUAT " 3:33:13 PM " + icu_full_gmt_name, // c
        "05","5","Feb", // deh
        "15","03","36","02", // HIjm
        "33","\n","PM", "03:33:13 PM",// Mnpr
        "15:33","13","\t","15:33:13", // RStT
        "Feb 5, 1970","3:33:13 PM","70","1970", // xXyY
        icu_full_gmt_name // Z
        ,"%" }; // %

    for(unsigned i=0;i<marks.size();i++) {
        format_string.clear();
        format_string+=static_cast<CharType>('%');
        format_string += static_cast<CharType>(marks[i]);
        TEST_FMT(as::ftime(format_string) << as::gmt,a_datetime,result[i]);
    }

    std::string sample_f[]={
        "Now is %A, %H o'clo''ck ' or not ' ",
        "'test %H'",
        "%H'",
        "'%H'"
    };
    std::string expected_f[] = {
        "Now is Thursday, 15 o'clo''ck ' or not ' ",
        "'test 15'",
        "15'",
        "'15'"
    };

    for(unsigned i=0;i<sizeof(sample_f)/sizeof(sample_f[0]);i++) {
        format_string.assign(sample_f[i].begin(),sample_f[i].end());
        TEST_FMT(as::ftime(format_string) << as::gmt,a_datetime,expected_f[i]);
    }

}

template<typename CharType>
void test_format_class(std::string charset="UTF-8")
{
    boost::locale::generator g;
    std::locale loc=g(test_locale_name + "." + charset);

    TEST_FORMAT_CLS("{3} {1} {2}", 1 % 2 % 3,"3 1 2");
    TEST_FORMAT_CLS("{1} {2}", "hello" % 2,"hello 2");
    TEST_FORMAT_CLS("{1}",1200.1,"1200.1");
    TEST_FORMAT_CLS("Test {1,num}",1200.1,"Test 1,200.1");
    TEST_FORMAT_CLS("{{}} {1,number}",1200.1,"{} 1,200.1");
    #if BOOST_LOCALE_ICU_VERSION < 5601
    // bug #13276
    TEST_FORMAT_CLS("{1,num=sci,p=3}",13.1,"1.310E1");
    TEST_FORMAT_CLS("{1,num=scientific,p=3}",13.1,"1.310E1");
    #endif
    TEST_FORMAT_CLS("{1,num=fix,p=3}",13.1,"13.100");
    TEST_FORMAT_CLS("{1,num=fixed,p=3}",13.1,"13.100");
    TEST_FORMAT_CLS("{1,<,w=3,num}",-1,"-1 ");
    TEST_FORMAT_CLS("{1,>,w=3,num}",1,"  1");
    TEST_FORMAT_CLS("{per,1}",0.1,"10%");
    TEST_FORMAT_CLS("{percent,1}",0.1,"10%");
    TEST_FORMAT_CLS("{1,cur}",1234,"$1,234.00");
    TEST_FORMAT_CLS("{1,currency}",1234,"$1,234.00");
    if(charset=="UTF-8") {
#if BOOST_LOCALE_ICU_VERSION >= 400
            TEST_FORMAT_CLS("{1,cur,locale=de_DE}",10,"10,00\xC2\xA0€");
#else
            TEST_FORMAT_CLS("{1,cur,locale=de_DE}",10,"10,00 €");
#endif
    }
    #if BOOST_LOCALE_ICU_VERSION >= 402
    TEST_FORMAT_CLS("{1,cur=nat}",1234,"$1,234.00");
    TEST_FORMAT_CLS("{1,cur=national}",1234,"$1,234.00");
    TEST_FORMAT_CLS("{1,cur=iso}",1234,get_icu_currency_iso(1234));
    #endif
    TEST_FORMAT_CLS("{1,spell}",10,"ten");
    TEST_FORMAT_CLS("{1,spellout}",10,"ten");
    #if 402 <= BOOST_LOCALE_ICU_VERSION && BOOST_LOCALE_ICU_VERSION < 408
    if(charset=="UTF-8") {
        TEST_FORMAT_CLS("{1,ord}",1,"1\xcb\xa2\xe1\xb5\x97");
        TEST_FORMAT_CLS("{1,ordinal}",1,"1\xcb\xa2\xe1\xb5\x97");
    }
    #else
    TEST_FORMAT_CLS("{1,ord}",1,"1st");
    TEST_FORMAT_CLS("{1,ordinal}",1,"1st");
    #endif

    time_t now=time(0);
    time_t lnow = now + 3600 * 4;
    char local_time_str[256];
    std::string format="'%H:%M:%S'";
    std::basic_string<CharType> format_string(format.begin(),format.end());
    strftime(local_time_str,sizeof(local_time_str),format.c_str(),gmtime(&lnow));

    TEST_FORMAT_CLS("{1,ftime='''%H:%M:%S'''}",now,local_time_str);
    TEST_FORMAT_CLS("{1,local,ftime='''%H:%M:%S'''}",now,local_time_str);
    TEST_FORMAT_CLS("{1,ftime='''%H:%M:%S'''}",now,local_time_str);

    time_t a_date = 3600*24*(31+4); // Feb 5th
    time_t a_time = 3600*15+60*33; // 15:33:05
    time_t a_timesec = 13;
    time_t a_datetime = a_date + a_time + a_timesec;
    TEST_FORMAT_CLS("{1,date,gmt};{1,time,gmt};{1,datetime,gmt};{1,dt,gmt}",a_datetime,
            "Feb 5, 1970;3:33:13 PM;Feb 5, 1970" PERIOD " 3:33:13 PM;Feb 5, 1970" PERIOD " 3:33:13 PM");
    #if BOOST_LOCALE_ICU_VERSION >= 408
    TEST_FORMAT_CLS("{1,time=short,gmt};{1,time=medium,gmt};{1,time=long,gmt};{1,date=full,gmt}",a_datetime,
            "3:33 PM;3:33:13 PM;3:33:13 PM GMT;Thursday, February 5, 1970");
    TEST_FORMAT_CLS("{1,time=s,gmt};{1,time=m,gmt};{1,time=l,gmt};{1,date=f,gmt}",a_datetime,
            "3:33 PM;3:33:13 PM;3:33:13 PM GMT;Thursday, February 5, 1970");
    #else
    TEST_FORMAT_CLS("{1,time=short,gmt};{1,time=medium,gmt};{1,time=long,gmt};{1,date=full,gmt}",a_datetime,
            "3:33 PM;3:33:13 PM;3:33:13 PM GMT+00:00;Thursday, February 5, 1970");
    TEST_FORMAT_CLS("{1,time=s,gmt};{1,time=m,gmt};{1,time=l,gmt};{1,date=f,gmt}",a_datetime,
            "3:33 PM;3:33:13 PM;3:33:13 PM GMT+00:00;Thursday, February 5, 1970");
    #endif
    TEST_FORMAT_CLS("{1,time=s,tz=GMT+01:00}",a_datetime,"4:33 PM");
    TEST_FORMAT_CLS("{1,time=s,timezone=GMT+01:00}",a_datetime,"4:33 PM");

    TEST_FORMAT_CLS("{1,gmt,ftime='%H'''}",a_datetime,"15'");
    TEST_FORMAT_CLS("{1,gmt,ftime='''%H'}",a_datetime,"'15");
    TEST_FORMAT_CLS("{1,gmt,ftime='%H o''clock'}",a_datetime,"15 o'clock");

    // Test not a year of the week
    a_datetime=1388491200; // 2013-12-31 12:00 - check we don't use week of year

    TEST_FORMAT_CLS("{1,gmt,ftime='%Y'}",a_datetime,"2013");
    TEST_FORMAT_CLS("{1,gmt,ftime='%y'}",a_datetime,"13");
    TEST_FORMAT_CLS("{1,gmt,ftime='%D'}",a_datetime,"12/31/13");
}


void test_main(int /*argc*/, char** /*argv*/)
{
    boost::locale::time_zone::global("GMT+4:00");
    std::cout << "Testing char, UTF-8" << std::endl;
    test_manip<char>();
    test_format_class<char>();
    std::cout << "Testing char, ISO8859-1" << std::endl;
    test_manip<char>("ISO8859-1");
    test_format_class<char>("ISO8859-1");

    std::cout << "Testing wchar_t" << std::endl;
    test_manip<wchar_t>();
    test_format_class<wchar_t>();

    #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
    std::cout << "Testing char16_t" << std::endl;
    test_manip<char16_t>();
    test_format_class<char16_t>();
    #endif

    #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
    std::cout << "Testing char32_t" << std::endl;
    test_manip<char32_t>();
    test_format_class<char32_t>();
    #endif
}

#endif // NOICU

// boostinspect:noascii
// boostinspect:nominmax
