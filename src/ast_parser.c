#include "../include/ast_parser.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    Token* const* toks;
    size_t ntoks;
    size_t pos;
} Parser;

static Token* peek(Parser* p, size_t offset) {
    size_t i = p->pos + offset;
    if (i >= p->ntoks) return NULL;
    return p->toks[i];
}

static Token* cur(Parser* p) { return peek(p, 0); }

static int is_eof(Token* t) { return t == NULL || t->type == TOKEN_EOF; }

static Token* consume(Parser* p) {
    Token* t = cur(p);
    if (!is_eof(t)) p->pos ++;
    return t;
}

static int is_kw(Token* t, KeywordKind kw) { return t && t->type == TK_KEYWORD && t->kw == kw; }
static int is_punc(Token* t, const char* s) { return t && t->type == TK_PUNCTUATION && t->lex && strcmp(t->lex, s) == 0; }
static int is_op(Token *t, const char* s) { return t && t->type == TK_OPERATOR && strcmp(t->lex, s) == 0; }

static const char* kw_label(KeywordKind kw) {
    switch (kw) {
        case KW_IF: return "IF";
        case KW_ELSE: return "ELSE";
        case KW_FOR: return "FOR";
        case KW_WHILE: return "WHILE";
        case KW_DO: return "DO";
        case KW_SWITCH: return "SWITCH";
        case KW_CASE: return "CASE";
        case KW_DEFAULT: return "DEFAULT";
        case KW_BREAK: return "BREAK";
        case KW_CONTINUE: return "CONTINUE";
        case KW_RETURN: return "RETURN";
        default: return "KW";
    }
}

static const char* token_label(Token* t) {
    if (!t) return "NULL";
    if (t->type == TK_KEYWORD) return kw_label(t->kw);
    if (t->type == TK_IDENT) return t->lex ? t->lex : "ID";
    if (t->type == TK_NUMBER) return "NUM";
    if (t->type == TK_STRING) return "STR";
    if (t->type == TK_CHAR) return "CHR";
    if ((t->type == TK_OPERATOR || t->type == TK_PUNCTUATION) && t->lex) return t->lex;
    return "TOK";
}

static ASTNode* leaf_from_token(Token* t) { return ast_new(AST_TOKEN, token_label(t)); }
static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_block(Parser* p);

static ASTNode* parse_paren_expr(Parser* p) {
    // (...) -> child of expr
    if (!is_punc(cur(p), "(")) return NULL;
    consume(p); // '('

    ASTNode* expr = ast_new(AST_EXPR, NULL);
    if (!expr) return NULL;

    int depth = 1;
    while (!is_eof(cur(p)) && depth > 0) {
        Token* t = consume(p);
        if (!t) break;

        if (t->type == TK_PUNCTUATION && t->lex) {
            if (strcmp(t->lex, ")") == 0) {
                depth --;
                if (depth == 0) break;
            }
        }
        ASTNode* lf = leaf_from_token(t);
        if (!lf || !ast_add_child(expr, lf)) {
            ast_free(lf);
            ast_free(expr);
            return NULL;
        }
    }
    return expr;
}

static ASTNode* parse_until_semicolon(Parser* p, ASTKind kind) {
    ASTNode* st = ast_new(kind, NULL);
    if (!st) return NULL;

    int par = 0, brk = 0; // () []
    while (!is_eof(cur(p))) {
        Token* t = cur(p);

        if (is_punc(t, "(")) par ++;
        else if (is_punc(t, ")")) { if (par > 0) par --; }

        if (is_punc(t, "[")) brk ++;
        else if (is_punc(t, "]")) { if (brk > 0) brk --; }

        if (par == 0 && brk == 0 && is_punc(t, ";")) {
            consume(p);
            break;
        }
        if (par == 0 && brk == 0 && is_punc(t, "{")) {
            break;
        }
        if (par == 0 && brk == 0 && is_punc(t, "}")) {
            break;
        }

        consume(p);
        ASTNode* lf = leaf_from_token(t);
        if (!lf || !ast_add_child(st, lf)) {
            ast_free(lf);
            ast_free(st);
            return NULL;
        }
    }
    return st;
}

static ASTNode* parse_if(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_IF, NULL);
    if (!n) return NULL;

    ASTNode* cond = parse_paren_expr(p);
    if (cond) ast_add_child(n, cond);

    ASTNode* then_st = parse_statement(p);
    if (then_st) ast_add_child(n, then_st);

    if (is_kw(cur(p), KW_ELSE)) {
        consume(p);
        ASTNode* else_node = ast_new(AST_BLOCK, "ELSE");
        if (!else_node) { ast_free(n); return NULL; }
        ASTNode* else_st = parse_statement(p);
        if (else_st) ast_add_child(else_node, else_st);
        ast_add_child(n, else_node);
    }
    return n;
}

static ASTNode* parse_for(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_FOR, NULL);
    if (!n) return NULL;

    ASTNode* head = parse_paren_expr(p);
    if (head) ast_add_child(n, head);

    ASTNode* body = parse_statement(p);
    if (body) ast_add_child(n, body);
    return n;
}

static ASTNode* parse_while(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_WHILE, NULL);
    if (!n) return NULL;

    ASTNode* cond = parse_paren_expr(p);
    if (cond) ast_add_child(n, cond);

    ASTNode* body = parse_statement(p);
    if (body) ast_add_child(n, body);
    return n;
}

static ASTNode* parse_do_while(Parser* p) {
    consume(p);
    ASTNode* n = ast_new(AST_DO_WHILE, NULL);
    if (!n) return NULL;

    ASTNode* body = parse_statement(p);
    if (body) ast_add_child(n, body);

    if (is_kw(cur(p), KW_WHILE)) {
        consume(p);
        ASTNode* cond = parse_paren_expr(p);
        if (cond) ast_add_child(n, cond);
        if (is_punc(cur(p), ";")) consume(p);
    }
    return n;
}

static ASTNode* parse_switch(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_SWITCH, NULL);
    if (!n) return NULL;

    ASTNode* cond = parse_paren_expr(p);
    if (cond) ast_add_child(n, cond);

    ASTNode* body = parse_statement(p);
    if (body) ast_add_child(n, body);
    return n;
}

static ASTNode* parse_case(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_CASE, NULL);
    if (!n) return NULL;

    ASTNode* expr = ast_new(AST_EXPR, NULL);
    if (!expr) return NULL;

    while (!is_eof(cur(p)) && !is_punc(cur(p), ":") && !is_punc(cur(p), "{") && !is_punc(cur(p), "}")) {
        Token* t = consume(p);
        ASTNode* lf = leaf_from_token(t);
        if (!lf || !ast_add_child(expr, lf)) {
            ast_free(lf);
            ast_free(expr);
            return NULL;
        }
    }
    if (is_punc(cur(p), ":")) consume(p);
    ast_add_child(n, expr);

    ASTNode* body = ast_new(AST_BLOCK, "CASE BODY");
    if (!body) return NULL;

    while (!is_eof(cur(p)) && !is_kw(cur(p), KW_CASE) && !is_kw(cur(p), KW_DEFAULT) && !is_punc(cur(p), "}")) {
        ASTNode* st = parse_statement(p);
        if (st) ast_add_child(body, st);
        else consume(p);
    }
    ast_add_child(n, body);
    return n;
}

static ASTNode* parse_default(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_DEFAULT, NULL);
    if (!n) return NULL;
    if (is_punc(cur(p), ":")) consume(p);

    ASTNode* body = ast_new(AST_BLOCK, "DEFAULT BODY");
    if (!body) { ast_free(n); return NULL; }

    while (!is_eof(cur(p)) && !is_kw(cur(p), KW_CASE) && !is_kw(cur(p), KW_DEFAULT) && !is_punc(cur(p), "}")) {
        ASTNode* st = parse_statement(p);
        if (st) ast_add_child(body, st);
        else consume(p);
    }
    ast_add_child(n, body);
    return n;
}

static ASTNode* parse_return(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_RETURN, NULL);
    if (!n) return NULL;

    ASTNode* expr = parse_until_semicolon(p, AST_EXPR);
    if (expr) ast_add_child(n, expr);
    return n;
}

static ASTNode* parse_break(Parser* p) {
    consume(p);
    ASTNode* n = ast_new(AST_BREAK, NULL);
    if (is_punc(cur(p), ";")) consume(p);
    return n;
}

static ASTNode* parse_continue(Parser* p) {
    consume(p);
    ASTNode* n = ast_new(AST_CONTINUE, NULL);
    if (is_punc(cur(p), ";")) consume(p);
    return n;
}

static ASTNode* parse_block(Parser *p) {
    if (!is_punc(cur(p), "{")) return NULL;
    consume(p); // '{'

    ASTNode* b = ast_new(AST_BLOCK, NULL);
    if (!b) return NULL;

    while (!is_eof(cur(p)) && !is_punc(cur(p), "}")) {
        ASTNode* st = parse_statement(p);
        if (st) ast_add_child(b, st);
        else consume(p);
    }

    if (is_punc(cur(p), "}")) consume(p); // '}'
    return b;
}


static ASTNode* parse_statement(Parser* p) {
    Token* t = cur(p);
    if (is_eof(t)) return NULL;

    if (is_punc(t, "{")) return parse_block(p);

    if (is_kw(t, KW_IF)) return parse_if(p);
    if (is_kw(t, KW_FOR)) return parse_for(p);
    if (is_kw(t, KW_WHILE)) return parse_while(p);
    if (is_kw(t, KW_DO)) return parse_do_while(p);
    if (is_kw(t, KW_SWITCH)) return parse_switch(p);
    if (is_kw(t, KW_CASE)) return parse_case(p);
    if (is_kw(t, KW_DEFAULT)) return parse_default(p);
    if (is_kw(t, KW_RETURN)) return parse_return(p);
    if (is_kw(t, KW_BREAK)) return parse_break(p);
    if (is_kw(t, KW_CONTINUE)) return parse_continue(p);

    return parse_until_semicolon(p, AST_STMT);
}

static int looks_like_function(Parser* p) {
    size_t i = p->pos;
    int par = 0;
    int saw_l = 0, saw_r = 0;

    while (i < p->ntoks) {
        Token* t = p->toks[i];
        if (!t || t->type == TOKEN_EOF) return 0;

        if (par == 0 && is_punc(t, ";")) return 0; // expr or indent
        if (is_punc(t, "(")) { par ++; saw_l = 1; }
        else if (is_punc(t, ")")) {
            if (par > 0) par --;
            if (par == 0 && saw_l) saw_r = 1;
        } else if (par == 0 && is_punc(t, "{")) {
            return saw_l && saw_r;
        }
        i ++;
    }
    return 0;
}

static ASTNode* parse_function(Parser *p) {
    ASTNode* fn = ast_new(AST_FUNCTION, NULL);
    if (!fn) return NULL;

    ASTNode* header = ast_new(AST_STMT, "FUNC_HEADER");
    if (!header) { ast_free(fn); return NULL; }

    while (!is_eof(cur(p)) && !is_punc(cur(p), "{")) {
        Token* t = consume(p);
        ASTNode* lf = leaf_from_token(t);
        if (!lf || !ast_add_child(header, lf)) {
            ast_free(lf);
            ast_free(header);
            ast_free(fn);
            return NULL;
        }
    }
    ast_add_child(fn, header);

    ASTNode* body = parse_block(p);
    if (body) ast_add_child(fn, body);
    return fn;
}

ASTNode* ast_parse_tokens(Token* const* toks, size_t ntoks) {
    Parser p = { toks, ntoks, 0 };

    ASTNode* root = ast_new(AST_PROGRAM, NULL);
    if (!root) return NULL;

    while (!is_eof(cur(&p))) {
        ASTNode* node = NULL;
        if (looks_like_function(&p)) node = parse_function(&p);
        else node = parse_statement(&p);

        if (node) ast_add_child(root, node);
        else consume(&p);
    }
    return root;
}