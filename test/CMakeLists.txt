# Copyright 2022 Alexander Grund
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

if(NOT TARGET tests)
  add_custom_target(tests)
endif()

function(boost_locale_add_test name)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "COMPILE_ONLY" "SRC" "")

  if(NOT ARG_SRC)
    set(ARG_SRC ${name}.cpp)
  endif()
  set(name ${PROJECT_NAME}-${name})
  
  add_executable(${name} ${ARG_SRC})
  add_dependencies(tests ${name})
  target_link_libraries(${name} PRIVATE Boost::locale)
  
  if(BOOST_LOCALE_ENABLE_ICONV)
    target_compile_definitions(${name} PRIVATE BOOST_LOCALE_WITH_ICONV=1)
  endif()
  if(BOOST_LOCALE_ENABLE_ICU)
    target_compile_definitions(${name} PRIVATE BOOST_LOCALE_WITH_ICU=1)
  endif()
  if(NOT BOOST_LOCALE_ENABLE_STD)
    target_compile_definitions(${name} PRIVATE BOOST_LOCALE_NO_STD_BACKEND=1)
  endif()
  if(NOT BOOST_LOCALE_ENABLE_WINAPI)
    target_compile_definitions(${name} PRIVATE BOOST_LOCALE_NO_WINAPI_BACKEND=1)
  endif()
  if(NOT BOOST_LOCALE_ENABLE_POSIX)
    target_compile_definitions(${name} PRIVATE BOOST_LOCALE_NO_POSIX_BACKEND=1)
  endif()

  if(NOT ARG_COMPILE_ONLY)
    add_test(NAME ${name} COMMAND ${name} ${ARG_ARGS})
  endif()
endfunction()

boost_locale_add_test(test_config)
# Shared
boost_locale_add_test(test_utf COMPILE_ONLY)
boost_locale_add_test(test_date_time COMPILE_ONLY)
boost_locale_add_test(test_ios_prop COMPILE_ONLY)
boost_locale_add_test(test_codecvt COMPILE_ONLY)
boost_locale_add_test(test_codepage_converter COMPILE_ONLY)
boost_locale_add_test(test_codepage COMPILE_ONLY)
boost_locale_add_test(test_message COMPILE_ONLY)
boost_locale_add_test(test_generator COMPILE_ONLY)
# icu
boost_locale_add_test(test_collate COMPILE_ONLY)
boost_locale_add_test(test_convert COMPILE_ONLY)
boost_locale_add_test(test_boundary COMPILE_ONLY)
boost_locale_add_test(test_formatting COMPILE_ONLY)
boost_locale_add_test(test_icu_vs_os_timezone COMPILE_ONLY)
# winapi
boost_locale_add_test(test_winapi_collate COMPILE_ONLY)
boost_locale_add_test(test_winapi_convert COMPILE_ONLY)
boost_locale_add_test(test_winapi_formatting COMPILE_ONLY)
# posix
boost_locale_add_test(test_posix_collate COMPILE_ONLY)
boost_locale_add_test(test_posix_convert COMPILE_ONLY)
boost_locale_add_test(test_posix_formatting COMPILE_ONLY)
# std
boost_locale_add_test(test_std_collate COMPILE_ONLY)
boost_locale_add_test(test_std_convert COMPILE_ONLY)
boost_locale_add_test(test_std_formatting COMPILE_ONLY)
