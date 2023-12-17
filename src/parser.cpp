#include "parser.h"
#include "error.h"
#include <iostream>

Node *Parser::Parse(const std::string &src)
{
    src_ = &src;
    lexer_.SetInput(src);

    return program();
}

Token *Parser::next() const
{
    if (curr_ == end_)
        return begin_;
    else
        return curr_ + 1;
}

Token *Parser::prev() const
{
    if (curr_ == begin_)
        return end_;
    else
        return curr_ - 1;
}

const Token *Parser::curtok() const
{
    return curr_;
}

const Token *Parser::gettok()
{
    if (curr_ != head_) {
        curr_ = next();
        return curr_;
    }
    else {
        curr_ = next();
        lexer_.Get(curr_);
        head_ = curr_;
        return curr_;
    }
}

void Parser::ungettok()
{
    curr_ = prev();
}

long Parser::tok_int() const
{
    return curr_->ival;
}

double Parser::tok_float() const
{
    return curr_->fval;
}

std::string_view Parser::tok_str() const
{
    return curr_->sval;
}

TokenKind Parser::peek()
{
    const Token *tok = gettok();
    const TokenKind kind = tok->kind;
    ungettok();
    return kind;
}

void Parser::expect(TokenKind kind)
{
    const Token *tok = gettok();
    if (tok->kind != kind) {
        const std::string msg =
            std::string("expected '") + GetTokenKindString(kind) + "'";
        Error(msg, *src_, tok->pos);
    }
}

bool Parser::consume(TokenKind kind)
{
    if (peek() == kind) {
        gettok();
        return true;
    }
    else {
        return false;
    }
}

void Parser::enter_scope(Func *func)
{
    if (func) {
        scope_ = func->scope;
    }
    else {
        scope_ = scope_->OpenChild();
    }
}

void Parser::enter_scope(Class *clss)
{
    scope_ = clss->scope;
}

void Parser::leave_scope()
{
    scope_ = scope_->Close();
}

CallExpr *Parser::arg_list(CallExpr *call)
{
    expect(TK::LPAREN);

    if (consume(TK::RPAREN))
        return call;

    do {
        call->AddArg(expression());
    }
    while (consume(TK::COMMA));

    expect(TK::RPAREN);
    return call;
}

// primary_expr =
//     IntNum |
//     FpNum |
//     StringLit |
//     primary_expr selector
Expr *Parser::primary_expr()
{
    if (consume(TK::INTLIT))
        return new IntNumExpr(tok_int(), new Type(TY::Integer));

    if (consume(TK::FLTLIT))
        return new FpNumExpr(tok_float(), new Type(TY::Float));

    if (consume(TK::STRLIT))
        return new StringLitExpr(tok_str(), new Type(TY::String));

    if (consume(TK::LPAREN)) {
        Expr *expr = expression();
        expect(TK::RPAREN);
        return expr;
    }

    Expr *expr = nullptr;

    for (;;) {
        const Token *tok = gettok();

        if (tok->kind == TK::IDENT) {
            if (peek() == TK::LPAREN) {
                Func *func = FindBuiltinFunc(tok->sval);
                if (!func) {
                    func = scope_->FindFunc(tok->sval);
                    if (!func) {
                        std::cerr << "error: undefined identifier: '" <<
                            tok->sval << "'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }

                CallExpr *call = new CallExpr(func);
                expr = arg_list(call);
            }
            else {
                Var *var = scope_->FindVar(tok->sval);
                if (!var) {
                    std::cerr << "error: undefined identifier: '" <<
                        tok->sval << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }

                expr = new IdentExpr(var);
            }
            continue;
        }
        else if (tok->kind == TK::PERIOD) {
            // TODO FIX
            if (!expr || !expr->type) {
                std::cerr << "error: no type" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (expr->type->kind != TY::ClassType) {
                std::cerr << "error: not a class type" << std::endl;
                exit(EXIT_FAILURE);
            }

            expect(TK::IDENT);

            Field *fld = expr->type->clss->FindField(tok_str());

            expr = new SelectExpr(expr, new FieldExpr(fld));
            continue;
        }
        else {
            if (!expr)
                Error("Unknown token", *src_, tok->pos);

            ungettok();
            return expr;
        }
    }

    return nullptr;
}

// unary_expr = primary_expr (unary_op primary_expr)*
// unary_op   = "+" | "-" | "!" | "~"
Expr *Parser::unary_expr()
{
    const Token *tok = gettok();

    switch (tok->kind) {
    case TK::PLUS:
    case TK::MINUS:
    case TK::EXCL:
    case TK::TILDA:
        return new UnaryExpr(tok->kind, unary_expr());

    default:
        ungettok();
        return primary_expr();
    }
}

// mul_expr = unary_expr (mul_op unary_expr)*
// mul_op   = "*" | "/" | "%" | "&" | "<<" | ">>"
Expr *Parser::mul_expr()
{
    Expr *expr = unary_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TK::STAR:
        case TK::SLASH:
        case TK::PERCENT:
        case TK::AMP:
        case TK::LT2:
        case TK::GT2:
            expr = new BinaryExpr(tok->kind, expr, unary_expr());
            break;

        default:
            ungettok();
            return expr;
        }
    }
}

// add_expr = mul_expr (add_op mul_expr)*
// add_op   = "+" | "-" | "|" | "^"
Expr *Parser::add_expr()
{
    Expr *expr = mul_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TK::PLUS:
        case TK::MINUS:
        case TK::BAR:
        case TK::CARET:
            expr = new BinaryExpr(tok->kind, expr, mul_expr());
            break;

        default:
            ungettok();
            return expr;
        }
    }
}

// rel_expr = add_expr (rel_op add_expr)*
// rel_op   = "==" | "!=" | "<" | ">" | "<=" | ">="
Expr *Parser::rel_expr()
{
    Expr *expr = add_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::EQ2:
        case TK::EXCLEQ:
        case TK::LT:
        case TK::GT:
        case TK::LTE:
        case TK::GTE:
            expr = new BinaryExpr(tok->kind, expr, add_expr());
            continue;

        default:
            ungettok();
            return expr;
        }
    }
}

// logand_expr = rel_expr ("&&" rel_expr)*
Expr *Parser::logand_expr()
{
    Expr *expr = rel_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::AMP2:
            expr = new BinaryExpr(tok->kind, expr, rel_expr());
            continue;

        default:
            ungettok();
            return expr;
        }
    }
}

// logor_expr = logand_expr ("||" logand_expr)*
Expr *Parser::logor_expr()
{
    Expr *expr = logand_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::BAR2:
            expr = new BinaryExpr(tok->kind, expr, logand_expr());
            continue;

        default:
            ungettok();
            return expr;
        }
    }
}

// assign_expr = logand_expr assing_op expression
//             | logand_expr incdec_op
// assign_op   = "="
// incdec_op   = "++" | "--"
Expr *Parser::assign_expr()
{
    Expr *expr = logor_expr();
    const Token *tok = gettok();

    switch (tok->kind) {
    case TK::EQ:
        return new AssignExpr(expr, expression());

    case TK::PLUS2:
    case TK::MINUS2:
        return new IncDecExpr(tok->kind, expr);

    default:
        ungettok();
        return expr;
    }
}

Expr *Parser::expression()
{
    return assign_expr();
}

Stmt *Parser::if_stmt()
{
    expect(TK::IF);
    Expr *cond = expression();
    expect(TK::NEWLINE);

    enter_scope();
    BlockStmt *then = block_stmt();
    leave_scope();

    BlockStmt *els = nullptr;
    if (consume(TK::ELSE)) {
        expect(TK::NEWLINE);

        enter_scope();
        els = block_stmt();
        leave_scope();
    }

    return new IfStmt(cond, then, els);
}

Stmt *Parser::for_stmt()
{
    expect(TK::FOR);

    Expr *init = nullptr;
    Expr *cond = nullptr;
    Expr *post = nullptr;

    if (consume(TK::NEWLINE)) {
        // infinite loop
        init = new NullExpr();
        cond = new IntNumExpr(1, new Type(TY::Integer));
        post = new NullExpr();
    }
    else {
        Expr *e = expression();

        if (consume(TK::SEMICOLON)) {
            // traditional for
            init = e;
            cond = expression();
            expect(TK::SEMICOLON);
            post = expression();
            expect(TK::NEWLINE);
        }
        else if (consume(TK::NEWLINE)) {
            // while style
            init = new NullExpr();
            cond = e;
            post = new NullExpr();
        }
        else {
            const Token *tok = gettok();
            Error("Unknown token", *src_, tok->pos);
        }
    }

    // body
    enter_scope();
    BlockStmt *body = block_stmt();
    leave_scope();

    return new ForStmt(init, cond, post, body);
}

Stmt *Parser::jump_stmt()
{
    const Token *tok = gettok();
    const TK kind = tok->kind;

    if (kind == TK::BREAK) {
    }
    else if (kind == TK::CONTINUE) {
    }

    expect(TK::NEWLINE);

    return new JumpStmt(kind);
}

Stmt *Parser::switch_stmt()
{
    expect(TK::SWITCH);

    Expr *expr = expression();
    expect(TK::NEWLINE);

    SwitchStmt *swtch = new SwitchStmt(expr);

    int default_count = 0;

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::CASE:
            if (default_count > 0) {
                Error("No 'case' should come after 'default'",
                        *src_, tok->pos);
            }
            swtch->AddCase(case_stmt(tok->kind));
            continue;

        case TK::DEFAULT:
            swtch->AddCase(case_stmt(tok->kind));
            default_count++;
            continue;

        default:
            return swtch;
        }
    }
}

CaseStmt *Parser::case_stmt(TK kind)
{
    Expr *expr = nullptr;

    if (kind == TK::CASE)
        expr = expression();
    else if (kind == TK::DEFAULT)
        expr = new NullExpr();

    expect(TK::NEWLINE);

    BlockStmt *block = block_stmt();
    return new CaseStmt(kind, expr, block);
}

Stmt *Parser::ret_stmt()
{
    expect(TK::RETURN);

    ReturnStmt *stmt = nullptr;

    if (consume(TK::NEWLINE)) {
        stmt = new ReturnStmt();
    }
    else {
        stmt = new ReturnStmt(expression());
        expect(TK::NEWLINE);
    }

    return stmt;
}

Stmt *Parser::expr_stmt()
{
    ExprStmt *stmt = new ExprStmt(expression());
    expect(TK::NEWLINE);

    return stmt;
}

// var_decl = "-" identifier type newline
//          | "-" identifier type = expression newline
Stmt *Parser::var_decl()
{
    expect(TK::MINUS);
    expect(TK::IDENT);

    if (scope_->FindVar(tok_str())) {
        const Token *tok = curtok();
        const std::string msg = "re-defined variable: '" +
            std::string(tok_str()) + "'";
        Error(msg, *src_, tok->pos);
    }

    // var
    Var *var = scope_->DefineVar(tok_str());
    Expr *init = nullptr;

    if (consume(TK::EQ)) {
        // "- x = 42"
        init = expression();
        var->type = DuplicateType(init->type);
    }
    else {
        var->type = type_spec();

        if (consume(TK::EQ)) {
            // "- x int = 42"
            init = expression();
        }
        else {
            // "- x int"
            if (var->type->IsInteger())
                init = new IntNumExpr(0, new Type(TY::Integer));
            else if (var->type->IsFloat())
                init = new FpNumExpr(0.0f, new Type(TY::Float));
            else if (var->type->IsString())
                init = new StringLitExpr("", new Type(TY::String));
            else
                // TODO Class type
                init = new IntNumExpr(0, new Type(TY::Integer));
        }
    }

    expect(TK::NEWLINE);

    Expr *ident = new IdentExpr(var);
    return new ExprStmt(new AssignExpr(ident, init));
}

Field *Parser::field_decl()
{
    expect(TK::MINUS);
    expect(TK::IDENT);

    if (scope_->FindField(tok_str())) {
        std::cerr
            << "error: re-defined variable: '"
            << tok_str() << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Field *fld = scope_->DefineFild(tok_str());
    fld->type = type_spec();
    expect(TK::NEWLINE);

    return fld;
}

Class *Parser::class_decl()
{
    expect(TK::HASH2);
    expect(TK::IDENT);

    // class name
    Class *clss = scope_->DefineClass(tok_str());
    if (!clss) {
        std::cerr << "error: re-defined class: '" << tok_str() << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    expect(TK::NEWLINE);
    enter_scope(clss);
    expect(TK::BLOCKBEGIN);
    field_list(clss);
    expect(TK::BLOCKEND);
    leave_scope();

    return nullptr;
    //return new ClasDef(clss);
}

BlockStmt *Parser::block_stmt()
{
    BlockStmt *block = new BlockStmt();
    expect(TK::BLOCKBEGIN);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::MINUS) {
            block->AddStmt(var_decl());
            continue;
        }
        else if (next == TK::IF) {
            block->AddStmt(if_stmt());
            continue;
        }
        else if (next == TK::FOR) {
            block->AddStmt(for_stmt());
            continue;
        }
        else if (next == TK::BREAK || next == TK::CONTINUE) {
            block->AddStmt(jump_stmt());
            continue;
        }
        else if (next == TK::SWITCH) {
            block->AddStmt(switch_stmt());
            continue;
        }
        else if (next == TK::RETURN) {
            block->AddStmt(ret_stmt());
            continue;
        }
        else if (next == TK::BLOCKEND) {
            break;
        }
        else {
            block->AddStmt(expr_stmt());
            continue;
        }
    }

    expect(TK::BLOCKEND);
    return block;
}

// type_spec = "int" | "float" | "string" | identifier
Type *Parser::type_spec()
{
    if (consume(TK::INT))
        return new Type(TY::Integer);

    if (consume(TK::FLOAT))
        return new Type(TY::Float);

    if (consume(TK::STRING))
        return new Type(TY::String);

    if (consume(TK::IDENT)) {
        Type *ty = new Type(TY::ClassType);
        ty->clss = scope_->FindClass(tok_str());
        return ty;
    }

    const Token *tok = gettok();

    std::cerr << "error: not a type name: '" << tok->kind << "'" << std::endl;
    std::exit(EXIT_FAILURE);

    return nullptr;
}

void Parser::field_list(Class *clss)
{
    expect(TK::MINUS);

    do {
        expect(TK::IDENT);
        std::string_view name = tok_str();

        clss->DeclareField(name, type_spec());
        expect(TK::NEWLINE);
    }
    while (consume(TK::MINUS));
}

void Parser::param_list(Func *func)
{
    expect(TK::LPAREN);

    if (consume(TK::RPAREN))
        return;

    do {
        expect(TK::IDENT);
        std::string_view name = tok_str();

        func->DeclareParam(name, type_spec());
    }
    while (consume(TK::COMMA));

    expect(TK::RPAREN);
}

FuncDef *Parser::func_def()
{
    expect(TK::HASH);
    expect(TK::IDENT);

    // func name
    Func *func = scope_->DefineFunc(tok_str());
    if (!func) {
        std::cerr << "error: re-defined function: '" << tok_str() << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    enter_scope(func);

    param_list(func);
    func->type = type_spec();
    expect(TK::NEWLINE);

    // func body
    BlockStmt *block = block_stmt();

    leave_scope();

    return new FuncDef(func, block);
}

Prog *Parser::program()
{
    Prog *prog = new Prog(scope_);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::HASH) {
            FuncDef *fdef = func_def();

            if (fdef->func->name == "main")
                prog->main_func = fdef->func;

            prog->AddFuncDef(fdef);
            continue;
        }

        if (next == TK::HASH2) {
            class_decl();
            continue;
        }

        if (next == TK::MINUS) {
            prog->AddGlobalVar(var_decl());
            continue;
        }

        if (next == TK::EOF_) {
            break;
        }

        std::cerr << "error: unexpected token: '" << next << "'" << std::endl;
        exit(EXIT_FAILURE);
    }

    return prog;
}
