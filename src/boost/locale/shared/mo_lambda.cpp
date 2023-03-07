//
// Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "boost/locale/shared/mo_lambda.hpp"
#include <cstdlib>
#include <cstring>

#ifdef BOOST_MSVC
#    pragma warning(disable : 4512) // assignment operator could not be generated
#endif

namespace boost { namespace locale { namespace gnu_gettext { namespace lambda {

    namespace { // anon
        struct identity : public plural {
            int operator()(int n) const override { return n; };
            plural_ptr clone() const override { return plural_ptr(new identity()); }
        };

        struct unary : public plural {
            unary(plural_ptr p) : op1(std::move(p)) {}

        protected:
            plural_ptr op1;
        };

        struct binary : public plural {
            binary(plural_ptr p1, plural_ptr p2) : op1(std::move(p1)), op2(std::move(p2)) {}

        protected:
            plural_ptr op1, op2;
        };

        struct number : public plural {
            number(int v) : val(v) {}
            int operator()(int /*n*/) const override { return val; }
            plural_ptr clone() const override { return plural_ptr(new number(val)); }

        private:
            int val;
        };

#define UNOP(name, oper)                                                                 \
    struct name : public unary {                                                         \
        name(plural_ptr op) : unary(std::move(op)) {}                                    \
        int operator()(int n) const override { return oper(*op1)(n); }                   \
        plural_ptr clone() const override { return plural_ptr(new name(op1->clone())); } \
    };

#define BINOP(name, oper)                                                                              \
    struct name : public binary {                                                                      \
        name(plural_ptr p1, plural_ptr p2) : binary(std::move(p1), std::move(p2)) {}                   \
                                                                                                       \
        int operator()(int n) const override { return (*op1)(n)oper(*op2)(n); }                        \
        plural_ptr clone() const override { return plural_ptr(new name(op1->clone(), op2->clone())); } \
    };

#define BINOPD(name, oper)                                                                             \
    struct name : public binary {                                                                      \
        name(plural_ptr p1, plural_ptr p2) : binary(std::move(p1), std::move(p2)) {}                   \
        int operator()(int n) const override                                                           \
        {                                                                                              \
            int v1 = (*op1)(n);                                                                        \
            int v2 = (*op2)(n);                                                                        \
            return v2 == 0 ? 0 : v1 oper v2;                                                           \
        }                                                                                              \
        plural_ptr clone() const override { return plural_ptr(new name(op1->clone(), op2->clone())); } \
    };

        enum { END = 0, SHL = 256, SHR, GTE, LTE, EQ, NEQ, AND, OR, NUM, VARIABLE };

        UNOP(l_not, !)
        UNOP(minus, -)
        UNOP(bin_not, ~)
#undef UNOP

        BINOP(mul, *)
        BINOPD(div, /)
        BINOPD(mod, %)
        static int level10[] = {3, '*', '/', '%'};
#undef BINOPD

        BINOP(add, +)
        BINOP(sub, -)
        static int level9[] = {2, '+', '-'};

        BINOP(shl, <<)
        BINOP(shr, >>)
        static int level8[] = {2, SHL, SHR};

        BINOP(gt, >)
        BINOP(lt, <)
        BINOP(gte, >=)
        BINOP(lte, <=)
        static int level7[] = {4, '<', '>', GTE, LTE};

        BINOP(eq, ==)
        BINOP(neq, !=)
        static int level6[] = {2, EQ, NEQ};

        BINOP(bin_and, &)
        static int level5[] = {1, '&'};

        BINOP(bin_xor, ^)
        static int level4[] = {1, '^'};

        BINOP(bin_or, |)
        static int level3[] = {1, '|'};

        BINOP(l_and, &&)
        static int level2[] = {1, AND};

        BINOP(l_or, ||)
        static int level1[] = {1, OR};

#undef BINOP

        struct conditional : public plural {
            conditional(plural_ptr p1, plural_ptr p2, plural_ptr p3) :
                op1(std::move(p1)), op2(std::move(p2)), op3(std::move(p3))
            {}
            int operator()(int n) const override { return (*op1)(n) ? (*op2)(n) : (*op3)(n); }
            plural_ptr clone() const override
            {
                return plural_ptr(new conditional(op1->clone(), op2->clone(), op3->clone()));
            }

        private:
            plural_ptr op1, op2, op3;
        };

        plural_ptr bin_factory(int value, plural_ptr left, plural_ptr right)
        {
#define BINOP_CASE(match, cls) \
    case match: return plural_ptr(new cls(std::move(left), std::move(right)))

            switch(value) {
                BINOP_CASE('/', div);
                BINOP_CASE('*', mul);
                BINOP_CASE('%', mod);
                BINOP_CASE('+', add);
                BINOP_CASE('-', sub);
                BINOP_CASE(SHL, shl);
                BINOP_CASE(SHR, shr);
                BINOP_CASE('>', gt);
                BINOP_CASE('<', lt);
                BINOP_CASE(GTE, gte);
                BINOP_CASE(LTE, lte);
                BINOP_CASE(EQ, eq);
                BINOP_CASE(NEQ, neq);
                BINOP_CASE('&', bin_and);
                BINOP_CASE('^', bin_xor);
                BINOP_CASE('|', bin_or);
                BINOP_CASE(AND, l_and);
                BINOP_CASE(OR, l_or);
                default: return plural_ptr();
            }
#undef BINOP_CASE
        }

        static inline bool is_in(int v, int* p)
        {
            int len = *p;
            p++;
            while(len && *p != v) {
                p++;
                len--;
            }
            return len != 0;
        }

        class tokenizer {
        public:
            tokenizer(const char* s) : text_(s), next_tocken_(0), int_value_(0) { step(); };
            int get(int* val = nullptr)
            {
                const int res = next_tocken_;
                if(val && res == NUM)
                    *val = int_value_;
                step();
                return res;
            };
            int next(int* val = nullptr)
            {
                if(val && next_tocken_ == NUM)
                    *val = int_value_;
                return next_tocken_;
            }

        private:
            const char* text_;
            int next_tocken_;
            int int_value_;
            static constexpr bool is_blank(char c) { return c == ' ' || c == '\r' || c == '\n' || c == '\t'; }
            static constexpr bool is_digit(char c) { return '0' <= c && c <= '9'; }
            template<size_t size>
            static bool is(const char* s, const char (&search)[size])
            {
                return strncmp(s, search, size - 1) == 0;
            }
            void step()
            {
                while(is_blank(*text_))
                    text_++;
                const char* text = text_;
                if(is(text, "<<")) {
                    text_ += 2;
                    next_tocken_ = SHL;
                } else if(is(text, ">>")) {
                    text_ += 2;
                    next_tocken_ = SHR;
                } else if(is(text, "&&")) {
                    text_ += 2;
                    next_tocken_ = AND;
                } else if(is(text, "||")) {
                    text_ += 2;
                    next_tocken_ = OR;
                } else if(is(text, "<=")) {
                    text_ += 2;
                    next_tocken_ = LTE;
                } else if(is(text, ">=")) {
                    text_ += 2;
                    next_tocken_ = GTE;
                } else if(is(text, "==")) {
                    text_ += 2;
                    next_tocken_ = EQ;
                } else if(is(text, "!=")) {
                    text_ += 2;
                    next_tocken_ = NEQ;
                } else if(*text == 'n') {
                    text_++;
                    next_tocken_ = VARIABLE;
                } else if(is_digit(*text)) {
                    char* tmp_ptr;
                    int_value_ = strtol(text, &tmp_ptr, 0);
                    text_ = tmp_ptr;
                    next_tocken_ = NUM;
                } else if(*text == '\0') {
                    next_tocken_ = 0;
                } else {
                    next_tocken_ = *text;
                    text_++;
                }
            }
        };

#define BINARY_EXPR(expr, hexpr, list)                            \
    plural_ptr expr()                                             \
    {                                                             \
        plural_ptr op1, op2;                                      \
        if(!(op1 = hexpr()))                                      \
            return plural_ptr();                                  \
        while(is_in(t.next(), list)) {                            \
            const int o = t.get();                                \
            if(!(op2 = hexpr()))                                  \
                return plural_ptr();                              \
            op1 = bin_factory(o, std::move(op1), std::move(op2)); \
        }                                                         \
        return op1;                                               \
    }

        class parser {
        public:
            parser(tokenizer& tin) : t(tin){};

            plural_ptr compile()
            {
                plural_ptr res = cond_expr();
                if(res && t.next() != END) {
                    return plural_ptr();
                };
                return res;
            }

        private:
            plural_ptr value_expr()
            {
                plural_ptr op;
                if(t.next() == '(') {
                    t.get();
                    if(!(op = cond_expr()))
                        return plural_ptr();
                    if(t.get() != ')')
                        return plural_ptr();
                    return op;
                } else if(t.next() == NUM) {
                    int value;
                    t.get(&value);
                    return plural_ptr(new number(value));
                } else if(t.next() == VARIABLE) {
                    t.get();
                    return plural_ptr(new identity());
                }
                return plural_ptr();
            };

            plural_ptr unary_expr()
            {
                static int level_unary[] = {3, '-', '!', '~'};
                if(is_in(t.next(), level_unary)) {
                    const int op = t.get();
                    plural_ptr op1 = unary_expr();
                    if(!op1)
                        return plural_ptr();
                    switch(op) {
                        case '-': return plural_ptr(new minus(std::move(op1)));
                        case '!': return plural_ptr(new l_not(std::move(op1)));
                        case '~': return plural_ptr(new bin_not(std::move(op1)));
                        default: return plural_ptr();
                    }
                } else {
                    return value_expr();
                }
            };

            BINARY_EXPR(l10, unary_expr, level10);
            BINARY_EXPR(l9, l10, level9);
            BINARY_EXPR(l8, l9, level8);
            BINARY_EXPR(l7, l8, level7);
            BINARY_EXPR(l6, l7, level6);
            BINARY_EXPR(l5, l6, level5);
            BINARY_EXPR(l4, l5, level4);
            BINARY_EXPR(l3, l4, level3);
            BINARY_EXPR(l2, l3, level2);
            BINARY_EXPR(l1, l2, level1);

            plural_ptr cond_expr()
            {
                plural_ptr cond, case1, case2;
                if(!(cond = l1()))
                    return plural_ptr();
                if(t.next() == '?') {
                    t.get();
                    if(!(case1 = cond_expr()))
                        return plural_ptr();
                    if(t.get() != ':')
                        return plural_ptr();
                    if(!(case2 = cond_expr()))
                        return plural_ptr();
                } else {
                    return cond;
                }
                return plural_ptr(new conditional(std::move(cond), std::move(case1), std::move(case2)));
            }

            tokenizer& t;
        };

    } // namespace

    plural_ptr compile(const char* str)
    {
        tokenizer t(str);
        parser p(t);
        return p.compile();
    }

}}}} // namespace boost::locale::gnu_gettext::lambda
