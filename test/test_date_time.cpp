//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <boost/locale/date_time.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/formatting.hpp>
#include <boost/locale/localization_backend.hpp>
#include <ctime>
#include <iomanip>
#include "boostLocale/test/unit_test.hpp"

#ifdef BOOST_LOCALE_WITH_ICU
#include <unicode/uversion.h>
#define BOOST_LOCALE_ICU_VERSION (U_ICU_VERSION_MAJOR_NUM*100 + U_ICU_VERSION_MINOR_NUM)
#else
#define BOOST_LOCALE_ICU_VERSION 0
#endif

#ifdef BOOST_MSVC
#  pragma warning(disable : 4244) // loose data
#endif

#define TEST_EQ_FMT(t,X)                             \
    ss.str(""); ss << (t);                           \
    test_eq_impl(ss.str(), X, #t "==" #X, __LINE__)

void test_main(int /*argc*/, char** /*argv*/)
{
    using namespace boost::locale;
    using namespace boost::locale::period;
    std::string def[] = {
    #ifdef BOOST_LOCALE_WITH_ICU
        "icu",
    #endif
    #ifndef BOOST_LOCALE_NO_STD_BACKEND
        "std",
    #endif
    #ifndef BOOST_LOCALE_NO_POSIX_BACKEND
        "posix",
    #endif
    #ifndef BOOST_LOCALE_NO_WINAPI_BACKEND
        "winapi",
    #endif
    };
    for(int type = 0 ; type < int(sizeof(def)/sizeof(def[0])) ; type ++ ) {
        boost::locale::localization_backend_manager tmp_backend = boost::locale::localization_backend_manager::global();
        tmp_backend.select(def[type]);
        boost::locale::localization_backend_manager::global(tmp_backend);
        const std::string backend_name = def[type];
        std::cout << "Testing for backend: " << backend_name << std::endl;
        {

            boost::locale::generator g;

            std::locale loc=g("en_US.UTF-8");

            std::locale::global(loc);

            const std::string tz = "GMT";
            time_zone::global(tz);
            // A call returns the old tz
            TEST_EQ(time_zone::global("GMT+01:00"), tz);
            TEST_EQ(time_zone::global(tz), "GMT+01:00");
            calendar cal(loc, tz);
            TEST(cal.get_locale() == loc);
            TEST(cal.get_time_zone() == tz);

            TEST(calendar() == cal);
            TEST(calendar(loc) == cal);
            TEST(calendar(tz) == cal);
            TEST(calendar(loc,"GMT+01:00") != cal);
            TEST(calendar(g("ru_RU.UTF-8")) != cal);

            TEST_EQ(cal.minimum(month()), 0);
            TEST_EQ(cal.maximum(month()), 11);
            TEST_EQ(cal.minimum(day()), 1);
            TEST_EQ(cal.greatest_minimum(day()), 1);
            TEST_EQ(cal.least_maximum(day()), 28);
            TEST_EQ(cal.maximum(day()), 31);
            TEST(cal.is_gregorian());

            TEST_EQ(calendar(g("ar_EG.UTF-8")).first_day_of_week(), 7);
            TEST_EQ(calendar(g("he_IL.UTF-8")).first_day_of_week(), 1);
            TEST_EQ(calendar(g("ru_RU.UTF-8")).first_day_of_week(), 2);

            std::ostringstream ss;
            ss.imbue(loc);
            ss << boost::locale::as::time_zone(tz);

            const time_t one_h = 60 * 60;
            const time_t a_date = 24 * one_h * (31+4); // Feb 5th
            const time_t a_time = 15 * one_h + 60 * 33 + 13; // 15:33:13
            const time_t a_datetime = a_date + a_time;

            const date_time tp_5_feb_1970_153313 = date_time(a_datetime); /// 5th Feb 1970 15:33:13
            ss << as::ftime("%Y-%m-%d");
            TEST_EQ_FMT(tp_5_feb_1970_153313, "1970-02-05");
            ss << as::ftime("%Y-%m-%d %H:%M:%S");
            TEST_EQ_FMT(tp_5_feb_1970_153313, "1970-02-05 15:33:13");

            // Test set()
            date_time time_point = tp_5_feb_1970_153313;
            time_point.set(year(), 1990);
            TEST_EQ_FMT(time_point, "1990-02-05 15:33:13");
            time_point.set(month(), 5);
            TEST_EQ_FMT(time_point, "1990-06-05 15:33:13");
            time_point.set(day(), 9);
            TEST_EQ_FMT(time_point, "1990-06-09 15:33:13");
            time_point.set(hour(), 11);
            TEST_EQ_FMT(time_point, "1990-06-09 11:33:13");
            time_point.set(minute(), 42);
            TEST_EQ_FMT(time_point, "1990-06-09 11:42:13");
            time_point.set(second(), 24);
            TEST_EQ_FMT(time_point, "1990-06-09 11:42:24");
            time_point.set(am_pm(), 1);
            TEST_EQ_FMT(time_point, "1990-06-09 23:42:24");
            // Overflow day of month
            time_point.set(day(), time_point.maximum(day()) + 1);
            TEST_EQ_FMT(time_point, "1990-07-01 23:42:24");

            // Same via assignment
            time_point = tp_5_feb_1970_153313;
            time_point = year(1990);
            TEST_EQ_FMT(time_point, "1990-02-05 15:33:13");
            time_point = month(5);
            TEST_EQ_FMT(time_point, "1990-06-05 15:33:13");
            time_point = day(9);
            TEST_EQ_FMT(time_point, "1990-06-09 15:33:13");
            time_point = hour(11);
            TEST_EQ_FMT(time_point, "1990-06-09 11:33:13");
            time_point = minute(42);
            TEST_EQ_FMT(time_point, "1990-06-09 11:42:13");
            time_point = second(24);
            TEST_EQ_FMT(time_point, "1990-06-09 11:42:24");
            time_point = am_pm(1);
            TEST_EQ_FMT(time_point, "1990-06-09 23:42:24");
            // Overflow day of month
            time_point = day(time_point.maximum(day()) + 1);
            TEST_EQ_FMT(time_point, "1990-07-01 23:42:24");
            // All at once
            time_point = year(1989) + month(2) + day(5) + hour(7) + minute(9) + second(11);
            TEST_EQ_FMT(time_point, "1989-03-05 07:09:11");
            // Partials:
            time_point = year(1970) + february() + day(5);
            TEST_EQ_FMT(time_point, "1970-02-05 07:09:11");
            time_point = 3 * hour_12() + 1 * am_pm() + 33 * minute() + 13 * second();
            TEST_EQ_FMT(time_point, "1970-02-05 15:33:13");

            time_point = tp_5_feb_1970_153313;
            time_point += hour();
            TEST_EQ_FMT(time_point,"1970-02-05 16:33:13");

            TEST_EQ(time_point.minimum(day()), 1);
            TEST_EQ(time_point.maximum(day()), 28);

            time_point = tp_5_feb_1970_153313;
            time_point += year() * 2 + 1 *month();
            TEST_EQ_FMT(time_point,"1972-03-05 15:33:13");

            time_point = tp_5_feb_1970_153313;
            time_point -= minute();
            TEST_EQ_FMT( time_point, "1970-02-05 15:32:13");

            time_point = tp_5_feb_1970_153313;
            time_point <<= minute() * 30;
            TEST_EQ_FMT( time_point, "1970-02-05 15:03:13");

            time_point = tp_5_feb_1970_153313;
            time_point >>= minute(40);
            TEST_EQ_FMT( time_point, "1970-02-05 15:53:13");

            time_point = tp_5_feb_1970_153313;
            TEST_EQ((time_point + month()) / month(), 2);
            TEST_EQ(month(time_point + month(1)), 2);
            TEST_EQ(time_point / month(), 1);
            TEST_EQ((time_point - month()) / month(), 0);
            TEST_EQ(time_point / month(), 1);
            TEST_EQ((time_point << month()) / month(), 2);
            TEST_EQ(time_point / month(), 1);
            TEST_EQ((time_point >> month()) / month(), 0);
            TEST_EQ(time_point / month(), 1);

            TEST_EQ( (time_point + 2 * hour() - time_point) / minute(), 120);
            TEST_EQ( (time_point + month()- time_point) / day(), 28);
            TEST_EQ( (time_point + 2* month()- (time_point+month())) / day(), 31);
            TEST_EQ( day(time_point + 2* month()- (time_point+month())), 31);

            // To subtract from the year, don't use 1970 which may be the lowest possible year
            const date_time tp_5_april_1990_153313 = (date_time(tp_5_feb_1970_153313) = (year(1990) + april()));
            TEST_EQ_FMT(tp_5_april_1990_153313, "1990-04-05 15:33:13");
            // Test each period
            TEST_EQ_FMT(tp_5_april_1990_153313 + year(2), "1992-04-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << year(2), "1992-04-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - year(10), "1980-04-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> year(10), "1980-04-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + month(2), "1990-06-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << month(2), "1990-06-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - month(1), "1990-03-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> month(1), "1990-03-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + day(2), "1990-04-07 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << day(2), "1990-04-07 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - day(3), "1990-04-02 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> day(3), "1990-04-02 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + hour(2), "1990-04-05 17:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << hour(2), "1990-04-05 17:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - hour(3), "1990-04-05 12:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> hour(3), "1990-04-05 12:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + minute(2), "1990-04-05 15:35:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << minute(2), "1990-04-05 15:35:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - minute(3), "1990-04-05 15:30:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> minute(3), "1990-04-05 15:30:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + second(2), "1990-04-05 15:33:15");
            TEST_EQ_FMT(tp_5_april_1990_153313 << second(2), "1990-04-05 15:33:15");
            TEST_EQ_FMT(tp_5_april_1990_153313 - second(2), "1990-04-05 15:33:11");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> second(2), "1990-04-05 15:33:11");
            // Difference between add and roll: The latter only changes the given field
            // So this tests what happens when going over/under the bound for each field
            TEST_EQ_FMT(tp_5_april_1990_153313 + month(12 + 2), "1991-06-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << month(12 + 2), "1990-06-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - month(12 * 3 + 1), "1987-03-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> month(12 * 3 + 1), "1990-03-05 15:33:13");
            // Check that possible int overflows get handled
            TEST_EQ_FMT(tp_5_april_1990_153313 >> month((std::numeric_limits<int>::max() / 12) * 12), "1990-04-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << month((std::numeric_limits<int>::max() / 12) * 12), "1990-04-05 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + day(30 + 2), "1990-05-07 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << day(30 + 2), "1990-04-07 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - day(10), "1990-03-26 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> day(10), "1990-04-25 15:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + hour(24 * 3 + 2), "1990-04-08 17:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << hour(24 * 3 + 2), "1990-04-05 17:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - hour(24 * 5 + 3), "1990-03-31 12:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> hour(24 * 5 + 3), "1990-04-05 12:33:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + minute(60 * 5 + 3), "1990-04-05 20:36:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 << minute(60 * 5 + 3), "1990-04-05 15:36:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 - minute(60 * 5 + 3), "1990-04-05 10:30:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> minute(60 * 5 + 3), "1990-04-05 15:30:13");
            TEST_EQ_FMT(tp_5_april_1990_153313 + second(60 * 3 + 2), "1990-04-05 15:36:15");
            TEST_EQ_FMT(tp_5_april_1990_153313 << second(60 * 3 + 2), "1990-04-05 15:33:15");
            TEST_EQ_FMT(tp_5_april_1990_153313 - second(60 * 5 + 2), "1990-04-05 15:28:11");
            TEST_EQ_FMT(tp_5_april_1990_153313 >> second(60 * 5 + 2), "1990-04-05 15:33:11");

            // Add a set of periods
            TEST_EQ_FMT(tp_5_feb_1970_153313 << (year(2) + month(3) - day(1) + hour(5) + minute(7) + second(9)), "1972-05-04 20:40:22");
            TEST_EQ_FMT(tp_5_feb_1970_153313 + (year(2) + month(3) - day(1) + hour(5) + minute(7) + second(9)), "1972-05-04 20:40:22");
            // std calendar can't go below 1970
            time_point = tp_5_feb_1970_153313;
            time_point = year(1972) + july();
            TEST_EQ_FMT(time_point, "1972-07-05 15:33:13");
            TEST_EQ_FMT(time_point >> (year(2) + month(3) - day(11) + hour(5) + minute(7) + second(9)), "1970-04-16 10:26:04");
            TEST_EQ_FMT(time_point - (year(2) + month(3) - day(11) + hour(5) + minute(7) + second(9)), "1970-04-16 10:26:04");

            time_point = tp_5_feb_1970_153313;
            TEST(time_point == tp_5_feb_1970_153313);
            TEST(!(time_point != tp_5_feb_1970_153313));
            TEST_EQ(time_point.get(hour()), 15);
            TEST_EQ(time_point/hour(), 15);
            TEST(time_point+year() != time_point);
            TEST(time_point - minute() <= time_point);
            TEST(time_point <= time_point);
            TEST(time_point + minute() >= time_point);
            TEST(time_point >= time_point);

            TEST(time_point < time_point + second());
            TEST(!(time_point < time_point - second()));
            TEST(time_point > time_point - second());
            TEST(!(time_point > time_point + second()));

            TEST_EQ(time_point.get(day()), 5);
            TEST_EQ(time_point.get(year()), 1970);

            TEST_EQ(time_point.get(era()), 1);
            TEST_EQ(time_point.get(year()), 1970);
            TEST_EQ(time_point.get(extended_year()), 1970);
            if(backend_name == "icu") {
                time_point=extended_year(-3);
                TEST_EQ(time_point.get(era()), 0);
                TEST_EQ(time_point.get(year()), 4);
            }

            time_point = tp_5_feb_1970_153313;
            TEST_EQ(time_point.get(month()), 1);
            TEST_EQ(time_point.get(day()), 5);
            TEST_EQ(time_point.get(day_of_year()), 36);
            TEST_EQ(time_point.get(day_of_week()), 5);
            TEST_EQ(time_point.get(day_of_week_in_month()),1);
            time_point=date_time(a_datetime,calendar(g("ru_RU.UTF-8")));
            TEST_EQ(time_point.get(day_of_week_local()), 4);
            time_point = year(2026) + january() + day(1);
            TEST_EQ(time_point.get(day_of_week()), 5);
            TEST_EQ(time_point.get(week_of_year()), 1);
            TEST_EQ(time_point.get(week_of_month()), 1);
            time_point = day_of_week() * 1;
            TEST_EQ(time_point.get(day()), 4);
            TEST_EQ(time_point.get(week_of_year()), 1);
            TEST_EQ(time_point.get(week_of_month()), 1);
            time_point += day() * 1;
            TEST_EQ(time_point.get(week_of_year()), 2);
            TEST_EQ(time_point.get(week_of_month()), 2);

            time_point = february() + day() * 2;

            TEST_EQ(time_point.get(week_of_year()), 6);

            // cldr changes
#if BOOST_LOCALE_ICU_VERSION >= 408 && BOOST_LOCALE_ICU_VERSION <= 6000
            const bool ICU_cldr_issue = backend_name == "icu";
#else
            const bool ICU_cldr_issue = false;
#endif
BOOST_LOCALE_START_CONST_CONDITION

            if((ICU_cldr_issue))
                TEST_EQ(time_point.get(week_of_month()), 2);
            else
                TEST_EQ(time_point.get(week_of_month()), 1);

            time_point = year(2010) + january() + day() * 3;

            if((ICU_cldr_issue))
                TEST_EQ(time_point.get(week_of_year()), 1);
            else
                TEST_EQ(time_point.get(week_of_year()), 53);

            time_point = year()*2010 + january() + day() * 4;

            if((ICU_cldr_issue))
                TEST_EQ(time_point.get(week_of_year()), 2);
            else
                TEST_EQ(time_point.get(week_of_year()), 1);

            time_point = year()*2010 + january() + day() * 10;

            if((ICU_cldr_issue))
                TEST_EQ(time_point.get(week_of_year()), 2);
            else
                TEST_EQ(time_point.get(week_of_year()), 1);

            time_point = year()*2010 + january() + day() * 11;

            if((ICU_cldr_issue))
                TEST_EQ(time_point.get(week_of_year()), 3);
            else 
                TEST_EQ(time_point.get(week_of_year()), 2);

BOOST_LOCALE_END_CONST_CONDITION

            time_point = date_time(a_datetime);
            TEST_EQ(time_point.get(hour()), 15);
            TEST_EQ(date_time(a_datetime,calendar("GMT+01:00")).get(hour()),16);
            TEST_EQ(time_point.get(hour_12()), 3);
            TEST_EQ(time_point.get(am_pm()), 1);
            TEST_EQ(time_point.get(minute()), 33);
            TEST_EQ(time_point.get(second()), 13);
            TEST_EQ(date_time(year() * 1984 + february() + day()).get(week_of_year()),5);
            TEST_EQ(time_point.get(week_of_month()), 1);

            // Make sure we don't get year() < 1970 so the test would
            // work on windows where mktime supports positive time_t
            // only
            time_point = year() * 2010;

            TEST_EQ((time_point + year() *1 - hour() * 1 - time_point) / year(), 0);
            TEST_EQ((time_point + year() *1 - time_point) / year(), 1);
            TEST_EQ((time_point + year() *1 + hour() * 1 - time_point) / year(), 1);
            TEST_EQ((time_point - year() *1 + hour() * 1 - time_point) / year(), 0);
            TEST_EQ((time_point - year() *1 - time_point) / year(), -1);
            TEST_EQ((time_point - year() *1 - hour() * 1 - time_point) / year(), -1);


            time_point.time(24*3600. * 2);

            time_point = year() * 2011;
            time_point = march();
            time_point = day() * 29;

            TEST_EQ(time_point.get(year()), 2011);
            TEST_EQ(time_point.get(month()), 2); // march
            TEST_EQ(time_point.get(day()), 29);

            date_time tp_29_march_2011 = time_point;

            time_point = year() * 2011;
            time_point = february();
            time_point = day() * 5;

            TEST_EQ(time_point.get(year()), 2011);
            TEST_EQ(time_point.get(month()), 2); // march
            TEST_EQ(time_point.get(day()), 5);

            time_point = tp_29_march_2011;

            time_point = year() * 2011 + february() + day() * 5;
            TEST_EQ(time_point.get(year()), 2011);
            TEST_EQ(time_point.get(month()), 1); // february
            TEST_EQ(time_point.get(day()), 5);

            // Default constructed time_point
            {
                const time_t current_time = std::time(0);
                const tm current_time_gmt = *std::gmtime(&current_time);
                date_time time_point_default;
                date_time time_point_1970 = year(1970) + february() + day(5);
                // Defaults to current time, i.e. different than a date in 1970
                TEST(time_point_default != time_point_1970);

                TEST_EQ(time_point_default.get(year()), current_time_gmt.tm_year + 1900);
                TEST_EQ(time_point_default.get(month()), current_time_gmt.tm_mon);
                TEST_EQ(time_point_default.get(day()), current_time_gmt.tm_mday);
                TEST_EQ(time_point_default.get(hour()), current_time_gmt.tm_hour);
                TEST_EQ(time_point_default.get(minute()), current_time_gmt.tm_min);
                TEST_EQ(time_point_default.get(second()), current_time_gmt.tm_sec);

                // Uses the current global timezone
                time_zone::global("GMT");
                date_time tp_gmt;
                time_zone::global("GMT+01:00");
                date_time tp_gmt1;
                // Both refer to the same point in time (i.e. comparison ignores timezones)
                TEST(tp_gmt == tp_gmt1);

                // But getting the hour shows the difference of 1 hour
                const int gmt_h = tp_gmt.get(hour());
                // Handle overflow to next day
                const int expected_gmt1_h = (gmt_h == tp_gmt.maximum(hour())) ? tp_gmt.minimum(hour()) : gmt_h + 1;
                TEST_EQ(expected_gmt1_h, tp_gmt1.get(hour()));
                // Adding the hour automatically handles the overflow, so this works too
                tp_gmt += hour();
                TEST_EQ(tp_gmt.get(hour()), tp_gmt1.get(hour()));
            }

        } // test
    }   // for loop
}

// boostinspect:noascii
