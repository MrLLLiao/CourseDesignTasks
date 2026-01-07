/**
* @file ast.h
 * @brief AST 数据结构与基础操作接口。
 *
 * 该 AST 不是完整 C 语法树，而是为“相似度检测”设计的简化结构：
 * - 重点表达控制结构、函数、块；
 * - 其它细节以 AST_TOKEN 叶子形式保留（可配合变量名归一化等预处理）。
 */


#pragma once

#ifndef COURSEDESIGNTASKS_AST_H
#define COURSEDESIGNTASKS_AST_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief AST 节点类别（用于描述语法结构类型）。
 *
 * 说明：
 * - AST_TOKEN 为叶子节点，text 字段保存 token 标签/文本；
 * - 其它类型通常为内部结构节点，text 可为空或用于调试标记。
 */
typedef enum {
    AST_PROGRAM = 0,
    AST_FUNCTION,
    AST_BLOCK,

    AST_IF,
    AST_FOR,
    AST_WHILE,
    AST_DO_WHILE,
    AST_SWITCH,
    AST_CASE,
    AST_DEFAULT,

    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,

    AST_STMT,   // 普通语句
    AST_EXPR,   // 括号表达式 or case 表达式
    AST_TOKEN   // 叶子：保存一个 token 的“标签”
} ASTKind;

/**
 * @brief AST 节点结构。
 *
 * children 为动态数组，child_count 为当前子节点数，child_cap 为容量。
 * 所有权：节点及其 children/text 均由 AST 模块管理，使用 ast_free 递归释放。
 */
typedef struct ASTNode {
    ASTKind kind;
    char* text;
    struct ASTNode** children;
    size_t child_count;
    size_t child_cap;
} ASTNode;

/** @brief 创建 AST 节点（见 ast.c 具体说明）。 */
ASTNode* ast_new(ASTKind kind, const char* text);
/** @brief 追加子节点（见 ast.c 具体说明）。 */
bool     ast_add_child(ASTNode* parent, ASTNode* child);
/** @brief 释放整棵树（见 ast.c 具体说明）。 */
void     ast_free(ASTNode* node);

/** @brief 将 ASTKind 转为字符串标签。 */
const char* ast_kind_name(ASTKind kind);

/** @brief 调试打印 AST（前序遍历）。 */
void ast_dump(const ASTNode* node, int indent);

#endif //COURSEDESIGNTASKS_AST_H