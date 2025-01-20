#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "parser_token.h"
#include <stdbool.h>
#include <stdint.h>

enum parser_node_kind {
    /* stmt */
    NOD_STMT_NOP,
    NOD_STMT_IF,
    NOD_STMT_WHILE,
    NOD_STMT_FORNUM,
    NOD_STMT_FORARRAY,
    NOD_STMT_FORMAP,
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
    NOD_EXPR_IDENT,
    NOD_EXPR_FIELD,
    NOD_EXPR_COLUMN,
    /* literal */
    NOD_EXPR_NILLIT,
    NOD_EXPR_BOOLLIT,
    NOD_EXPR_INTLIT,
    NOD_EXPR_FLOATLIT,
    NOD_EXPR_STRINGLIT,
    NOD_EXPR_FUNCLIT,
    NOD_EXPR_ARRAYLIT,
    NOD_EXPR_MAPLIT,
    NOD_EXPR_STRUCTLIT,
    NOD_EXPR_TABLELIT,
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
    /* array, struct, map, func */
    NOD_EXPR_INDEX,
    NOD_EXPR_MAPINDEX,
    NOD_EXPR_SELECT,
    NOD_EXPR_ENUMACCESS,
    NOD_EXPR_CALL,
    NOD_EXPR_ELEMENT, /* TODO may not need */
    NOD_EXPR_MODULE,
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
struct parser_symbol;
struct parser_var;
struct parser_func;
struct parser_field;
struct parser_struct;
struct parser_table;
struct parser_column;

struct parser_expr {
    int kind;
    const struct parser_type *type;

    struct parser_expr *l;
    struct parser_expr *r;
    struct parser_expr *next;

    struct parser_symbol *sym;
    struct parser_var *var;
    struct parser_field *field;
    struct parser_column *column;
    /* this is mostly use for args */
    struct parser_pos pos;

    /* literals */
    union {
        int64_t ival;
        double fval;
        const char *sval;
        struct parser_func *func;
    };
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
struct parser_expr *parser_new_funclit_expr(struct parser_func *func);
struct parser_expr *parser_new_arraylit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_maplit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len);
struct parser_expr *parser_new_structlit_expr(const struct parser_struct *strct,
        struct parser_expr *fields);
struct parser_expr *parser_new_tablelit_expr(const struct parser_table *table,
        int row_idx);
struct parser_expr *parser_new_conversion_expr(struct parser_expr *from,
        struct parser_type *to);
struct parser_expr *parser_new_ident_expr(struct parser_symbol *sym);
struct parser_expr *parser_new_index_expr(struct parser_expr *ary, struct parser_expr *idx);
struct parser_expr *parser_new_mapindex_expr(struct parser_expr *map, struct parser_expr *key);
struct parser_expr *parser_new_select_expr(struct parser_expr *inst, struct parser_expr *fld);
struct parser_expr *parser_new_enum_access_expr(struct parser_expr *enm,
        struct parser_expr *fld);
struct parser_expr *parser_new_field_expr(struct parser_field *f);
struct parser_expr *parser_new_column_expr(struct parser_column *c);
struct parser_expr *parser_new_call_expr(struct parser_expr *callee, struct parser_expr *args);
struct parser_expr *parser_new_element_expr(struct parser_expr *key, struct parser_expr *val);
struct parser_expr *parser_new_module_expr(struct parser_expr *mod,
        struct parser_expr *member);

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
struct parser_stmt *parser_new_while_stmt(struct parser_expr *cond, struct parser_stmt *body);
struct parser_stmt *parser_new_fornum_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_forarray_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body);
struct parser_stmt *parser_new_formap_stmt(struct parser_expr *iter,
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

struct parser_stmt *parser_new_inc_stmt(struct parser_expr *l);
struct parser_stmt *parser_new_dec_stmt(struct parser_expr *l);

/* node info */
struct parser_node_info {
    const char *str;
    char type;
};

const struct parser_node_info *parser_get_node_info(int kind);

#endif /* _H */
