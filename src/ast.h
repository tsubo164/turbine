#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"
#include "lexer.h"

struct Type;

struct Node {
    Node() {}
    virtual ~Node() {}
    virtual void Print(int depth = 0) const = 0;
    virtual void Gen(Bytecode &code) const = 0;
};

struct Expr : public Node {
    Expr(const Type *t) : type(t) {}
    const Type *type = nullptr;

    virtual int Addr() const { return -1; }
    virtual bool IsGlobal() const { return false; }
};

struct NullExpr : public Expr {
    NullExpr();
    void Print(int depth) const override final {}
    void Gen(Bytecode &code) const override final {}
};

struct IntNumExpr : public Expr {
    IntNumExpr(long n, const Type *t) : Expr(t), ival(n) {}
    long ival;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct FpNumExpr : public Expr {
    FpNumExpr(double n, const Type *t) : Expr(t), fval(n) {}
    double fval;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct StringLitExpr : public Expr {
    StringLitExpr(std::string_view s, const Type *t) : Expr(t), sval(s) {}
    std::string_view sval;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IdentExpr : public Expr {
    IdentExpr(const Var *v) : Expr(v->type), var(v) {}
    const Var *var;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;

    int Addr() const override;
    bool IsGlobal() const override { return var->is_global; }
};

struct FieldExpr : public Expr {
    FieldExpr(const Field *f) : Expr(f->type), fld(f) {}
    const Field *fld;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;

    int Addr() const override { return fld->id; }
};

struct SelectExpr : public Expr {
    SelectExpr(Expr *i, Expr *f) : Expr(f->type), inst(i), fld(f) {}
    Expr *inst;
    Expr *fld;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;

    int Addr() const override { return inst->Addr() + fld->Addr(); }
    bool IsGlobal() const override { return inst->IsGlobal(); }
};

struct CallExpr : public Expr {
    CallExpr(Func *f) : Expr(f->type), func(f) {}
    void AddArg(Expr *e) { args.push_back(e); }
    std::vector<Expr*> args;
    const Func *func;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct BinaryExpr : public Expr {
    BinaryExpr(TokenKind Kind, Expr *L, Expr *R);
    TokenKind kind;
    std::unique_ptr<Expr> l;
    std::unique_ptr<Expr> r;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct UnaryExpr : public Expr {
    UnaryExpr(TokenKind Kind, Expr *R);
    TokenKind kind;
    std::unique_ptr<Expr> r;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct AssignExpr : public Expr {
    AssignExpr(Expr *l, Expr *r);
    std::unique_ptr<Expr> lval;
    std::unique_ptr<Expr> rval;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IncDecExpr : public Expr {
    IncDecExpr(TokenKind k, Expr *e) : Expr(e->type), kind(k), lval(e) {}
    TokenKind kind;
    std::unique_ptr<Expr> lval;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct Stmt : public Node {
};

struct BlockStmt : public Stmt {
    BlockStmt() {}
    ~BlockStmt()
    {
        for (auto stmt: stmts)
            delete stmt;
    }
    void AddStmt(Stmt *stmt) { stmts.push_back(stmt); }
    std::vector<Stmt*> stmts;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IfStmt : public Stmt {
    IfStmt(Expr *cond_, BlockStmt *then_, BlockStmt *els_)
        : cond(cond_), then(then_), els(els_) {}
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> then;
    std::unique_ptr<BlockStmt> els;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ForStmt : public Stmt {
    ForStmt(Expr *i, Expr *c, Expr *p, BlockStmt *b)
        : init(i), cond(c), post(p), body(b) {}
    std::unique_ptr<Expr> init;
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> post;
    std::unique_ptr<BlockStmt> body;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ReturnStmt : public Stmt {
    ReturnStmt() : expr(new NullExpr()) {}
    ReturnStmt(Expr *e) : expr(e) {}
    std::unique_ptr<Expr> expr;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ExprStmt : public Stmt {
    ExprStmt(Expr *e) : expr(e) {}
    std::unique_ptr<Expr> expr;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct FuncDef : public Node {
    FuncDef(Func *f, BlockStmt *b) : func(f), block(b) {}
    ~FuncDef() {}
    Func *func = nullptr;
    std::unique_ptr<BlockStmt> block;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct Prog: public Node {
    Prog(Scope *sc) : scope(sc) {}
    void AddFuncDef(FuncDef *func) { funcs.push_back(func); }
    std::vector<FuncDef*> funcs;
    const Scope *scope;
    // TODO remove this
    const Func *main_func = nullptr;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

#endif // _H
