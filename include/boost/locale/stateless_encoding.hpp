//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_ENCODING_HPP_INCLUDED
#define BOOST_LOCALE_ENCODING_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <boost/locale/info.hpp>
#include <boost/locale/encoding_errors.hpp>
#include <boost/locale/encoding_utf.hpp>



namespace boost {
namespace locale {
namespace conv {

    namespace tables {
        struct single_byte_ascii_compatible {
            static const int max_width = 1;
            
            unsigned short to_unicode[128];
            char from_unicode[256];


            template<typename Iterator>
            code_point decode(Iterator &p,Iterator e) const
            {
                if(p==e)
                    return incomplate;
                unsigned char v = *p++;
                if(v < 128)
                    return v;
                code_point value = to_unicode[v-128];
                if(value == 0)
                    return illegal;
                return value;
            }

            template<typename Iterator>
            Iterator encode(code_point value,Iterator out) const
            {
                if(value < 128) {
                    *out++  = value;
                    return out;
                }
                else {
                    unsigned pos = value & 0xFF;
                    unsigned char res;
                    while((res = from_unicode[pos]) != 0 && to_unicode[res] != value)
                        pos = (pos + 1) % 0xFF;
                    if(res == 0)
                        return illegal;
                    return res;
                }
            }
        };

        struct single_byte_ascii_incompatible {
            static const int max_width = 1;
            
            unsigned short to_unicode[256];
            char from_unicode[512];


            template<typename Iterator>
            code_point decode(Iterator &p,Iterator e) const
            {
                if(p==e)
                    return incomplate;
                unsigned char v = *p++;
                if(v==0)
                    return v;
                code_point value = to_unicode[v];
                if(value == 0)
                    return illegal;
                return value;
            }

            template<typename Iterator>
            bool encode(code_point value,Iterator &out) const
            {
                if(value == 0) {
                    *out++  = value;
                    return true;
                }
                else {
                    unsigned pos = value & 0x1FF;
                    unsigned char res;
                    while((res = from_unicode[pos]) != 0 && to_unicode[res] != value)
                        pos = (pos + 1) % 0x1FF;
                    if(res == 0)
                        return false;
                    *out++ = res;
                    return true;
                }
            }
        };
        struct double_byte_ascii_compatible {
            static const int max_width = 2;
            unsigned short to_unicode[128];
            unsigned short *to_unicode_by_first[128];
            unsigned short *from_unicode_by_first[256];
            
            template<typename Iterator>
            code_point decode(Iterator &p,Iterator e) const
            {
                if(p==e)
                    return incomplate;
                unsigned char v = *p++;
                if(v < 128)
                    return v;
                code_point value = to_unicode[v-128];
                if(value == 0)
                    return illegal;
                if(value < 128)
                    return value;
                unsigned short const *second_ptr = to_unicode_by_first[value - 128];
                if(second_ptr == 0)
                    return illegal;
                if(p==e)
                    return incomplete;
                unsigned char v2 = *p++;
                value = second_ptr[v2];
                if(value == 0)
                    return illegal;
                return value;
            }
            template<typename Iterator>
            Iterator encode(code_point value,Iterator out) const
            {
                if(value < 128) {
                    *out++  = value;
                    return out;
                }
                else {
                    unsigned msb = value >> 8;
                    unsigned short const *tbl = from_unicode_by_first[msb];
                    if(!tbl)
                        return 
                    unsigned pos = value % from_table_size;
                    unsigned char res;
                    while((res = from_unicode[pos]) != 0 && to_unicode[res] != value)
                        pos = (pos + 1) % 0x1FF;
                    if(res == 0)
                        return illegal;
                    return res;
                }
            }

        };
    } // tables
    

} // conv

} // locale
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

