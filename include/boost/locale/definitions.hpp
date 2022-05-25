//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_DEFINITIONS_HPP_INCLUDED
#define BOOST_LOCALE_DEFINITIONS_HPP_INCLUDED

#include <boost/config.hpp>

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_LOCALE_DYN_LINK)
#   ifdef BOOST_LOCALE_SOURCE
#       define BOOST_LOCALE_DECL BOOST_SYMBOL_EXPORT
#   else
#       define BOOST_LOCALE_DECL BOOST_SYMBOL_IMPORT
#   endif  // BOOST_LOCALE_SOURCE
#else
#   define BOOST_LOCALE_DECL
#endif // BOOST_LOCALE_DYN_LINK

#endif // boost/locale/config.hpp
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

