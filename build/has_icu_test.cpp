// Copyright (c) 2010 John Maddock
// Copyright (c) 2024 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <unicode/coll.h>
#include <unicode/locid.h>
#include <unicode/uchar.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>

// Sanity check to avoid false negatives for version checks
#ifndef U_ICU_VERSION_MAJOR_NUM
#    error "U_ICU_VERSION_MAJOR_NUM is missing"
#endif
#ifndef U_ICU_VERSION_MINOR_NUM
#    error "U_ICU_VERSION_MINOR_NUM is missing"
#endif
#ifndef U_ICU_VERSION_PATCHLEVEL_NUM
#    error "U_ICU_VERSION_PATCHLEVEL_NUM is missing"
#endif

#if(U_ICU_VERSION_MAJOR_NUM == 50) && (U_ICU_VERSION_MINOR_NUM == 1) && (U_ICU_VERSION_PATCHLEVEL_NUM < 1)
// https://unicode-org.atlassian.net/browse/ICU-9780
#    error "ICU 50.1.0 has a bug with integer parsing and cannot be used reliably"
#endif

int main()
{
    icu::Locale loc;
    UErrorCode err = U_ZERO_ERROR;
    UChar32 c = ::u_charFromName(U_UNICODE_CHAR_NAME, "GREEK SMALL LETTER ALPHA", &err);
    return err;
}
