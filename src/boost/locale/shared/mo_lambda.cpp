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
        template<class TExp, typename... Ts>
        expr_ptr make_expr(Ts... ts)
        {
            return expr_ptr(new TExp(std::forward<Ts>(ts)...));
        }

        struct identity : public expr {
            int operator()(int n) const override { return n; };
        };

        struct unary : public expr {
            unary(expr_ptr p) : op1(std::move(p)) {}

        protected:
            expr_ptr op1;
        };

        struct binary : public expr {
            binary(expr_ptr p1, expr_ptr p2) : op1(std::move(p1)), op2(std::move(p2)) {}

        protected:
            expr_ptr op1, op2;
        };

        struct number : public expr {
            number(int v) : val(v) {}
            int operator()(int /*n*/) const override { return val; }

        private:
            int val;
        };

        enum { END = 0, SHL = 256, SHR, GTE, LTE, EQ, NEQ, AND, OR, NUM, VARIABLE };

#define UNOP(name, oper)                                               \
    struct name : public unary {                                       \
        name(expr_ptr op) : unary(std::move(op)) {}                    \
        int operator()(int n) const override { return oper(*op1)(n); } \
    };

        UNOP(l_not, !)
        UNOP(minus, -)
        UNOP(bin_not, ~)
#undef UNOP

#define BINOP(name, oper)                                                        \
    struct name : public binary {                                                \
        name(expr_ptr p1, expr_ptr p2) : binary(std::move(p1), std::move(p2)) {} \
                                                                                 \
        int operator()(int n) const override { return (*op1)(n)oper(*op2)(n); }  \
    };

#define BINOPD(name, oper)                                                       \
    struct name : public binary {                                                \
        name(expr_ptr p1, expr_ptr p2) : binary(std::move(p1), std::move(p2)) {} \
        int operator()(int n) const override                                     \
        {                                                                        \
            int v1 = (*op1)(n);                                                  \
            int v2 = (*op2)(n);                                                  \
            return v2 == 0 ? 0 : v1 oper v2;                                     \
        }                                                                        \
    };

        BINOP(mul, *)
        BINOPD(div, /)
        BINOPD(mod, %)
        constexpr int level10[] = {'*', '/', '%'};
#undef BINOPD

        BINOP(add, +)
        BINOP(sub, -)
        constexpr int level9[] = {'+', '-'};

        BINOP(shl, <<)
        BINOP(shr, >>)
        constexpr int level8[] = {SHL, SHR};

        BINOP(gt, >)
        BINOP(lt, <)
        BINOP(gte, >=)
        BINOP(lte, <=)
        constexpr int level7[] = {'<', '>', GTE, LTE};

        BINOP(eq, ==)
        BINOP(neq, !=)
        constexpr int level6[] = {EQ, NEQ};

        BINOP(bin_and, &)
        constexpr int level5[] = {'&'};

        BINOP(bin_xor, ^)
        constexpr int level4[] = {'^'};

        BINOP(bin_or, |)
        constexpr int level3[] = {'|'};

        BINOP(l_and, &&)
        constexpr int level2[] = {AND};

        BINOP(l_or, ||)
        constexpr int level1[] = {OR};

#undef BINOP

        struct conditional : public expr {
            conditional(expr_ptr p1, expr_ptr p2, expr_ptr p3) :
                op1(std::move(p1)), op2(std::move(p2)), op3(std::move(p3))
            {}
            int operator()(int n) const override { return (*op1)(n) ? (*op2)(n) : (*op3)(n); }

        private:
            expr_ptr op1, op2, op3;
        };

        expr_ptr bin_factory(const int value, expr_ptr left, expr_ptr right)
        {
#define BINOP_CASE(match, cls) \
    case match: return make_expr<cls>(std::move(left), std::move(right))

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
                default: return expr_ptr();
            }
#undef BINOP_CASE
        }

        template<size_t size>
        bool is_in(const int value, const int (&list)[size])
        {
            for(const int el : list) {
                if(value == el)
                    return true;
            }
            return false;
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

        class parser {
        public:
            parser(const char* str) : t(str){};

            expr_ptr compile()
            {
                expr_ptr res = cond_expr();
                if(res && t.next() != END) {
                    return expr_ptr();
                };
                return res;
            }

        private:
            expr_ptr value_expr()
            {
                expr_ptr op;
                if(t.next() == '(') {
                    t.get();
                    if(!(op = cond_expr()))
                        return expr_ptr();
                    if(t.get() != ')')
                        return expr_ptr();
                    return op;
                } else if(t.next() == NUM) {
                    int value;
                    t.get(&value);
                    return make_expr<number>(value);
                } else if(t.next() == VARIABLE) {
                    t.get();
                    return make_expr<identity>();
                }
                return expr_ptr();
            };

            expr_ptr unary_expr()
            {
                constexpr int level_unary[] = {'-', '!', '~'};
                if(is_in(t.next(), level_unary)) {
                    const int op = t.get();
                    expr_ptr op1 = unary_expr();
                    if(!op1)
                        return expr_ptr();
                    switch(op) {
                        case '-': return make_expr<minus>(std::move(op1));
                        case '!': return make_expr<l_not>(std::move(op1));
                        case '~': return make_expr<bin_not>(std::move(op1));
                        default: return expr_ptr();
                    }
                } else {
                    return value_expr();
                }
            };

#define BINARY_EXPR(lvl, nextLvl, list)                           \
    expr_ptr lvl()                                                \
    {                                                             \
        expr_ptr op1 = nextLvl();                                 \
        if(!op1)                                                  \
            return expr_ptr();                                    \
        while(is_in(t.next(), list)) {                            \
            const int o = t.get();                                \
            expr_ptr op2 = nextLvl();                             \
            if(!op2)                                              \
                return expr_ptr();                                \
            op1 = bin_factory(o, std::move(op1), std::move(op2)); \
        }                                                         \
        return op1;                                               \
    }

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
#undef BINARY_EXPR

            expr_ptr cond_expr()
            {
                expr_ptr cond;
                if(!(cond = l1()))
                    return expr_ptr();
                if(t.next() != '?')
                    return cond;
                t.get();
                expr_ptr case1, case2;
                if(!(case1 = cond_expr()))
                    return expr_ptr();
                if(t.get() != ':')
                    return expr_ptr();
                if(!(case2 = cond_expr()))
                    return expr_ptr();
                return make_expr<conditional>(std::move(cond), std::move(case1), std::move(case2));
            }

            tokenizer t;
        };

    } // namespace

    plural_expr compile(const char* str)
    {
        parser p(str);
        return plural_expr(p.compile());
    }

}}}} // namespace boost::locale::gnu_gettext::lambda
