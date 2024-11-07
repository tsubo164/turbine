#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "token.h"
#include <stdbool.h>
#include <stdint.h>

enum parser_node_kind {
    /* stmt */
    NOD_STMT_NOP,
    NOD_STMT_IF,
    NOD_STMT_FOR,
    NOD_STMT_ELSE,
    NOD_STMT_BREAK,
    NOD_STMT_CONTINUE,
    NOD_STMT_SWITCH,
    NOD_STMT_CASE,
    NOD_STMT_DEFAULT,
    NOD_STMT_RETURN,
    NOD_STMT_EXPR,
    NOD_STMT_ASSIGN,
    NOD_STMT_INIT,
    NOD_STMT_BLOCK,
    /* identifier */
    NOD_EXPR_FIELD,
    NOD_EXPR_IDENT,
    /* literal */
    NOD_EXPR_NILLIT,
    NOD_EXPR_BOOLLIT,
    NOD_EXPR_INTLIT,
    NOD_EXPR_FLOATLIT,
    NOD_EXPR_STRINGLIT,
    NOD_EXPR_FUNCLIT,
    NOD_EXPR_ARRAYLIT,
    NOD_EXPR_STRUCTLIT,
    /* binary */
    NOD_EXPR_ADD,
    NOD_EXPR_SUB,
    NOD_EXPR_MUL,
    NOD_EXPR_DIV,
    NOD_EXPR_REM,
    /* relational */
    NOD_EXPR_EQ,
    NOD_EXPR_NEQ,
    NOD_EXPR_LT,
    NOD_EXPR_LTE,
    NOD_EXPR_GT,
    NOD_EXPR_GTE,
    /* bitwise */
    NOD_EXPR_SHL,
    NOD_EXPR_SHR,
    NOD_EXPR_OR,
    NOD_EXPR_XOR,
    NOD_EXPR_AND,
    /* logical */
    NOD_EXPR_LOGOR,
    NOD_EXPR_LOGAND,
    NOD_EXPR_LOGNOT,
    /* unary */
    NOD_EXPR_POS,
    NOD_EXPR_NEG,
    NOD_EXPR_ADDRESS,
    NOD_EXPR_DEREF,
    NOD_EXPR_NOT,
    NOD_EXPR_INC,
    NOD_EXPR_DEC,
    NOD_EXPR_CONV,
    /* array, struct, func */
    NOD_EXPR_SELECT,
    NOD_EXPR_INDEX,
    NOD_EXPR_CALL,
    NOD_EXPR_ELEMENT, /* TODO may not need */
    /* assign */
    /* TODO should be NOD_EXPR */
    NOD_EXPR_ASSIGN,
    NOD_EXPR_ADDASSIGN,
    NOD_EXPR_SUBASSIGN,
    NOD_EXPR_MULASSIGN,
    NOD_EXPR_DIVASSIGN,
    NOD_EXPR_REMASSIGN,
    NOD_EXPR_INIT,
};

struct Type;
struct Symbol;
struct Var;
struct Func;
struct Field;
struct Struct;

struct parser_expr {
    int kind;
    const struct Type *type;
    struct Pos pos;

    struct parser_expr *l;
    struct parser_expr *r;
    struct parser_expr *next;

    struct Symbol *sym;
    struct Var *var;
    struct Field *field;

    /* literals */
    union {
        long ival;
        double fval;
        const char *sval;
        struct Func *func;
    };
    const char *converted;
};

struct parser_stmt {
    int kind;

    struct parser_expr* expr;
    struct parser_stmt* init;
    struct parser_expr* cond;
    struct parser_stmt* post;
    struct parser_stmt* body;
    /* children */
    struct parser_stmt *children;
    struct parser_stmt *next;
};

/* expr */
struct parser_expr *parser_new_nillit_expr(void);
struct parser_expr *parser_new_boollit_expr(bool b);
struct parser_expr *parser_new_intlit_expr(long l);
struct parser_expr *parser_new_floatlit_expr(double d);
struct parser_expr *parser_new_stringlit_expr(const char *s);
struct parser_expr *parser_new_funclit_expr(struct Func *func);
struct parser_expr *parser_new_arraylit_expr(struct parser_expr *elems, int len);
struct parser_expr *parser_new_structlit_expr(struct Struct *strct, struct parser_expr *fields);
struct parser_expr *parser_new_conversion_expr(struct parser_expr *from, struct Type *to);
struct parser_expr *parser_new_ident_expr(struct Symbol *sym);
struct parser_expr *parser_new_field_expr(struct Field *f);
struct parser_expr *parser_new_select_expr(struct parser_expr *inst, struct parser_expr *fld);
struct parser_expr *parser_new_index_expr(struct parser_expr *ary, struct parser_expr *idx);
struct parser_expr *parser_new_call_expr(struct parser_expr *callee, struct Pos p);
struct parser_expr *parser_new_element_expr(struct parser_expr *key, struct parser_expr *val);

/* unary expr */
struct parser_expr *parser_new_posi_expr(struct parser_expr *l);
struct parser_expr *parser_new_nega_expr(struct parser_expr *l);
struct parser_expr *parser_new_lognot_expr(struct parser_expr *l);
struct parser_expr *parser_new_not_expr(struct parser_expr *l);
struct parser_expr *parser_new_addr_expr(struct parser_expr *l);
struct parser_expr *parser_new_deref_expr(struct parser_expr *l);

/* binary expr */
struct parser_expr *parser_new_add_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_sub_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_mul_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_div_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_rem_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_shl_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_shr_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_and_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_or_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_xor_expr(struct parser_expr *l, struct parser_expr *r);

/* relational expr */
struct parser_expr *parser_new_eq_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_neq_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_lt_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_lte_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_gt_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_gte_expr(struct parser_expr *l, struct parser_expr *r);

/* logical expr */
struct parser_expr *parser_new_logand_expr(struct parser_expr *l, struct parser_expr *r);
struct parser_expr *parser_new_logor_expr(struct parser_expr *l, struct parser_expr *r);

/* stmt */
struct parser_stmt *parser_new_nop_stmt(void);
struct parser_stmt *parser_new_block_stmt(struct parser_stmt *children);
struct parser_stmt *parser_new_if_stmt(struct parser_stmt *or_list);
struct parser_stmt *parser_new_else_stmt(struct parser_expr *cond, struct parser_stmt *body);
struct parser_stmt *parser_new_for_stmt(struct parser_stmt *init, struct parser_expr *cond,
        struct parser_stmt *post, struct parser_stmt *body);
struct parser_stmt *parser_new_break_stmt(void);
struct parser_stmt *parser_new_continue_stmt(void);
struct parser_stmt *parser_new_switch_stmt(struct parser_expr *cond, struct parser_stmt *cases);
struct parser_stmt *parser_new_case_stmt(struct parser_expr *conds, struct parser_stmt *body);
struct parser_stmt *parser_new_default_stmt(struct parser_stmt *body);
struct parser_stmt *parser_new_return_stmt(struct parser_expr *e);
struct parser_stmt *parser_new_expr_stmt(struct parser_expr *e);

/* assign stmt */
struct parser_stmt *parser_new_init_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_assign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_addassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_subassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_mulassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_divassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_remassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_inc_stmt(struct parser_expr *l);
struct parser_stmt *parser_new_dec_stmt(struct parser_expr *l);

#endif /* _H */
