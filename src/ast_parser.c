/**
* @file ast_parser.c
 * @brief 将 Token 序列解析为“查重友好”的简化 AST。
 *
 * 目标是保留控制结构/函数/块等结构信息，提升对空白、换行等文本变形的鲁棒性。
 * 该解析器并不追求完整覆盖 C 语法，而是面向课程设计的相似度检测场景做取舍。
 */
#include "../include/ast_parser.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    Token* const* toks;
    size_t ntoks;
    size_t pos;
} Parser;

/**
 * @brief 向前查看 offset 个 token（不移动指针）。
 *
 * @param p      解析器状态。
 * @param offset 偏移量（0 表示当前 token）。
 * @return token 指针；越界返回 NULL。
 */
static Token* peek(Parser* p, size_t offset) {
    size_t i = p->pos + offset;
    if (i >= p->ntoks) return NULL;
    return p->toks[i];
}

/**
 * @brief 获取当前 token（peek(p,0) 的便捷封装）。
 */
static Token* cur(Parser* p) { return peek(p, 0); }

/**
 * @brief 判断 token 是否为 EOF（或 NULL）。
 */
static int is_eof(Token* t) { return t == NULL || t->type == TOKEN_EOF; }

/**
 * @brief 读取并消费一个 token（将 pos 前移）。
 *
 * @param p 解析器状态。
 * @return 被消费的 token；若已到 EOF 则返回当前/NULL。
 */
static Token* consume(Parser* p) {
    Token* t = cur(p);
    if (!is_eof(t)) p->pos ++;
    return t;
}

/**
 * @brief 判断当前 token 是否为指定关键字。
 */
static int is_kw(Token* t, KeywordKind kw) { return t && t->type == TK_KEYWORD && t->kw == kw; }
/**
 * @brief 判断当前 token 是否为指定标点（分隔符），如 "(", "{", ";" 等。
 */
static int is_punc(Token* t, const char* s) { return t && t->type == TK_PUNCTUATION && t->lex && strcmp(t->lex, s) == 0; }
/**
 * @brief 判断当前 token 是否为指定运算符，如 "+", "==" 等。
 */
static int is_op(Token *t, const char* s) { return t && t->type == TK_OPERATOR && strcmp(t->lex, s) == 0; }

/**
 * @brief 将关键字枚举映射为稳定的标签字符串（用于 AST_TOKEN 叶子）。
 *
 * 目的：让“语义相同但书写不同”的代码（如不同空格/换行）在序列化后更稳定。
 */
static const char* kw_label(KeywordKind kw) {
    switch (kw) {
        case KW_IF:         return "IF";
        case KW_ELSE:       return "ELSE";
        case KW_FOR:        return "FOR";
        case KW_WHILE:      return "WHILE";
        case KW_DO:         return "DO";
        case KW_SWITCH:     return "SWITCH";
        case KW_CASE:       return "CASE";
        case KW_DEFAULT:    return "DEFAULT";
        case KW_BREAK:      return "BREAK";
        case KW_CONTINUE:   return "CONTINUE";
        case KW_RETURN:     return "RETURN";
        default:            return "KW";
    }
}

/**
 * @brief 将 Token 归一化为用于 AST 叶子节点的“标签”。
 *
 * 策略：
 * - 关键字：用关键字标签（IF/FOR/RETURN...）；
 * - 标识符：保留 lex（由其他模块可先做变量名归一化）；
 * - 常量：统一为 NUM/STR/CHR；
 * - 运算符与分隔符：直接用其字面量。
 */
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

/**
 * @brief 将一个 Token 包装成 AST_TOKEN 叶子节点。
 */
static ASTNode* leaf_from_token(Token* t) { return ast_new(AST_TOKEN, token_label(t)); }
static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_block(Parser* p);

/**
 * @brief 解析括号表达式 "( ... )" 为 AST_EXPR。
 *
 * 实现说明：
 * - 不做完整表达式语法树（为查重任务保留结构/符号即可）；
 * - 通过 depth 计数处理嵌套括号，直到匹配到最外层 ')'.
 *
 * @return 成功返回 AST_EXPR；失败返回 NULL。
 */
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

/**
 * @brief 解析一段“以分号结束的语句片段”或在遇到 '{' / '}' 时提前停止。
 *
 * 该函数用于：
 * - 普通语句（AST_STMT）：一直收集 token 直到 ';'；
 * - return 表达式（AST_EXPR）：return 后面的表达式片段。
 *
 * 细节：
 * - par/brk 计数用于忽略括号/中括号内部的 ';'；
 * - 在 block 边界 '{' 或 '}' 处停止，避免跨语句吞噬。
 */
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

/**
 * @brief 解析 if 语句：IF + 条件括号 + then 语句 + 可选 else 分支。
 */
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

/**
 * @brief 解析 for 语句：FOR + 头部括号 + 循环体语句。
 */
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

/**
 * @brief 解析 while 语句：WHILE + 条件括号 + 循环体语句。
 */
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

/**
 * @brief 解析 do-while 语句：DO + 循环体语句 + WHILE(条件) + ';'。
 */
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

/**
 * @brief 解析 switch 语句：SWITCH + 条件括号 + 语句体（通常是 block）。
 */
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

/**
 * @brief 解析 case 分支：CASE + 表达式 + ':' + 分支体语句序列。
 *
 * 解析策略：
 * - ':' 之前收集为 AST_EXPR；
 * - ':' 之后直到遇到下一个 case/default 或 '}' 为止，作为 CASE BODY。
 */
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

/**
 * @brief 解析 default 分支：DEFAULT + ':' + 分支体语句序列。
 */
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

/**
 * @brief 解析 return：RETURN + 可选表达式片段（直到 ';'）。
 */
static ASTNode* parse_return(Parser *p) {
    consume(p);
    ASTNode* n = ast_new(AST_RETURN, NULL);
    if (!n) return NULL;

    ASTNode* expr = parse_until_semicolon(p, AST_EXPR);
    if (expr) ast_add_child(n, expr);
    return n;
}

/**
 * @brief 解析 break 语句（可选消费 ';'）。
 */
static ASTNode* parse_break(Parser* p) {
    consume(p);
    ASTNode* n = ast_new(AST_BREAK, NULL);
    if (is_punc(cur(p), ";")) consume(p);
    return n;
}

/**
 * @brief 解析 continue 语句（可选消费 ';'）。
 */
static ASTNode* parse_continue(Parser* p) {
    consume(p);
    ASTNode* n = ast_new(AST_CONTINUE, NULL);
    if (is_punc(cur(p), ";")) consume(p);
    return n;
}

/**
 * @brief 解析代码块 "{ ... }" 为 AST_BLOCK，内部递归解析若干 statement。
 */
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

/**
 * @brief 解析一条语句（statement）的入口分发器。
 *
 * 优先级：
 * - '{' -> block；
 * - 关键字控制结构（if/for/while/do/switch/case/default/return/break/continue）；
 * - 其他情况 -> 普通语句（直到 ';'）。
 */
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

/**
 * @brief 启发式判断当前位置是否像“函数定义”。
 *
 * 判定思路：
 * - 向前扫描：必须看到匹配的一对 '(' 与 ')'；
 * - 在最外层括号之后，遇到 '{' 才认为是函数（而不是声明/表达式）；
 * - 若在最外层括号前先遇到 ';'，则排除（更像语句/声明结束）。
 *
 * @return 像函数返回 1，否则 0。
 */
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

/**
 * @brief 解析函数定义：函数头（直到 '{'）+ 函数体 block。
 *
 * 为查重任务保留“函数这个结构节点”，但不细分返回类型/参数等语义；
 * 头部 token 统一收集到一个 AST_STMT("FUNC_HEADER") 子节点中。
 */
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

/**
 * @brief 解析 token 序列为 AST 根节点（AST_PROGRAM）。
 *
 * 设计目标：为“代码相似度检测”提供结构化表示，而不是完整 C 语法。
 * 因此：
 * - 只对控制结构、函数、块做显式树节点；
 * - 其余部分以 token 叶子序列保留（对变量名归一化等更鲁棒）。
 *
 * @param toks  token 指针数组（每个元素为 Token*）。
 * @param ntoks token 数量（包含 EOF）。
 * @return AST 根节点；失败返回 NULL。
 */
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