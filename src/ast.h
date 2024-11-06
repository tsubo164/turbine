#ifndef AST_H
#define AST_H

#include "token.h"
#include <stdint.h>

enum parser_node_kind {
    /* stmt */
    NOD_STMT_NOP                    = T_NOP,
    NOD_STMT_IF                     = T_IF,
    NOD_STMT_FOR                    = T_FOR,
    NOD_STMT_ELSE                   = T_ELS,
    NOD_STMT_BREAK                  = T_BRK,
    NOD_STMT_CONTINUE               = T_CNT,
    NOD_STMT_SWITCH                 = T_SWT,
    NOD_STMT_CASE                   = T_CASE,
    NOD_STMT_DEFAULT                = T_DFLT,
    NOD_STMT_RETURN                 = T_RET,
    NOD_STMT_EXPR                   = T_EXPR,
    NOD_STMT_BLOCK                  = T_BLOCK,
    /* identifier */
    NOD_EXPR_FIELD                  = T_FIELD,
    NOD_EXPR_IDENT                  = T_IDENT,
    /* literal */
    NOD_EXPR_NILLIT                 = T_NILLIT,
    NOD_EXPR_BOOLLIT                = T_BOLLIT,
    NOD_EXPR_INTLIT                 = T_INTLIT,
    NOD_EXPR_FLOATLIT               = T_FLTLIT,
    NOD_EXPR_STRINGLIT              = T_STRLIT,
    NOD_EXPR_FUNCLIT                = T_FUNCLIT,
    NOD_EXPR_ARRAYLIT               = T_ARRAYLIT,
    NOD_EXPR_STRUCTLIT              = T_STRUCTLIT,
    /* binary */
    NOD_EXPR_ADD                    = T_ADD,
    NOD_EXPR_SUB                    = T_SUB,
    NOD_EXPR_MUL                    = T_MUL,
    NOD_EXPR_DIV                    = T_DIV,
    NOD_EXPR_REM                    = T_REM,
    /* relational */
    NOD_EXPR_EQ                     = T_EQ,
    NOD_EXPR_NEQ                    = T_NEQ,
    NOD_EXPR_LT                     = T_LT,
    NOD_EXPR_LTE                    = T_LTE,
    NOD_EXPR_GT                     = T_GT,
    NOD_EXPR_GTE                    = T_GTE,
    /* bitwise */
    NOD_EXPR_SHL                    = T_SHL,
    NOD_EXPR_SHR                    = T_SHR,
    NOD_EXPR_OR                     = T_OR,
    NOD_EXPR_XOR                    = T_XOR,
    NOD_EXPR_AND                    = T_AND,
    /* logical */
    NOD_EXPR_LOGOR                  = T_LOR,
    NOD_EXPR_LOGAND                 = T_LAND,
    NOD_EXPR_LOGNOT                 = T_LNOT,
    /* unary */
    NOD_EXPR_POS                    = T_POS,
    NOD_EXPR_NEG                    = T_NEG,
    NOD_EXPR_ADDRESS                = T_ADR,
    NOD_EXPR_DEREF                  = T_DRF,
    NOD_EXPR_NOT                    = T_NOT,
    NOD_EXPR_INC                    = T_INC,
    NOD_EXPR_DEC                    = T_DEC,
    NOD_EXPR_CONV                   = T_CONV,
    /* array, struct, func */
    NOD_EXPR_SELECT                 = T_SELECT,
    NOD_EXPR_INDEX                  = T_INDEX,
    NOD_EXPR_CALL                   = T_CALL,
    /* assign */
    /* TODO should be NOD_EXPR */
    NOD_EXPR_ASSIGN                 = T_ASSN,
    NOD_EXPR_ADDASSIGN              = T_AADD,
    NOD_EXPR_SUBASSIGN              = T_ASUB,
    NOD_EXPR_MULASSIGN              = T_AMUL,
    NOD_EXPR_DIVASSIGN              = T_ADIV,
    NOD_EXPR_REMASSIGN              = T_AREM,
    NOD_EXPR_INIT                   = T_INIT,
    NOD_STMT_ASSIGN                 = NOD_EXPR_ASSIGN,
    NOD_STMT_INIT                   = NOD_EXPR_INIT,
    NOD_EXPR_ELEMENT                = T_ELEMENT, /* TODO may not need */
};

struct Type;
struct Symbol;
struct Var;
struct Func;
struct Field;
struct Struct;

struct Expr {
    int kind;
    const struct Type *type;
    struct Pos pos;

    struct Expr *l;
    struct Expr *r;
    struct Expr *next;

    struct Symbol *sym;
    struct Var *var;
    struct Field *field;

    // literals
    union {
        long ival;
        double fval;
        const char *sval;
        struct Func *func;
    };
    const char *converted;
};

struct Stmt {
    int kind;

    struct Expr* expr;
    struct Stmt* init;
    struct Expr* cond;
    struct Stmt* post;
    struct Stmt* body;
    // children
    struct Stmt *children;
    struct Stmt *next;
};

// Expr
struct Expr *NewNilLitExpr(void);
struct Expr *NewBoolLitExpr(bool b);
struct Expr *NewIntLitExpr(long l);
struct Expr *NewFloatLitExpr(double d);
struct Expr *NewStringLitExpr(const char *s);
struct Expr *NewFuncLitExpr(struct Func *func);
struct Expr *NewArrayLitExpr(struct Expr *elems, int len);
struct Expr *NewStructLitExpr(struct Struct *strct, struct Expr *fields);
struct Expr *NewConversionExpr(struct Expr *from, struct Type *to);
struct Expr *NewIdentExpr(struct Symbol *sym);
struct Expr *NewFieldExpr(struct Field *f);
struct Expr *NewSelectExpr(struct Expr *inst, struct Expr *fld);
struct Expr *NewIndexExpr(struct Expr *ary, struct Expr *idx);
struct Expr *NewCallExpr(struct Expr *callee, struct Pos p);
//struct Expr *NewBinaryExpr(struct Expr *L, struct Expr *R, int kind);
//struct Expr *NewRelationalExpr(struct Expr *L, struct Expr *R, int kind);
//struct Expr *NewUnaryExpr(struct Expr *L, struct Type *t, int kind);
struct Expr *NewElementExpr(struct Expr *key, struct Expr *val);

/* binary expr */
struct Expr *parser_new_add_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_sub_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_mul_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_div_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_rem_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_shl_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_shr_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_and_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_or_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_xor_expr(struct Expr *l, struct Expr *r);

/* relational expr */
struct Expr *parser_new_eq_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_neq_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_lt_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_lte_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_gt_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_gte_expr(struct Expr *l, struct Expr *r);

/* unary expr */
struct Expr *parser_new_posi_expr(struct Expr *l);
struct Expr *parser_new_nega_expr(struct Expr *l);
struct Expr *parser_new_lognot_expr(struct Expr *l);
struct Expr *parser_new_not_expr(struct Expr *l);
struct Expr *parser_new_addr_expr(struct Expr *l);
struct Expr *parser_new_deref_expr(struct Expr *l);

/* logical expr */
struct Expr *parser_new_logand_expr(struct Expr *l, struct Expr *r);
struct Expr *parser_new_logor_expr(struct Expr *l, struct Expr *r);

// Stmt
struct Stmt *NewNopStmt(void);
struct Stmt *NewBlockStmt(struct Stmt *children);
struct Stmt *NewOrStmt(struct Expr *cond, struct Stmt *body);
struct Stmt *NewIfStmt(struct Stmt *or_list);
struct Stmt *NewForStmt(struct Stmt *init, struct Expr *cond, struct Stmt *post,
        struct Stmt *body);
struct Stmt *NewJumpStmt(int kind);
struct Stmt *NewCaseStmt(struct Expr *conds, struct Stmt *body, int kind);
struct Stmt *NewSwitchStmt(struct Expr *cond, struct Stmt *cases);
struct Stmt *NewReturnStmt(struct Expr *e);
struct Stmt *NewExprStmt(struct Expr *e);
//struct Stmt *NewAssignStmt(struct Expr *l, struct Expr *r, int kind);
struct Stmt *NewInitStmt(struct Expr *l, struct Expr *r);
//struct Stmt *NewIncDecStmt(struct Expr *l, int kind);
struct Stmt *parser_new_assign_stmt(struct Expr *l, struct Expr *r);
struct Stmt *parser_new_addassign_stmt(struct Expr *l, struct Expr *r);
struct Stmt *parser_new_subassign_stmt(struct Expr *l, struct Expr *r);
struct Stmt *parser_new_mulassign_stmt(struct Expr *l, struct Expr *r);
struct Stmt *parser_new_divassign_stmt(struct Expr *l, struct Expr *r);
struct Stmt *parser_new_remassign_stmt(struct Expr *l, struct Expr *r);
struct Stmt *parser_new_inc_stmt(struct Expr *l);
struct Stmt *parser_new_dec_stmt(struct Expr *l);

bool IsGlobal(const struct Expr *e);
bool IsMutable(const struct Expr *e);

const struct Var *FindRootObject(const struct Expr *e);

int Addr(const struct Expr *e);
bool EvalExpr(const struct Expr *e, int64_t *result);
bool EvalAddr(const struct Expr *e, int *result);

#endif // _H
