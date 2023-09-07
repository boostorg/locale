//
// Copyright (c) 2023 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <locale>
int main()
{
    std::locale l(std::locale(), new std::codecvt<char8_t, char, std::mbstate_t>());
    (void)l;
}
