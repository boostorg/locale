//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "../src/boost/locale/shared/mo_lambda.hpp"
#include "boostLocale/test/unit_test.hpp"
#include <iostream>
#include <limits>
#include <random>

// We want to use `boost::locale::gnu_gettext::lambda::plural`
// without exporting it which leads to false positives under UBSAN -> Disable this check
#ifdef __has_attribute
#    if __has_attribute(no_sanitize)
#        define BOOST_LOCALE_WRONG_VPTR_OK __attribute__((no_sanitize("vptr")))
#    else
#        define BOOST_LOCALE_WRONG_VPTR_OK
#    endif
#else
#    define BOOST_LOCALE_WRONG_VPTR_OK
#endif

template<typename T>
T getRandValue(const T min, const T max)
{
    static std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<T> distrib(min, max);
    return distrib(gen);
}

template<typename T>
BOOST_LOCALE_WRONG_VPTR_OK void test_plural_expr_rand(const T& ref, const char* expr)
{
    constexpr auto minVal = std::numeric_limits<int>::min() / 1024;
    constexpr auto maxVal = std::numeric_limits<int>::max() / 1024;
    const auto ptr = boost::locale::gnu_gettext::lambda::compile(expr);
    constexpr int number_of_tries = 256;
    for(int i = 0; i < number_of_tries; i++) {
        const auto n = getRandValue(minVal, maxVal);
        const auto result = (*ptr)(n);
        const auto refResult = ref(n);
        if(result != refResult) {
            std::cerr << "Expression: " << expr << "; n=" << n << '\n'; // LCOV_EXCL_LINE
            TEST_EQ(result, refResult);                                 // LCOV_EXCL_LINE
        }
    }
}

BOOST_LOCALE_WRONG_VPTR_OK void test_plural_expr()
{
#define TEST_PLURAL_EXPR(expr) \
    test_plural_expr_rand(     \
      [](int n) {              \
          (void)n;             \
          return expr;         \
      },                       \
      #expr);
    TEST_PLURAL_EXPR(42);
    TEST_PLURAL_EXPR(1337);
    TEST_PLURAL_EXPR(n + 3);
    TEST_PLURAL_EXPR(n - 3);
    TEST_PLURAL_EXPR(n * 5);
    TEST_PLURAL_EXPR(n / 5);
    TEST_PLURAL_EXPR(n % 8);
    // Parentheses
    TEST_PLURAL_EXPR((5 * n) + 3);
    TEST_PLURAL_EXPR(5 * (n + 3));
    // Comparisons and ternary
    TEST_PLURAL_EXPR(n % 2 == 0 ? n + 5 : n * 3);
    TEST_PLURAL_EXPR(n % 2 != 0 ? n + 5 : n * 3);
    TEST_PLURAL_EXPR(n % 4 < 2 ? n + 5 : n * 3);
    TEST_PLURAL_EXPR(n % 4 <= 2 ? n + 5 : n * 3);
    TEST_PLURAL_EXPR(n % 4 > 2 ? n + 5 : n * 3);
    TEST_PLURAL_EXPR(n % 4 >= 2 ? n + 5 : n * 3);
    // Complex expression (e.g. for Russian)
    TEST_PLURAL_EXPR((n % 10 == 1 && n % 100 != 11                                  ? 0 :
                      n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20) ? 1 :
                                                                                      2));
#undef TEST_PLURAL_EXPR

    using boost::locale::gnu_gettext::lambda::compile;
    constexpr auto minVal = std::numeric_limits<int>::min();
    constexpr auto maxVal = std::numeric_limits<int>::max();

    // E.g. Japanese
    {
        const auto ptr = compile("0");
        TEST(ptr);
        const auto& p = *ptr;
        TEST_EQ(p(0), 0);
        TEST_EQ(p(minVal), 0);
        TEST_EQ(p(maxVal), 0);
    }
    // E.g. English
    {
        const auto ptr = compile("(n != 1)");
        TEST(ptr);
        const auto& p = *ptr;
        TEST_EQ(p(0), 1);
        TEST_EQ(p(1), 0);
        TEST_EQ(p(minVal), 1);
        TEST_EQ(p(maxVal), 1);
    }
    // E.g. French
    {
        const auto ptr = compile("(n > 1)");
        TEST(ptr);
        const auto& p = *ptr;
        TEST_EQ(p(0), 0);
        TEST_EQ(p(1), 0);
        TEST_EQ(p(2), 1);
        TEST_EQ(p(minVal), 0);
        TEST_EQ(p(maxVal), 1);
    }
    // E.g. Latvian
    {
        const auto ptr = compile("(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2)");
        TEST(ptr);
        const auto& p = *ptr;
        TEST_EQ(p(0), 2);
        TEST_EQ(p(1), 0);
        TEST_EQ(p(2), 1);
        TEST_EQ(p(3), 1);
        TEST_EQ(p(11), 1);
        TEST_EQ(p(12), 1);
        TEST_EQ(p(21), 0);
        TEST_EQ(p(31), 0);
        TEST_EQ(p(101), 0);
        TEST_EQ(p(111), 1);
    }
    // E.g. Irish
    {
        const auto ptr = compile("n == 1 ? 0 : n == 2 ? 1 : 2");
        TEST(ptr);
        const auto& p = *ptr;
        TEST_EQ(p(0), 2);
        TEST_EQ(p(1), 0);
        TEST_EQ(p(2), 1);
        TEST_EQ(p(3), 2);
        TEST_EQ(p(4), 2);
        TEST_EQ(p(minVal), 2);
        TEST_EQ(p(maxVal), 2);
    }
    // E.g. Czech
    {
        const auto ptr = compile("(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2");
        TEST(ptr);
        const auto& p = *ptr;
        TEST_EQ(p(0), 2);
        TEST_EQ(p(1), 0);
        TEST_EQ(p(2), 1);
        TEST_EQ(p(3), 1);
        TEST_EQ(p(4), 1);
        TEST_EQ(p(5), 2);
        TEST_EQ(p(minVal), 2);
        TEST_EQ(p(maxVal), 2);
    }
    // Error cases
    TEST(!compile("") && compile("n")); // Empty
    // Invalid comparison
    TEST(!compile("n===1") && compile("n==1"));
    TEST(!compile("n!1") && compile("n!=1"));
    TEST(!compile("n!<1") && compile("n<1"));
    TEST(!compile("n<==1") && compile("n<=1"));
    TEST(!compile("n<>1") && compile("n>1"));
    // Incomplete ternary
    TEST(!compile("n==1 ?"));
    TEST(!compile("n==1 ? 1"));
    TEST(!compile("n==1 ? 1 :"));
    TEST(compile("n==1 ? 1 : 0"));
    // Missing closing parenthesis
    TEST(!compile("(n==1") && compile("(n==1)"));
    TEST(!compile("(n + 1") && compile("(n + 1)"));
    TEST(!compile("(n==1 ? 1 : 2") && compile("(n==1 ? 1 : 2)"));
    // Extra closing parenthesis
    TEST(!compile("n==1)") && compile("(n==1)"));
    TEST(!compile("n + 1)") && compile("(n + 1)"));
    TEST(!compile("n==1 ? 1 : 2)") && compile("(n==1 ? 1 : 2)"));
}

void test_main(int /*argc*/, char** /*argv*/)
{
    test_plural_expr();
}
