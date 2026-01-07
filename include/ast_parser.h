/**
* @file ast_parser.h
 * @brief Token -> AST 的解析接口。
 */

#pragma once

#ifndef COURSEDESIGNTASKS_AST_PARSER_H
#define COURSEDESIGNTASKS_AST_PARSER_H

#include <stddef.h>
#include "std_token.h"
#include "ast.h"

/**
 * @brief 将 Token 数组解析为 AST 根节点（AST_PROGRAM）。
 *
 * @param toks  token 指针数组。
 * @param ntoks token 数量（建议包含 EOF）。
 * @return AST 根节点（用 ast_free 释放）。
 */
ASTNode* ast_parse_tokens(Token* const* toks, size_t ntoks);

#endif //COURSEDESIGNTASKS_AST_PARSER_H