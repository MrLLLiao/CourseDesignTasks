/**
* @file std_token.h
 * @brief 小组共享的 Token 定义（供 AST 解析模块使用）。
 */

#pragma once

#ifndef COURSEDESIGNTASKS_STD_TOKEN_H
#define COURSEDESIGNTASKS_STD_TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,      // 终止
    TK_IDENT,       // 变量名、函数名等
    TK_KEYWORD,     // 关键字
    TK_NUMBER,      // 数字常量
    TK_STRING,      // 字符串
    TK_CHAR,        // 字符
    TK_OPERATOR,    // 操作运算符
    TK_PUNCTUATION  // 分隔符
} TokenType;

typedef enum { // 关键字集合
    KW_IF, KW_ELSE, KW_FOR, KW_WHILE, KW_DO, KW_SWITCH, KW_CASE, KW_DEFAULT,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    KW_INT, KW_CHAR, KW_FLOAT, KW_DOUBLE, KW_VOID, KW_STRUCT, KW_TYPEDEF,

    KW_UNKNOWN
} KeywordKind;

typedef struct Token {
    TokenType type;
    KeywordKind kw;     // kind == TK_KEYWORD 时有效，其它为 KW_UNKNOWN
    char *lex;          // 归一化后字符串
    char *raw;          // 原始字符串
    int line, col;
} Token;

void token_free(Token *tok);

#endif //COURSEDESIGNTASKS_STD_TOKEN_H