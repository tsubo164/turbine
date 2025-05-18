#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "parser_token.h"
#include "parser_symbol.h"
#include "value_types.h"
#include <stdbool.h>

enum parser_node_kind {
    /* stmt */
    NOD_STMT_NOP,
    NOD_STMT_IF,
    NOD_STMT_ELSE,
    NOD_STMT_WHILE,
    NOD_STMT_FORNUM,
    NOD_STMT_FORVEC,
    NOD_STMT_FORMAP,
    NOD_STMT_FORSET,
    NOD_STMT_FORSTACK,
    NOD_STMT_FORQUEUE,
    NOD_STMT_FORENUM,
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
    NOD_EXPR_VAR,
    NOD_EXPR_STRUCTFIELD,
    NOD_EXPR_ENUMFIELD,
    /* literal */
    NOD_EXPR_NILLIT,
    NOD_EXPR_BOOLLIT,
    NOD_EXPR_INTLIT,
    NOD_EXPR_FLOATLIT,
    NOD_EXPR_STRINGLIT,
    NOD_EXPR_FUNCLIT,
    NOD_EXPR_VECLIT,
    NOD_EXPR_MAPLIT,
    NOD_EXPR_SETLIT,
    NOD_EXPR_STACKLIT,
    NOD_EXPR_QUEUELIT,
    NOD_EXPR_STRUCTLIT,
    NOD_EXPR_ENUMLIT,
    NOD_EXPR_MODULELIT,
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
    NOD_EXPR_NOT,
    NOD_EXPR_CONV,
    /* vec, map, struct, func */
    NOD_EXPR_INDEX,
    NOD_EXPR_MAPINDEX,
    NOD_EXPR_STRUCTACCESS,
    NOD_EXPR_ENUMACCESS,
    NOD_EXPR_MODULEACCESS,
    NOD_EXPR_CALL,
    NOD_EXPR_ELEMENT, /* TODO may not need */
    /* assign */
    NOD_EXPR_ASSIGN,
    NOD_EXPR_ADDASSIGN,
    NOD_EXPR_SUBASSIGN,
    NOD_EXPR_MULASSIGN,
    NOD_EXPR_DIVASSIGN,
    NOD_EXPR_REMASSIGN,
    NOD_EXPR_SHLASSIGN,
    NOD_EXPR_SHRASSIGN,
    NOD_EXPR_ORASSIGN,
    NOD_EXPR_XORASSIGN,
    NOD_EXPR_ANDASSIGN,
    NOD_EXPR_INIT,
};

struct parser_type;

struct parser_expr {
    int kind;
    int kind_orig; /* constexpr */
    const struct parser_type *type;
    /* this is mostly use for args */
    struct parser_pos pos;

    struct parser_expr *l;
    struct parser_expr *r;
    struct parser_expr *next;

    /* identifiers */
    union {
        struct parser_var *var;
        struct parser_struct_field *struct_field;
        struct parser_enum_field *enum_field;
        struct parser_func *func;
    };

    /* literals */
    union {
        value_int_t ival;
        value_float_t fval;
        const char *sval;
    };

    /* constexpr */
    bool is_const;
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
struct parser_expr *parser_new_funclit_expr(const struct parser_type *func_type,
        struct parser_func *func);
struct parser_expr *parser_new_veclit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_maplit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_setlit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_stacklit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_queuelit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_structlit_expr(const struct parser_type *struct_type,
        struct parser_expr *fields);
struct parser_expr *parser_new_enumlit_expr(const struct parser_type *enum_type,
        int member_idx);
struct parser_expr *parser_new_modulelit_expr(const struct parser_type *module_type);
struct parser_expr *parser_new_conversion_expr(struct parser_expr *from,
        struct parser_type *to);

/* access */
struct parser_expr *parser_new_index_expr(struct parser_expr *ary, struct parser_expr *idx);
struct parser_expr *parser_new_mapindex_expr(struct parser_expr *map, struct parser_expr *key);
struct parser_expr *parser_new_struct_access_expr(struct parser_expr *inst,
        struct parser_expr *fld);
struct parser_expr *parser_new_enum_access_expr(struct parser_expr *enm,
        struct parser_expr *fld);
struct parser_expr *parser_new_module_access_expr(struct parser_expr *mod,
        struct parser_expr *member);

struct parser_expr *parser_new_var_expr(struct parser_var *v);
struct parser_expr *parser_new_struct_field_expr(struct parser_struct_field *f);
struct parser_expr *parser_new_enum_field_expr(struct parser_enum_field *f);
struct parser_expr *parser_new_call_expr(struct parser_expr *callee, struct parser_expr *args);
struct parser_expr *parser_new_element_expr(struct parser_expr *key, struct parser_expr *val);
struct parser_expr *parser_new_const_expr(struct parser_expr *orig, struct parser_expr *cnst);

/* unary expr */
struct parser_expr *parser_new_posi_expr(struct parser_expr *l);
struct parser_expr *parser_new_nega_expr(struct parser_expr *l);
struct parser_expr *parser_new_lognot_expr(struct parser_expr *l);
struct parser_expr *parser_new_not_expr(struct parser_expr *l);

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
struct parser_stmt *parser_new_while_stmt(struct parser_expr *cond, struct parser_stmt *body);

struct parser_stmt *parser_new_fornum_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_forvec_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_formap_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_forset_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_forstack_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_forqueue_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_forenum_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);

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

struct parser_stmt *parser_new_shlassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_shrassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_andassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_orassign_stmt(struct parser_expr *l, struct parser_expr *r);
struct parser_stmt *parser_new_xorassign_stmt(struct parser_expr *l, struct parser_expr *r);

/* node string */
const char *parser_node_string(int kind);

/* global */
bool parser_ast_is_global(const struct parser_expr *e);
bool parser_ast_is_mutable(const struct parser_expr *e);

void parser_free_expr(struct parser_expr *s);
void parser_free_stmt(struct parser_stmt *s);

#endif /* _H */
