# Copyright 2020, 2021 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.5...3.20)

project(boost_locale VERSION "${BOOST_SUPERPROJECT_VERSION}" LANGUAGES CXX)

# Using glob here is ok as it is only for headers
file(GLOB_RECURSE headers include/*.hpp)

add_library(boost_locale
  src/encoding/codepage.cpp
  src/encoding/conv.hpp
  src/encoding/iconv_codepage.ipp
  src/encoding/uconv_codepage.ipp
  src/encoding/wconv_codepage.ipp
  src/shared/date_time.cpp
  src/shared/format.cpp
  src/shared/formatting.cpp
  src/shared/generator.cpp
  src/shared/ids.cpp
  src/shared/localization_backend.cpp
  src/shared/message.cpp
  src/shared/mo_lambda.cpp
  src/util/codecvt_converter.cpp
  src/util/default_locale.cpp
  src/util/info.cpp
  src/util/locale_data.cpp
  src/util/locale_data.hpp
  src/util/numeric.hpp
  src/util/timezone.hpp
  ${headers}
)

add_library(Boost::locale ALIAS boost_locale)

target_include_directories(boost_locale PUBLIC include)

target_link_libraries(boost_locale
  PUBLIC
    Boost::assert
    Boost::config
    Boost::function
    Boost::iterator
    Boost::smart_ptr
    Boost::static_assert
    Boost::type_traits
  PRIVATE
    Boost::thread
    Boost::unordered
    Boost::predef
)

target_compile_definitions(boost_locale
  PUBLIC BOOST_LOCALE_NO_LIB
  PRIVATE _CRT_SECURE_NO_WARNINGS
)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(boost_locale PUBLIC BOOST_LOCALE_DYN_LINK)
else()
  target_compile_definitions(boost_locale PUBLIC BOOST_LOCALE_STATIC_LINK)
endif()

# Build Options

find_package(Iconv QUIET) # requires CMake 3.11
find_package(ICU COMPONENTS data i18n uc QUIET)

set(_default_posix OFF)

if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
  set(_default_posix ON)
endif()

option(BOOST_LOCALE_ENABLE_ICONV "Boost.Locale: enable iconv backend" ${ICONV_FOUND})
option(BOOST_LOCALE_ENABLE_ICU "Boost.Locale: enable ICU backend" ${ICU_FOUND})
option(BOOST_LOCALE_ENABLE_POSIX "Boost.Locale: enable POSIX backend" ${_default_posix})
option(BOOST_LOCALE_ENABLE_STD "Boost.Locale: enable std::locale backend" ON)
option(BOOST_LOCALE_ENABLE_WINAPI "Boost.Locale: enable Windows API backend" ${WIN32})

unset(_default_posix)

if(BOOST_LOCALE_ENABLE_ICONV)

  find_package(Iconv REQUIRED)

  target_compile_definitions(boost_locale PRIVATE BOOST_LOCALE_WITH_ICONV=1)
  target_link_libraries(boost_locale PRIVATE Iconv::Iconv)

  target_sources(boost_locale PRIVATE
    src/util/iconv.hpp
  )
endif()

if(BOOST_LOCALE_ENABLE_ICU)

  find_package(ICU COMPONENTS data i18n uc REQUIRED)

  target_compile_definitions(boost_locale PRIVATE BOOST_LOCALE_WITH_ICU=1)
  target_link_libraries(boost_locale PRIVATE ICU::data ICU::i18n ICU::uc)

  target_sources(boost_locale PRIVATE
    src/icu/all_generator.hpp
    src/icu/boundary.cpp
    src/icu/cdata.hpp
    src/icu/codecvt.cpp
    src/icu/codecvt.hpp
    src/icu/collator.cpp
    src/icu/conversion.cpp
    src/icu/date_time.cpp
    src/icu/formatter.cpp
    src/icu/formatter.hpp
    src/icu/icu_backend.cpp
    src/icu/icu_backend.hpp
    src/icu/icu_util.hpp
    src/icu/numeric.cpp
    src/icu/predefined_formatters.hpp
    src/icu/time_zone.cpp
    src/icu/time_zone.hpp
    src/icu/uconv.hpp
  )

endif()

if(BOOST_LOCALE_ENABLE_STD)

  target_sources(boost_locale PRIVATE
    src/std/all_generator.hpp
    src/std/codecvt.cpp
    src/std/collate.cpp
    src/std/converter.cpp
    src/std/numeric.cpp
    src/std/std_backend.cpp
    src/std/std_backend.hpp
  )

else()

  target_compile_definitions(boost_locale PRIVATE BOOST_LOCALE_NO_STD_BACKEND=1)

endif()

if(BOOST_LOCALE_ENABLE_WINAPI)

  target_sources(boost_locale PRIVATE
    src/win32/all_generator.hpp
    src/win32/api.hpp
    src/win32/collate.cpp
    src/win32/converter.cpp
    src/win32/numeric.cpp
    src/win32/win_backend.cpp
    src/win32/win_backend.hpp
  )

else()

  target_compile_definitions(boost_locale PRIVATE BOOST_LOCALE_NO_WINAPI_BACKEND=1)

endif()

if(BOOST_LOCALE_ENABLE_WINAPI OR (BOOST_LOCALE_ENABLE_STD AND WIN32))

  target_sources(boost_locale PRIVATE
    src/win32/lcid.cpp
    src/win32/lcid.hpp
  )

endif()

if(BOOST_LOCALE_ENABLE_POSIX)

  target_sources(boost_locale PRIVATE
    src/posix/all_generator.hpp
    src/posix/codecvt.cpp
    src/posix/codecvt.hpp
    src/posix/collate.cpp
    src/posix/converter.cpp
    src/posix/numeric.cpp
    src/posix/posix_backend.cpp
    src/posix/posix_backend.hpp
  )

else()

  target_compile_definitions(boost_locale PRIVATE BOOST_LOCALE_NO_POSIX_BACKEND=1)

endif()

if(BOOST_LOCALE_ENABLE_WINAPI OR BOOST_LOCALE_ENABLE_STD OR BOOST_LOCALE_ENABLE_POSIX)

  target_sources(boost_locale PRIVATE
    src/util/gregorian.cpp
    src/util/gregorian.hpp
  )

endif()

message(STATUS "Boost.Locale: "
  "iconv ${BOOST_LOCALE_ENABLE_ICONV}, "
  "ICU ${BOOST_LOCALE_ENABLE_ICU}, "
  "POSIX ${BOOST_LOCALE_ENABLE_POSIX}, "
  "std ${BOOST_LOCALE_ENABLE_STD}, "
  "winapi ${BOOST_LOCALE_ENABLE_WINAPI}"
)

if(MSVC)
    set(BOOST_LOCALE_WARNING_OPTIONS /W4)
    if(BOOST_LOCALE_WERROR)
        list(APPEND BOOST_LOCALE_WARNING_OPTIONS /WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(BOOST_LOCALE_WARNING_OPTIONS -Wall -Wextra -Wno-error=deprecated-declarations)
    if(BOOST_LOCALE_WERROR)
        list(APPEND BOOST_LOCALE_WARNING_OPTIONS -Werror)
    endif()
elseif(NOT DEFINED BOOST_LOCALE_WARNING_OPTIONS)
    set(BOOST_LOCALE_WARNING_OPTIONS "")
endif()
target_compile_options(boost_locale PRIVATE ${BOOST_LOCALE_WARNING_OPTIONS})

# Testing

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")

  add_subdirectory(test)
  add_subdirectory(examples)

endif()
