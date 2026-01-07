/**
* @file ast_serial.h
 * @brief AST 序列化接口：将树结构转换为字符串序列（StrVec）。
 */

#pragma once

#ifndef COURSEDESIGNTASKS_AST_SERIAL_H
#define COURSEDESIGNTASKS_AST_SERIAL_H

#include <stddef.h>
#include <stdbool.h>
#include "ast.h"

/**
 * @brief 简单的字符串向量（动态数组）。
 *
 * data[i] 为以 '\0' 结尾的字符串指针；StrVec 持有其所有权。
 */
typedef struct {
    char** data;
    size_t size;
    size_t cap;
} StrVec;

void  sv_init(StrVec* v);
bool  sv_push(StrVec* v, const char* s);
void  sv_free(StrVec* v);

bool  ast_serialize_preorder(const ASTNode* root, StrVec* out);

#endif //COURSEDESIGNTASKS_AST_SERIAL_H