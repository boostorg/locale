//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_LOCALE_IMPL_WCONV_CODEPAGE_HPP
#define BOOST_LOCALE_IMPL_WCONV_CODEPAGE_HPP

#include <boost/locale/encoding.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <windows.h>

#include "boost/locale/encoding/conv.hpp"

namespace boost { namespace locale { namespace conv { namespace impl {

    struct windows_encoding {
        const char* name;
        unsigned codepage;
        unsigned was_tested;
    };

    bool operator<(const windows_encoding& l, const windows_encoding& r)
    {
        return strcmp(l.name, r.name) < 0;
    }

    windows_encoding all_windows_encodings[] = {
      {"asmo708", 708, 0},
      {"big5", 950, 0},
      {"cp1025", 21025, 0},
      {"cp1250", 1250, 0},
      {"cp1251", 1251, 0},
      {"cp1252", 1252, 0},
      {"cp1253", 1253, 0},
      {"cp1254", 1254, 0},
      {"cp1255", 1255, 0},
      {"cp1256", 1256, 0},
      {"cp1257", 1257, 0},
      {"cp866", 866, 0},
      {"cp874", 874, 0},
      {"cp875", 875, 0},
      {"cp932", 932, 0},
      {"cp936", 936, 0},
      {"csiso2022jp", 50221, 0},
      {"dos720", 720, 0},
      {"dos862", 862, 0},
      {"euccn", 51936, 0},
      {"eucjp", 20932, 0},
      {"euckr", 51949, 0},
      {"gb18030", 54936, 0},
      {"gb2312", 936, 0},
      {"gbk", 936, 0},
      {"hzgb2312", 52936, 0},
      {"ibm00858", 858, 0},
      {"ibm00924", 20924, 0},
      {"ibm037", 37, 0},
      {"ibm01026", 1026, 0},
      {"ibm01047", 1047, 0},
      {"ibm01140", 1140, 0},
      {"ibm01141", 1141, 0},
      {"ibm01142", 1142, 0},
      {"ibm01143", 1143, 0},
      {"ibm01144", 1144, 0},
      {"ibm01145", 1145, 0},
      {"ibm01146", 1146, 0},
      {"ibm01147", 1147, 0},
      {"ibm01148", 1148, 0},
      {"ibm01149", 1149, 0},
      {"ibm273", 20273, 0},
      {"ibm277", 20277, 0},
      {"ibm278", 20278, 0},
      {"ibm280", 20280, 0},
      {"ibm284", 20284, 0},
      {"ibm285", 20285, 0},
      {"ibm290", 20290, 0},
      {"ibm297", 20297, 0},
      {"ibm420", 20420, 0},
      {"ibm423", 20423, 0},
      {"ibm424", 20424, 0},
      {"ibm437", 437, 0},
      {"ibm500", 500, 0},
      {"ibm737", 737, 0},
      {"ibm775", 775, 0},
      {"ibm850", 850, 0},
      {"ibm852", 852, 0},
      {"ibm855", 855, 0},
      {"ibm857", 857, 0},
      {"ibm860", 860, 0},
      {"ibm861", 861, 0},
      {"ibm863", 863, 0},
      {"ibm864", 864, 0},
      {"ibm865", 865, 0},
      {"ibm869", 869, 0},
      {"ibm870", 870, 0},
      {"ibm871", 20871, 0},
      {"ibm880", 20880, 0},
      {"ibm905", 20905, 0},
      {"ibmthai", 20838, 0},
      {"iso2022jp", 50220, 0},
      {"iso2022jp", 50222, 0},
      {"iso2022kr", 50225, 0},
      {"iso88591", 28591, 0},
      {"iso88591", 28591, 0},
      {"iso885913", 28603, 0},
      {"iso885913", 28603, 0},
      {"iso885915", 28605, 0},
      {"iso885915", 28605, 0},
      {"iso88592", 28592, 0},
      {"iso88592", 28592, 0},
      {"iso88593", 28593, 0},
      {"iso88593", 28593, 0},
      {"iso88594", 28594, 0},
      {"iso88594", 28594, 0},
      {"iso88595", 28595, 0},
      {"iso88595", 28595, 0},
      {"iso88596", 28596, 0},
      {"iso88596", 28596, 0},
      {"iso88597", 28597, 0},
      {"iso88597", 28597, 0},
      {"iso88598", 28598, 0},
      {"iso88598", 28598, 0},
      {"iso88598i", 38598, 0},
      {"iso88599", 28599, 0},
      {"iso88599", 28599, 0},
      {"johab", 1361, 0},
      {"koi8r", 20866, 0},
      {"koi8u", 21866, 0},
      {"koi8u", 21866, 0},
      {"ksc56011987", 949, 0},
      {"macintosh", 10000, 0},
      {"ms936", 936, 0},
      {"shiftjis", 932, 0},
      {"sjis", 932, 0},
      {"unicodefffe", 1201, 0},
      {"usascii", 20127, 0},
      {"usascii", 20127, 0},
      {"utf16", 1200, 0},
      {"utf32", 12000, 0},
      {"utf32be", 12001, 0},
      {"utf7", 65000, 0},
      {"utf8", 65001, 0},
      {"windows1250", 1250, 0},
      {"windows1251", 1251, 0},
      {"windows1252", 1252, 0},
      {"windows1253", 1253, 0},
      {"windows1254", 1254, 0},
      {"windows1255", 1255, 0},
      {"windows1256", 1256, 0},
      {"windows1257", 1257, 0},
      {"windows1258", 1258, 0},
      {"windows874", 874, 0},
      {"windows874", 874, 0},
      {"windows932", 932, 0},
      {"windows936", 936, 0},
      {"xchinesecns", 20000, 0},
      {"xchineseeten", 20002, 0},
      {"xcp20001", 20001, 0},
      {"xcp20003", 20003, 0},
      {"xcp20004", 20004, 0},
      {"xcp20005", 20005, 0},
      {"xcp20261", 20261, 0},
      {"xcp20269", 20269, 0},
      {"xcp20936", 20936, 0},
      {"xcp20949", 20949, 0},
      {"xcp50227", 50227, 0},
      {"xebcdickoreanextended", 20833, 0},
      {"xeuropa", 29001, 0},
      {"xia5", 20105, 0},
      {"xia5german", 20106, 0},
      {"xia5norwegian", 20108, 0},
      {"xia5swedish", 20107, 0},
      {"xisciias", 57006, 0},
      {"xisciibe", 57003, 0},
      {"xisciide", 57002, 0},
      {"xisciigu", 57010, 0},
      {"xisciika", 57008, 0},
      {"xisciima", 57009, 0},
      {"xisciior", 57007, 0},
      {"xisciipa", 57011, 0},
      {"xisciita", 57004, 0},
      {"xisciite", 57005, 0},
      {"xmacarabic", 10004, 0},
      {"xmacce", 10029, 0},
      {"xmacchinesesimp", 10008, 0},
      {"xmacchinesetrad", 10002, 0},
      {"xmaccroatian", 10082, 0},
      {"xmaccyrillic", 10007, 0},
      {"xmacgreek", 10006, 0},
      {"xmachebrew", 10005, 0},
      {"xmacicelandic", 10079, 0},
      {"xmacjapanese", 10001, 0},
      {"xmackorean", 10003, 0},
      {"xmacromanian", 10010, 0},
      {"xmacthai", 10021, 0},
      {"xmacturkish", 10081, 0},
      {"xmacukrainian", 10017, 0},
    };

    size_t remove_substitutions(std::vector<char>& v)
    {
        if(std::find(v.begin(), v.end(), 0) == v.end()) {
            return v.size();
        }
        std::vector<char> v2;
        v2.reserve(v.size());
        for(unsigned i = 0; i < v.size(); i++) {
            if(v[i] != 0)
                v2.push_back(v[i]);
        }
        v.swap(v2);
        return v.size();
    }

    void multibyte_to_wide_one_by_one(int codepage, const char* begin, const char* end, std::vector<wchar_t>& buf)
    {
        buf.reserve(end - begin);
        while(begin != end) {
            wchar_t wide_buf[4];
            int n = 0;
            int len = IsDBCSLeadByteEx(codepage, *begin) ? 2 : 1;
            if(len == 2 && begin + 1 == end)
                return;
            n = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, begin, len, wide_buf, 4);
            for(int i = 0; i < n; i++)
                buf.push_back(wide_buf[i]);
            begin += len;
        }
    }

    void multibyte_to_wide(int codepage, const char* begin, const char* end, bool do_skip, std::vector<wchar_t>& buf)
    {
        if(begin == end)
            return;
        const std::ptrdiff_t num_chars = end - begin;
        if(num_chars > std::numeric_limits<int>::max())
            throw conversion_error();
        int n = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, begin, static_cast<int>(num_chars), 0, 0);
        if(n == 0) {
            if(do_skip) {
                multibyte_to_wide_one_by_one(codepage, begin, end, buf);
                return;
            }
            throw conversion_error();
        }

        buf.resize(n);
        if(MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, begin, static_cast<int>(num_chars), &buf.front(), n)
           == 0)
            throw conversion_error();
    }

    void wide_to_multibyte_non_zero(int codepage,
                                    const wchar_t* begin,
                                    const wchar_t* end,
                                    bool do_skip,
                                    std::vector<char>& buf)
    {
        if(begin == end)
            return;
        BOOL substitute = FALSE;
        BOOL* substitute_ptr = codepage == 65001 || codepage == 65000 ? 0 : &substitute;
        char subst_char = 0;
        char* subst_char_ptr = codepage == 65001 || codepage == 65000 ? 0 : &subst_char;

        const std::ptrdiff_t num_chars = end - begin;
        if(num_chars > std::numeric_limits<int>::max())
            throw conversion_error();
        int n =
          WideCharToMultiByte(codepage, 0, begin, static_cast<int>(num_chars), 0, 0, subst_char_ptr, substitute_ptr);
        buf.resize(n);

        if(WideCharToMultiByte(codepage,
                               0,
                               begin,
                               static_cast<int>(num_chars),
                               &buf[0],
                               n,
                               subst_char_ptr,
                               substitute_ptr)
           == 0)
            throw conversion_error();
        if(substitute) {
            if(do_skip)
                remove_substitutions(buf);
            else
                throw conversion_error();
        }
    }

    void wide_to_multibyte(int codepage, const wchar_t* begin, const wchar_t* end, bool do_skip, std::vector<char>& buf)
    {
        if(begin == end)
            return;
        buf.reserve(end - begin);
        const wchar_t* e = std::find(begin, end, L'\0');
        const wchar_t* b = begin;
        for(;;) {
            std::vector<char> tmp;
            wide_to_multibyte_non_zero(codepage, b, e, do_skip, tmp);
            size_t osize = buf.size();
            buf.resize(osize + tmp.size());
            std::copy(tmp.begin(), tmp.end(), buf.begin() + osize);
            if(e != end) {
                buf.push_back('\0');
                b = e + 1;
                e = std::find(b, end, L'0');
            } else
                break;
        }
    }

    int encoding_to_windows_codepage(const char* ccharset)
    {
        std::string charset = normalize_encoding(ccharset);
        windows_encoding ref;
        ref.name = charset.c_str();
        size_t n = sizeof(all_windows_encodings) / sizeof(all_windows_encodings[0]);
        windows_encoding* begin = all_windows_encodings;
        windows_encoding* end = all_windows_encodings + n;
        windows_encoding* ptr = std::lower_bound(begin, end, ref);
        if(ptr != end && strcmp(ptr->name, charset.c_str()) == 0) {
            if(ptr->was_tested) {
                return ptr->codepage;
            } else if(IsValidCodePage(ptr->codepage)) {
                // the thread safety is not an issue, maximum
                // it would be checked more then once
                ptr->was_tested = 1;
                return ptr->codepage;
            } else {
                return -1;
            }
        }
        return -1;
    }

    template<typename CharType>
    bool validate_utf16(const CharType* str, size_t len)
    {
        const CharType* begin = str;
        const CharType* end = str + len;
        while(begin != end) {
            utf::code_point c = utf::utf_traits<CharType, 2>::template decode<const CharType*>(begin, end);
            if(c == utf::illegal || c == utf::incomplete)
                return false;
        }
        return true;
    }

    template<typename CharType, typename OutChar>
    void clean_invalid_utf16(const CharType* str, size_t len, std::vector<OutChar>& out)
    {
        out.reserve(len);
        for(size_t i = 0; i < len; i++) {
            uint16_t c = static_cast<uint16_t>(str[i]);

            if(0xD800 <= c && c <= 0xDBFF) {
                i++;
                if(i >= len)
                    return;
                uint16_t c2 = static_cast<uint16_t>(str[i]);
                if(0xDC00 <= c2 && c2 <= 0xDFFF) {
                    out.push_back(static_cast<OutChar>(c));
                    out.push_back(static_cast<OutChar>(c2));
                }
            } else if(0xDC00 <= c && c <= 0xDFFF)
                continue;
            else
                out.push_back(static_cast<OutChar>(c));
        }
    }

    class wconv_between : public converter_between {
    public:
        wconv_between() : how_(skip), to_code_page_(-1), from_code_page_(-1) {}
        bool open(const char* to_charset, const char* from_charset, method_type how) override
        {
            how_ = how;
            to_code_page_ = encoding_to_windows_codepage(to_charset);
            from_code_page_ = encoding_to_windows_codepage(from_charset);
            if(to_code_page_ == -1 || from_code_page_ == -1)
                return false;
            return true;
        }
        std::string convert(const char* begin, const char* end) override
        {
            if(to_code_page_ == 65001 && from_code_page_ == 65001)
                return utf_to_utf<char>(begin, end, how_);

            std::string res;

            std::vector<wchar_t> tmp; // buffer for mb2w
            std::wstring tmps;        // buffer for utf_to_utf
            const wchar_t* wbegin = 0;
            const wchar_t* wend = 0;

            if(from_code_page_ == 65001) {
                tmps = utf_to_utf<wchar_t>(begin, end, how_);
                if(tmps.empty())
                    return res;
                wbegin = tmps.c_str();
                wend = wbegin + tmps.size();
            } else {
                multibyte_to_wide(from_code_page_, begin, end, how_ == skip, tmp);
                if(tmp.empty())
                    return res;
                wbegin = &tmp[0];
                wend = wbegin + tmp.size();
            }

            if(to_code_page_ == 65001) {
                return utf_to_utf<char>(wbegin, wend, how_);
            }

            std::vector<char> ctmp;
            wide_to_multibyte(to_code_page_, wbegin, wend, how_ == skip, ctmp);
            if(ctmp.empty())
                return res;
            res.assign(&ctmp.front(), ctmp.size());
            return res;
        }

    private:
        method_type how_;
        int to_code_page_;
        int from_code_page_;
    };

    template<typename CharType, int size = sizeof(CharType)>
    class wconv_to_utf;

    template<typename CharType, int size = sizeof(CharType)>
    class wconv_from_utf;

    template<>
    class wconv_to_utf<char, 1> : public converter_to_utf<char> {
    public:
        bool open(const char* cs, method_type how) override { return cvt.open("UTF-8", cs, how); }
        std::string convert(const char* begin, const char* end) override { return cvt.convert(begin, end); }

    private:
        wconv_between cvt;
    };

    template<>
    class wconv_from_utf<char, 1> : public converter_from_utf<char> {
    public:
        bool open(const char* cs, method_type how) override { return cvt.open(cs, "UTF-8", how); }
        std::string convert(const char* begin, const char* end) override { return cvt.convert(begin, end); }

    private:
        wconv_between cvt;
    };

    template<typename CharType>
    class wconv_to_utf<CharType, 2> : public converter_to_utf<CharType> {
    public:
        typedef CharType char_type;

        typedef std::basic_string<char_type> string_type;

        wconv_to_utf() : how_(skip), code_page_(-1) {}

        bool open(const char* charset, method_type how) override
        {
            how_ = how;
            code_page_ = encoding_to_windows_codepage(charset);
            return code_page_ != -1;
        }

        string_type convert(const char* begin, const char* end) override
        {
            if(code_page_ == 65001) {
                return utf_to_utf<char_type>(begin, end, how_);
            }
            std::vector<wchar_t> tmp;
            multibyte_to_wide(code_page_, begin, end, how_ == skip, tmp);
            string_type res;
            if(!tmp.empty())
                res.assign(reinterpret_cast<char_type*>(&tmp.front()), tmp.size());
            return res;
        }

    private:
        method_type how_;
        int code_page_;
    };

    template<typename CharType>
    class wconv_from_utf<CharType, 2> : public converter_from_utf<CharType> {
    public:
        typedef CharType char_type;

        typedef std::basic_string<char_type> string_type;

        wconv_from_utf() : how_(skip), code_page_(-1) {}

        bool open(const char* charset, method_type how) override
        {
            how_ = how;
            code_page_ = encoding_to_windows_codepage(charset);
            return code_page_ != -1;
        }

        std::string convert(const CharType* begin, const CharType* end) override
        {
            if(code_page_ == 65001) {
                return utf_to_utf<char>(begin, end, how_);
            }
            const wchar_t* wbegin = 0;
            const wchar_t* wend = 0;
            std::vector<wchar_t> buffer; // if needed
            if(begin == end)
                return std::string();
            if(validate_utf16(begin, end - begin)) {
                wbegin = reinterpret_cast<const wchar_t*>(begin);
                wend = reinterpret_cast<const wchar_t*>(end);
            } else {
                if(how_ == stop) {
                    throw conversion_error();
                } else {
                    clean_invalid_utf16(begin, end - begin, buffer);
                    if(!buffer.empty()) {
                        wbegin = &buffer[0];
                        wend = wbegin + buffer.size();
                    }
                }
            }
            std::string res;
            if(wbegin == wend)
                return res;
            std::vector<char> ctmp;
            wide_to_multibyte(code_page_, wbegin, wend, how_ == skip, ctmp);
            if(ctmp.empty())
                return res;
            res.assign(&ctmp.front(), ctmp.size());
            return res;
        }

    private:
        method_type how_;
        int code_page_;
    };

    template<typename CharType>
    class wconv_to_utf<CharType, 4> : public converter_to_utf<CharType> {
    public:
        typedef CharType char_type;

        typedef std::basic_string<char_type> string_type;

        wconv_to_utf() : how_(skip), code_page_(-1) {}

        bool open(const char* charset, method_type how) override
        {
            how_ = how;
            code_page_ = encoding_to_windows_codepage(charset);
            return code_page_ != -1;
        }

        string_type convert(const char* begin, const char* end) override
        {
            if(code_page_ == 65001) {
                return utf_to_utf<char_type>(begin, end, how_);
            }
            std::vector<wchar_t> buf;
            multibyte_to_wide(code_page_, begin, end, how_ == skip, buf);

            if(buf.empty())
                return string_type();

            return utf_to_utf<CharType>(&buf[0], &buf[0] + buf.size(), how_);
        }

    private:
        method_type how_;
        int code_page_;
    };

    template<typename CharType>
    class wconv_from_utf<CharType, 4> : public converter_from_utf<CharType> {
    public:
        typedef CharType char_type;

        typedef std::basic_string<char_type> string_type;

        wconv_from_utf() : how_(skip), code_page_(-1) {}

        bool open(const char* charset, method_type how) override
        {
            how_ = how;
            code_page_ = encoding_to_windows_codepage(charset);
            return code_page_ != -1;
        }

        std::string convert(const CharType* begin, const CharType* end) override
        {
            if(code_page_ == 65001) {
                return utf_to_utf<char>(begin, end, how_);
            }
            std::wstring tmp = utf_to_utf<wchar_t>(begin, end, how_);

            std::vector<char> ctmp;
            wide_to_multibyte(code_page_, tmp.c_str(), tmp.c_str() + tmp.size(), how_ == skip, ctmp);
            std::string res;
            if(ctmp.empty())
                return res;
            res.assign(&ctmp.front(), ctmp.size());
            return res;
        }

    private:
        method_type how_;
        int code_page_;
    };

}}}} // namespace boost::locale::conv::impl

// boostinspect:nominmax
#endif
