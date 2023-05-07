//
// Copyright (c) 2009-2015 Artyom Beilis (Tonkikh)
// Copyright (c) 2021-2023 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

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
#include "boost/locale/shared/mo_hash.hpp"
#include "boost/locale/shared/mo_lambda.hpp"
#include "boost/locale/util/encoding.hpp"
#include <boost/assert.hpp>
#include <boost/version.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace boost { namespace locale { namespace gnu_gettext {

    std::vector<std::string> messages_info::get_lang_folders() const
    {
        // List of fallbacks: en_US@euro, en@euro, en_US, en.
        std::vector<std::string> result;
        if(!language.empty()) {
            if(!variant.empty() && !country.empty())
                result.push_back(language + "_" + country + "@" + variant);

            if(!variant.empty())
                result.push_back(language + "@" + variant);

            if(!country.empty())
                result.push_back(language + "_" + country);

            result.push_back(language);
        }
        return result;
    }

    std::vector<std::string> messages_info::get_catalog_paths() const
    {
        const auto lang_folders = get_lang_folders();
        std::vector<std::string> result;
        result.reserve(lang_folders.size() * paths.size());
        for(const std::string& lang_folder : lang_folders) {
            for(const std::string& search_path : paths)
                result.push_back(search_path + "/" + lang_folder + "/" + locale_category);
        }
        return result;
    }

    class c_file {
    public:
        FILE* handle;

        c_file(const c_file&) = delete;
        void operator=(const c_file&) = delete;

        ~c_file()
        {
            if(handle)
                fclose(handle);
        }

#if defined(BOOST_WINDOWS)

        c_file(const std::string& file_name, const std::string& encoding)
        {
            // Under windows we have to use "_wfopen" to get access to path's with Unicode in them
            //
            // As not all standard C++ libraries support nonstandard std::istream::open(wchar_t const *)
            // we would use old and good stdio and _wfopen CRTL functions

            std::wstring wfile_name = conv::to_utf<wchar_t>(file_name, encoding);
            handle = _wfopen(wfile_name.c_str(), L"rb");
        }

#else // POSIX systems do not have all this Wide API crap, as native codepages are UTF-8

        // We do not use encoding as we use native file name encoding

        c_file(const std::string& file_name, const std::string& /* encoding */) : handle(fopen(file_name.c_str(), "rb"))
        {}
#endif
    };

    std::vector<char> read_file(FILE* file)
    {
        fseek(file, 0, SEEK_END);
        const auto len = ftell(file);
        if(BOOST_UNLIKELY(len < 0))
            throw std::runtime_error("Wrong file object"); // LCOV_EXCL_LINE
        else {
            fseek(file, 0, SEEK_SET);
            std::vector<char> data(len);
            if(BOOST_LIKELY(!data.empty()) && fread(&data.front(), 1, data.size(), file) != data.size())
                throw std::runtime_error("Failed to read file"); // LCOV_EXCL_LINE
            return data;
        }
    }

    class mo_file {
    public:
        typedef std::pair<const char*, const char*> pair_type;

        mo_file(std::vector<char> data) : data_(std::move(data))
        {
            if(data_.size() < 4)
                throw std::runtime_error("invalid 'mo' file format - the file is too short");
            uint32_t magic;
            static_assert(sizeof(magic) == 4, "!");
            memcpy(&magic, data_.data(), sizeof(magic));
            if(magic == 0x950412de)
                native_byteorder_ = true;
            else if(magic == 0xde120495)
                native_byteorder_ = false;
            else
                throw std::runtime_error("Invalid file format - invalid magic number");

            // Read all format sizes
            size_ = get(8);
            keys_offset_ = get(12);
            translations_offset_ = get(16);
            hash_size_ = get(20);
            hash_offset_ = get(24);
        }

        pair_type find(const char* context_in, const char* key_in) const
        {
            pair_type null_pair;
            if(!has_hash())
                return null_pair;

            pj_winberger_hash::state_type st = pj_winberger_hash::initial_state;
            if(context_in) {
                st = pj_winberger_hash::update_state(st, context_in);
                st = pj_winberger_hash::update_state(st, '\4'); // EOT
            }
            st = pj_winberger_hash::update_state(st, key_in);
            uint32_t hkey = st;
            const uint32_t incr = 1 + hkey % (hash_size_ - 2);
            hkey %= hash_size_;
            const uint32_t orig_hkey = hkey;

            do {
                const uint32_t idx = get(hash_offset_ + 4 * hkey);
                // Not found
                if(idx == 0)
                    return null_pair;
                // If equal values return translation
                if(key_equals(key(idx - 1), context_in, key_in))
                    return value(idx - 1);
                // Rehash
                hkey = (hkey + incr) % hash_size_;
            } while(hkey != orig_hkey);
            return null_pair;
        }

        static bool key_equals(const char* real_key, const char* cntx, const char* key)
        {
            if(!cntx)
                return strcmp(real_key, key) == 0;
            else {
                const size_t real_key_len = strlen(real_key);
                const size_t cntx_len = strlen(cntx);
                const size_t key_len = strlen(key);
                if(cntx_len + 1 + key_len != real_key_len)
                    return false;
                return memcmp(real_key, cntx, cntx_len) == 0 && real_key[cntx_len] == '\4'
                       && memcmp(real_key + cntx_len + 1, key, key_len) == 0;
            }
        }

        const char* key(unsigned id) const
        {
            const uint32_t off = get(keys_offset_ + id * 8 + 4);
            return data_.data() + off;
        }

        pair_type value(unsigned id) const
        {
            const uint32_t len = get(translations_offset_ + id * 8);
            const uint32_t off = get(translations_offset_ + id * 8 + 4);
            if(len > data_.size() || off > data_.size() - len)
                throw std::runtime_error("Bad mo-file format");
            return pair_type(&data_[off], &data_[off] + len);
        }

        bool has_hash() const { return hash_size_ != 0; }

        size_t size() const { return size_; }

        bool empty() { return size_ == 0; }

    private:
        uint32_t get(unsigned offset) const
        {
            if(offset > data_.size() - 4)
                throw std::runtime_error("Bad mo-file format");
            uint32_t v;
            memcpy(&v, &data_[offset], 4);
            if(!native_byteorder_)
                v = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);

            return v;
        }

        uint32_t keys_offset_;
        uint32_t translations_offset_;
        uint32_t hash_size_;
        uint32_t hash_offset_;

        const std::vector<char> data_;
        bool native_byteorder_;
        size_t size_;
    };

    template<typename CharType>
    struct mo_file_use_traits {
        static constexpr bool in_use = false;
        typedef std::pair<const CharType*, const CharType*> pair_type;
        static pair_type use(const mo_file&, const CharType*, const CharType*)
        {
            throw std::logic_error("Unexpected call"); // LCOV_EXCL_LINE
        }
    };

    template<>
    struct mo_file_use_traits<char> {
        static constexpr bool in_use = true;
        typedef char CharType;
        typedef std::pair<const CharType*, const CharType*> pair_type;
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
        typedef std::basic_string<CharType> string_type;

        message_key(const string_type& c = string_type()) : c_context_(nullptr), c_key_(nullptr)
        {
            const size_t pos = c.find(CharType(4));
            if(pos == string_type::npos) {
                key_ = c;
            } else {
                context_ = c.substr(0, pos);
                key_ = c.substr(pos + 1);
            }
        }
        message_key(const CharType* c, const CharType* k) : c_key_(k)
        {
            static const CharType empty = 0;
            if(c != nullptr)
                c_context_ = c;
            else
                c_context_ = &empty;
        }
        bool operator<(const message_key& other) const
        {
            const int cc = compare(context(), other.context());
            if(cc != 0)
                return cc < 0;
            return compare(key(), other.key()) < 0;
        }
        bool operator==(const message_key& other) const
        {
            return compare(context(), other.context()) == 0 && compare(key(), other.key()) == 0;
        }
        bool operator!=(const message_key& other) const { return !(*this == other); }
        const CharType* context() const
        {
            if(c_context_)
                return c_context_;
            return context_.c_str();
        }
        const CharType* key() const
        {
            if(c_key_)
                return c_key_;
            return key_.c_str();
        }

    private:
        static int compare(const CharType* l, const CharType* r)
        {
            typedef std::char_traits<CharType> traits_type;
            for(;;) {
                const CharType cl = *l++;
                const CharType cr = *r++;
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
        const CharType* c_context_;
        const CharType* c_key_;
    };

    template<typename CharType>
    struct hash_function {
        size_t operator()(const message_key<CharType>& msg) const
        {
            pj_winberger_hash::state_type state = pj_winberger_hash::initial_state;
            const CharType* p = msg.context();
            if(*p != 0) {
                const CharType* e = util::str_end(p);
                state = pj_winberger_hash::update_state(state,
                                                        reinterpret_cast<const char*>(p),
                                                        reinterpret_cast<const char*>(e));
                state = pj_winberger_hash::update_state(state, '\4');
            }
            p = msg.key();
            const CharType* e = util::str_end(p);
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
        typedef std::basic_string<CharType> string_type;
        typedef message_key<CharType> key_type;
        typedef std::unordered_map<key_type, string_type, hash_function<CharType>> catalog_type;
        struct domain_data_type {
            std::unique_ptr<mo_file> mo_catalog; /// Message catalog (.mo file) if it can be directly used
            catalog_type catalog;                /// Converted message catalog when .mo file cannot be directly used
            lambda::plural_expr plural_form;     /// Expression to determine the plural form index
        };

    public:
        typedef std::pair<const CharType*, const CharType*> pair_type;

        const CharType* get(int domain_id, const CharType* context, const CharType* in_id) const override
        {
            return get_string(domain_id, context, in_id).first;
        }

        const CharType*
        get(int domain_id, const CharType* context, const CharType* single_id, count_type n) const override
        {
            const pair_type ptr = get_string(domain_id, context, single_id);
            if(!ptr.first)
                return nullptr;

            // domain_id is already checked by get_string -> Would return a null-pair
            BOOST_ASSERT(domain_id >= 0 && static_cast<size_t>(domain_id) < domain_data_.size());
            lambda::expr::value_type plural_idx;
            if(domain_data_[domain_id].plural_form)
                plural_idx = domain_data_[domain_id].plural_form(n);
            else
                plural_idx = n == 1 ? 0 : 1; // Fallback to English plural form

            const CharType* p = ptr.first;
            for(decltype(plural_idx) i = 0; p < ptr.second && i < plural_idx; ++i) {
                p = std::find(p, ptr.second, CharType(0));
                if(BOOST_UNLIKELY(p == ptr.second))
                    return nullptr;
                ++p;
            }
            return (p < ptr.second) ? p : nullptr;
        }

        int domain(const std::string& domain) const override
        {
            const auto p = domains_.find(domain);
            if(p == domains_.end())
                return -1;
            return p->second;
        }

        mo_message(const messages_info& inf) : key_conversion_required_(false)
        {
            const std::vector<messages_info::domain>& domains = inf.domains;
            domain_data_.resize(domains.size());

            const auto catalog_paths = inf.get_catalog_paths();
            for(unsigned i = 0; i < domains.size(); i++) {
                const auto& domain = domains[i];
                domains_[domain.name] = i;
                const std::string filename = domain.name + ".mo";
                for(std::string path : catalog_paths) {
                    path += "/" + filename;
                    if(load_file(path, inf.encoding, domain.encoding, domain_data_[i], inf.callback))
                        break;
                }
            }
        }

        const CharType* convert(const CharType* msg, string_type& buffer) const override
        {
            return runtime_conversion<CharType>(msg, buffer, key_conversion_required_, locale_encoding_, key_encoding_);
        }

    private:
        bool load_file(const std::string& file_name,
                       const std::string& locale_encoding,
                       const std::string& key_encoding,
                       domain_data_type& data,
                       const messages_info::callback_type& callback)
        {
            locale_encoding_ = locale_encoding;
            key_encoding_ = key_encoding;

            key_conversion_required_ =
              sizeof(CharType) == 1 && !util::are_encodings_equal(locale_encoding, key_encoding);

            std::unique_ptr<mo_file> mo;

            {
                std::vector<char> file_data;
                if(callback)
                    file_data = callback(file_name, locale_encoding);
                else {
                    c_file the_file(file_name, locale_encoding);
                    if(!the_file.handle)
                        return false;
                    file_data = read_file(the_file.handle);
                }
                if(file_data.empty())
                    return false;
                mo.reset(new mo_file(std::move(file_data)));
            }

            const std::string plural = extract(mo->value(0).first, "plural=", "\r\n;");
            const std::string mo_encoding = extract(mo->value(0).first, "charset=", " \r\n;");

            if(mo_encoding.empty())
                throw std::runtime_error("Invalid mo-format, encoding is not specified");

            if(!plural.empty())
                data.plural_form = lambda::compile(plural.c_str());

            if(mo_useable_directly(mo_encoding, *mo))
                data.mo_catalog = std::move(mo);
            else {
                converter<CharType> cvt_value(locale_encoding, mo_encoding);
                converter<CharType> cvt_key(key_encoding, mo_encoding);
                for(unsigned i = 0; i < mo->size(); i++) {
                    const char* ckey = mo->key(i);
                    const key_type key(cvt_key(ckey, ckey + strlen(ckey)));

                    mo_file::pair_type tmp = mo->value(i);
                    data.catalog[key] = cvt_value(tmp.first, tmp.second);
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
            if(!util::are_encodings_equal(mo_encoding, locale_encoding_))
                return false;
            if(util::are_encodings_equal(mo_encoding, key_encoding_)) {
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
            pos += key.size();
            const size_t end_pos = meta.find_first_of(separator, pos);
            return meta.substr(pos, end_pos - pos);
        }

        pair_type get_string(int domain_id, const CharType* context, const CharType* in_id) const
        {
            pair_type null_pair;
            if(domain_id < 0 || static_cast<size_t>(domain_id) >= domain_data_.size())
                return null_pair;
            const auto& data = domain_data_[domain_id];

            BOOST_LOCALE_START_CONST_CONDITION
            if(mo_file_use_traits<CharType>::in_use && data.mo_catalog) {
                BOOST_LOCALE_END_CONST_CONDITION
                return mo_file_use_traits<CharType>::use(*data.mo_catalog, context, in_id);
            } else {
                const key_type key(context, in_id);
                const catalog_type& cat = data.catalog;
                const auto p = cat.find(key);
                if(p == cat.end()) {
                    return null_pair;
                }
                return pair_type(p->second.data(), p->second.data() + p->second.size());
            }
        }

        std::map<std::string, unsigned> domains_;
        std::vector<domain_data_type> domain_data_;

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
