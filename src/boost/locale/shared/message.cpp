//
// Copyright (c) 2009-2015 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define BOOST_LOCALE_SOURCE
#define BOOST_DETAIL_NO_CONTAINER_FWD

// Need _wfopen which is an extension on MinGW but not on MinGW-w64
// So remove the strict-mode define on (only) MinGW before including anything
#if defined(__MINGW32__) && defined(__STRICT_ANSI__)
#    include <_mingw.h>
#    ifndef __MINGW64_VERSION_MAJOR
#        undef __STRICT_ANSI__
#    endif
#endif

#include <boost/locale/encoding.hpp>
#include <boost/locale/gnu_gettext.hpp>
#include <boost/locale/hold_ptr.hpp>
#include <boost/locale/message.hpp>
#include <boost/version.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "boost/locale/shared/mo_hash.hpp"
#include "boost/locale/shared/mo_lambda.hpp"

#ifdef BOOST_MSVC
#    pragma warning(disable : 4996)
#endif

namespace boost { namespace locale { namespace gnu_gettext {

    class c_file {
        c_file(const c_file&);
        void operator=(const c_file&);

    public:
        FILE* file;

        c_file() : file(0) {}
        ~c_file() { close(); }

        void close()
        {
            if(file) {
                fclose(file);
                file = 0;
            }
        }

#if defined(BOOST_WINDOWS)

        bool open(const std::string& file_name, const std::string& encoding)
        {
            close();

            //
            // Under windows we have to use "_wfopen" to get
            // access to path's with Unicode in them
            //
            // As not all standard C++ libraries support nonstandard std::istream::open(wchar_t const *)
            // we would use old and good stdio and _wfopen CRTL functions
            //

            std::wstring wfile_name = conv::to_utf<wchar_t>(file_name, encoding);
            file = _wfopen(wfile_name.c_str(), L"rb");

            return file != 0;
        }

#else // POSIX systems do not have all this Wide API crap, as native codepages are UTF-8

        // We do not use encoding as we use native file name encoding

        bool open(const std::string& file_name, const std::string& /* encoding */)
        {
            close();

            file = fopen(file_name.c_str(), "rb");

            return file != 0;
        }

#endif
    };

    class mo_file {
    public:
        typedef std::pair<const char*, const char*> pair_type;

        mo_file(std::vector<char>& file) : native_byteorder_(true), size_(0)
        {
            load_file(file);
            init();
        }

        mo_file(FILE* file) : native_byteorder_(true), size_(0)
        {
            load_file(file);
            init();
        }

        pair_type find(const char* context_in, const char* key_in) const
        {
            pair_type null_pair((const char*)0, (const char*)0);
            if(hash_size_ == 0)
                return null_pair;
            uint32_t hkey = 0;
            if(context_in == 0)
                hkey = pj_winberger_hash_function(key_in);
            else {
                pj_winberger_hash::state_type st = pj_winberger_hash::initial_state;
                st = pj_winberger_hash::update_state(st, context_in);
                st = pj_winberger_hash::update_state(st, '\4'); // EOT
                st = pj_winberger_hash::update_state(st, key_in);
                hkey = st;
            }
            uint32_t incr = 1 + hkey % (hash_size_ - 2);
            hkey %= hash_size_;
            uint32_t orig = hkey;

            do {
                uint32_t idx = get(hash_offset_ + 4 * hkey);
                /// Not found
                if(idx == 0)
                    return null_pair;
                /// If equal values return translation
                if(key_equals(key(idx - 1), context_in, key_in))
                    return value(idx - 1);
                /// Rehash
                hkey = (hkey + incr) % hash_size_;
            } while(hkey != orig);
            return null_pair;
        }

        static bool key_equals(const char* real_key, const char* cntx, const char* key)
        {
            if(cntx == 0)
                return strcmp(real_key, key) == 0;
            else {
                size_t real_len = strlen(real_key);
                size_t cntx_len = strlen(cntx);
                size_t key_len = strlen(key);
                if(cntx_len + 1 + key_len != real_len)
                    return false;
                return memcmp(real_key, cntx, cntx_len) == 0 && real_key[cntx_len] == '\4'
                       && memcmp(real_key + cntx_len + 1, key, key_len) == 0;
            }
        }

        const char* key(int id) const
        {
            uint32_t off = get(keys_offset_ + id * 8 + 4);
            return data_ + off;
        }

        pair_type value(int id) const
        {
            uint32_t len = get(translations_offset_ + id * 8);
            uint32_t off = get(translations_offset_ + id * 8 + 4);
            if(off >= file_size_ || off + len >= file_size_)
                throw std::runtime_error("Bad mo-file format");
            return pair_type(&data_[off], &data_[off] + len);
        }

        bool has_hash() const { return hash_size_ != 0; }

        size_t size() const { return size_; }

        bool empty() { return size_ == 0; }

    private:
        void init()
        {
            // Read all format sizes
            size_ = get(8);
            keys_offset_ = get(12);
            translations_offset_ = get(16);
            hash_size_ = get(20);
            hash_offset_ = get(24);
        }

        void load_file(std::vector<char>& data)
        {
            vdata_.swap(data);
            file_size_ = vdata_.size();
            data_ = &vdata_[0];
            if(file_size_ < 4)
                throw std::runtime_error("invalid 'mo' file format - the file is too short");
            uint32_t magic = 0;
            memcpy(&magic, data_, 4);
            if(magic == 0x950412de)
                native_byteorder_ = true;
            else if(magic == 0xde120495)
                native_byteorder_ = false;
            else
                throw std::runtime_error("Invalid file format - invalid magic number");
        }

        void load_file(FILE* file)
        {
            uint32_t magic = 0;
            // if the size is wrong magic would be wrong
            // ok to ingnore fread result
            size_t four_bytes = fread(&magic, 4, 1, file);
            (void)four_bytes; // shut GCC

            if(magic == 0x950412de)
                native_byteorder_ = true;
            else if(magic == 0xde120495)
                native_byteorder_ = false;
            else
                throw std::runtime_error("Invalid file format");

            fseek(file, 0, SEEK_END);
            long len = ftell(file);
            if(len < 0) {
                throw std::runtime_error("Wrong file object");
            }
            fseek(file, 0, SEEK_SET);
            vdata_.resize(len + 1, 0); // +1 to make sure the vector is not empty
            if(fread(&vdata_.front(), 1, len, file) != unsigned(len))
                throw std::runtime_error("Failed to read file");
            data_ = &vdata_[0];
            file_size_ = len;
        }

        uint32_t get(unsigned offset) const
        {
            uint32_t tmp;
            if(offset > file_size_ - 4) {
                throw std::runtime_error("Bad mo-file format");
            }
            memcpy(&tmp, data_ + offset, 4);
            convert(tmp);
            return tmp;
        }

        void convert(uint32_t& v) const
        {
            if(native_byteorder_)
                return;
            v = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
        }

        uint32_t keys_offset_;
        uint32_t translations_offset_;
        uint32_t hash_size_;
        uint32_t hash_offset_;

        const char* data_;
        size_t file_size_;
        std::vector<char> vdata_;
        bool native_byteorder_;
        size_t size_;
    };

    template<typename CharType>
    struct mo_file_use_traits {
        static const bool in_use = false;
        typedef CharType char_type;
        typedef std::pair<const char_type*, const char_type*> pair_type;
        static pair_type use(const mo_file& /*mo*/, const char_type* /*context*/, const char_type* /*key*/)
        {
            return pair_type((const char_type*)(0), (const char_type*)(0));
        }
    };

    template<>
    struct mo_file_use_traits<char> {
        static const bool in_use = true;
        typedef char char_type;
        typedef std::pair<const char_type*, const char_type*> pair_type;
        static pair_type use(const mo_file& mo, const char* context, const char* key) { return mo.find(context, key); }
    };

    template<typename CharType>
    class converter {
    public:
        converter(std::string /*out_enc*/, std::string in_enc) : in_(in_enc) {}

        std::basic_string<CharType> operator()(const char* begin, const char* end)
        {
            return conv::to_utf<CharType>(begin, end, in_, conv::stop);
        }

    private:
        std::string in_;
    };

    template<>
    class converter<char> {
    public:
        converter(std::string out_enc, std::string in_enc) : out_(out_enc), in_(in_enc) {}

        std::string operator()(const char* begin, const char* end)
        {
            return conv::between(begin, end, out_, in_, conv::stop);
        }

    private:
        std::string out_, in_;
    };

    template<typename CharType>
    struct message_key {
        typedef CharType char_type;
        typedef std::basic_string<char_type> string_type;

        message_key(const string_type& c = string_type()) : c_context_(0), c_key_(0)
        {
            size_t pos = c.find(char_type(4));
            if(pos == string_type::npos) {
                key_ = c;
            } else {
                context_ = c.substr(0, pos);
                key_ = c.substr(pos + 1);
            }
        }
        message_key(const char_type* c, const char_type* k) : c_key_(k)
        {
            static const char_type empty = 0;
            if(c != 0)
                c_context_ = c;
            else
                c_context_ = &empty;
        }
        bool operator<(const message_key& other) const
        {
            int cc = compare(context(), other.context());
            if(cc != 0)
                return cc < 0;
            return compare(key(), other.key()) < 0;
        }
        bool operator==(const message_key& other) const
        {
            return compare(context(), other.context()) == 0 && compare(key(), other.key()) == 0;
        }
        bool operator!=(const message_key& other) const { return !(*this == other); }
        const char_type* context() const
        {
            if(c_context_)
                return c_context_;
            return context_.c_str();
        }
        const char_type* key() const
        {
            if(c_key_)
                return c_key_;
            return key_.c_str();
        }

    private:
        static int compare(const char_type* l, const char_type* r)
        {
            typedef std::char_traits<char_type> traits_type;
            for(;;) {
                char_type cl = *l++;
                char_type cr = *r++;
                if(cl == 0 && cr == 0)
                    return 0;
                if(traits_type::lt(cl, cr))
                    return -1;
                if(traits_type::lt(cr, cl))
                    return 1;
            }
        }
        string_type context_;
        string_type key_;
        const char_type* c_context_;
        const char_type* c_key_;
    };

    template<typename CharType>
    struct hash_function {
        size_t operator()(const message_key<CharType>& msg) const
        {
            pj_winberger_hash::state_type state = pj_winberger_hash::initial_state;
            const CharType* p = msg.context();
            if(*p != 0) {
                const CharType* e = p;
                while(*e)
                    e++;
                state = pj_winberger_hash::update_state(state,
                                                        reinterpret_cast<const char*>(p),
                                                        reinterpret_cast<const char*>(e));
                state = pj_winberger_hash::update_state(state, '\4');
            }
            p = msg.key();
            const CharType* e = p;
            while(*e)
                e++;
            state = pj_winberger_hash::update_state(state,
                                                    reinterpret_cast<const char*>(p),
                                                    reinterpret_cast<const char*>(e));
            return state;
        }
    };

    // By default for wide types the conversion is not required
    template<typename CharType>
    const CharType* runtime_conversion(const CharType* msg,
                                       std::basic_string<CharType>& /*buffer*/,
                                       bool /*do_conversion*/,
                                       const std::string& /*locale_encoding*/,
                                       const std::string& /*key_encoding*/)
    {
        return msg;
    }

    // But still need to specialize for char
    template<>
    const char* runtime_conversion(const char* msg,
                                   std::string& buffer,
                                   bool do_conversion,
                                   const std::string& locale_encoding,
                                   const std::string& key_encoding)
    {
        if(!do_conversion)
            return msg;
        if(detail::is_us_ascii_string(msg))
            return msg;
        std::string tmp = conv::between(msg, locale_encoding, key_encoding, conv::skip);
        buffer.swap(tmp);
        return buffer.c_str();
    }

    template<typename CharType>
    class mo_message : public message_format<CharType> {
        typedef CharType char_type;
        typedef std::basic_string<CharType> string_type;
        typedef message_key<CharType> key_type;
        typedef std::unordered_map<key_type, string_type, hash_function<CharType>> catalog_type;
        typedef std::vector<catalog_type> catalogs_set_type;
        typedef std::map<std::string, int> domains_map_type;

    public:
        typedef std::pair<const CharType*, const CharType*> pair_type;

        const char_type* get(int domain_id, const char_type* context, const char_type* in_id) const override
        {
            return get_string(domain_id, context, in_id).first;
        }

        const char_type* get(int domain_id, const char_type* context, const char_type* single_id, int n) const override
        {
            pair_type ptr = get_string(domain_id, context, single_id);
            if(!ptr.first)
                return 0;
            int form = 0;
            if(plural_forms_.at(domain_id))
                form = (*plural_forms_[domain_id])(n);
            else
                form = n == 1 ? 0 : 1; // Fallback to English plural form

            const CharType* p = ptr.first;
            for(int i = 0; p < ptr.second && i < form; i++) {
                p = std::find(p, ptr.second, 0);
                if(p == ptr.second)
                    return 0;
                ++p;
            }
            if(p >= ptr.second)
                return 0;
            return p;
        }

        int domain(const std::string& domain) const override
        {
            domains_map_type::const_iterator p = domains_.find(domain);
            if(p == domains_.end())
                return -1;
            return p->second;
        }

        mo_message(const messages_info& inf) : key_conversion_required_(false)
        {
            std::string language = inf.language;
            std::string variant = inf.variant;
            std::string country = inf.country;
            std::string encoding = inf.encoding;
            std::string lc_cat = inf.locale_category;
            const std::vector<messages_info::domain>& domains = inf.domains;
            const std::vector<std::string>& search_paths = inf.paths;

            //
            // List of fallbacks: en_US@euro, en@euro, en_US, en.
            //
            std::vector<std::string> paths;

            if(!variant.empty() && !country.empty())
                paths.push_back(language + "_" + country + "@" + variant);

            if(!variant.empty())
                paths.push_back(language + "@" + variant);

            if(!country.empty())
                paths.push_back(language + "_" + country);

            paths.push_back(language);

            catalogs_.resize(domains.size());
            mo_catalogs_.resize(domains.size());
            plural_forms_.resize(domains.size());

            for(unsigned i = 0; i < domains.size(); i++) {
                std::string domain = domains[i].name;
                std::string key_encoding = domains[i].encoding;
                domains_[domain] = i;

                bool found = false;
                for(unsigned j = 0; !found && j < paths.size(); j++) {
                    for(unsigned k = 0; !found && k < search_paths.size(); k++) {
                        std::string full_path = search_paths[k] + "/" + paths[j] + "/" + lc_cat + "/" + domain + ".mo";
                        found = load_file(full_path, encoding, key_encoding, i, inf.callback);
                    }
                }
            }
        }

        const char_type* convert(const char_type* msg, string_type& buffer) const override
        {
            return runtime_conversion<char_type>(msg,
                                                 buffer,
                                                 key_conversion_required_,
                                                 locale_encoding_,
                                                 key_encoding_);
        }

    private:
        int compare_encodings(const std::string& left, const std::string& right)
        {
            return convert_encoding_name(left).compare(convert_encoding_name(right));
        }

        std::string convert_encoding_name(const std::string& in)
        {
            std::string result;
            for(unsigned i = 0; i < in.size(); i++) {
                char c = in[i];
                if('A' <= c && c <= 'Z')
                    c = c - 'A' + 'a';
                else if(('a' <= c && c <= 'z') || ('0' <= c && c <= '9'))
                    ;
                else
                    continue;
                result += c;
            }
            return result;
        }

        bool load_file(const std::string& file_name,
                       const std::string& locale_encoding,
                       const std::string& key_encoding,
                       int idx,
                       const messages_info::callback_type& callback)
        {
            locale_encoding_ = locale_encoding;
            key_encoding_ = key_encoding;

            key_conversion_required_ = sizeof(CharType) == 1 && compare_encodings(locale_encoding, key_encoding) != 0;

            std::shared_ptr<mo_file> mo;

            if(callback) {
                std::vector<char> vfile = callback(file_name, locale_encoding);
                if(vfile.empty())
                    return false;
                mo.reset(new mo_file(vfile));
            } else {
                c_file the_file;
                the_file.open(file_name, locale_encoding);
                if(!the_file.file)
                    return false;
                mo.reset(new mo_file(the_file.file));
            }

            std::string plural = extract(mo->value(0).first, "plural=", "\r\n;");

            std::string mo_encoding = extract(mo->value(0).first, "charset=", " \r\n;");

            if(mo_encoding.empty())
                throw std::runtime_error("Invalid mo-format, encoding is not specified");

            if(!plural.empty())
                plural_forms_[idx] = lambda::compile(plural.c_str());

            if(mo_useable_directly(mo_encoding, *mo))
                mo_catalogs_[idx] = mo;
            else {
                converter<CharType> cvt_value(locale_encoding, mo_encoding);
                converter<CharType> cvt_key(key_encoding, mo_encoding);
                for(unsigned i = 0; i < mo->size(); i++) {
                    const char* ckey = mo->key(i);
                    string_type skey = cvt_key(ckey, ckey + strlen(ckey));
                    key_type key(skey);

                    mo_file::pair_type tmp = mo->value(i);
                    string_type value = cvt_value(tmp.first, tmp.second);
                    catalogs_[idx][key].swap(value);
                }
            }
            return true;
        }

        // Check if the mo file as-is is useful
        // 1. It is char and not wide character
        // 2. The locale encoding and mo encoding is same
        // 3. The source strings encoding and mo encoding is same or all
        //    mo key strings are US-ASCII
        bool mo_useable_directly(const std::string& mo_encoding, const mo_file& mo)
        {
            BOOST_LOCALE_START_CONST_CONDITION
            if(sizeof(CharType) != 1)
                return false;
            BOOST_LOCALE_END_CONST_CONDITION
            if(!mo.has_hash())
                return false;
            if(compare_encodings(mo_encoding, locale_encoding_) != 0)
                return false;
            if(compare_encodings(mo_encoding, key_encoding_) == 0) {
                return true;
            }
            for(unsigned i = 0; i < mo.size(); i++) {
                if(!detail::is_us_ascii_string(mo.key(i))) {
                    return false;
                }
            }
            return true;
        }

        static std::string extract(const std::string& meta, const std::string& key, const char* separator)
        {
            size_t pos = meta.find(key);
            if(pos == std::string::npos)
                return "";
            pos += key.size(); /// size of charset=
            size_t end_pos = meta.find_first_of(separator, pos);
            return meta.substr(pos, end_pos - pos);
        }

        pair_type get_string(int domain_id, const char_type* context, const char_type* in_id) const
        {
            pair_type null_pair((const CharType*)0, (const CharType*)0);
            if(domain_id < 0 || size_t(domain_id) >= catalogs_.size())
                return null_pair;
            BOOST_LOCALE_START_CONST_CONDITION
            if(mo_file_use_traits<char_type>::in_use && mo_catalogs_[domain_id]) {
                BOOST_LOCALE_END_CONST_CONDITION
                return mo_file_use_traits<char_type>::use(*mo_catalogs_[domain_id], context, in_id);
            } else {
                key_type key(context, in_id);
                const catalog_type& cat = catalogs_[domain_id];
                typename catalog_type::const_iterator p = cat.find(key);
                if(p == cat.end()) {
                    return null_pair;
                }
                return pair_type(p->second.data(), p->second.data() + p->second.size());
            }
        }

        catalogs_set_type catalogs_;
        std::vector<std::shared_ptr<mo_file>> mo_catalogs_;
        std::vector<std::shared_ptr<lambda::plural>> plural_forms_;
        domains_map_type domains_;

        std::string locale_encoding_;
        std::string key_encoding_;
        bool key_conversion_required_;
    };

    template<>
    message_format<char>* create_messages_facet(const messages_info& info)
    {
        return new mo_message<char>(info);
    }

    template<>
    message_format<wchar_t>* create_messages_facet(const messages_info& info)
    {
        return new mo_message<wchar_t>(info);
    }

#ifdef BOOST_LOCALE_ENABLE_CHAR16_T

    template<>
    message_format<char16_t>* create_messages_facet(const messages_info& info)
    {
        return new mo_message<char16_t>(info);
    }
#endif

#ifdef BOOST_LOCALE_ENABLE_CHAR32_T

    template<>
    message_format<char32_t>* create_messages_facet(const messages_info& info)
    {
        return new mo_message<char32_t>(info);
    }
#endif

}}} // namespace boost::locale::gnu_gettext
