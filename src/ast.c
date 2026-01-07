/**
 * @file ast.c
 * @brief AST 节点的创建、管理与调试输出。
 *
 * 本模块提供 ASTNode 的生命周期管理（new/add_child/free）以及调试打印。
 */

#include "../include/ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief strdup 的安全替代实现（可在非 POSIX 环境使用）。
 *
 * @param s 以 '\0' 结尾的 C 字符串；可为 NULL。
 * @return  返回一份堆上拷贝（malloc）；若 s 为 NULL 返回 NULL；内存不足返回 NULL。
 *
 * @note 由调用方负责 free 返回指针。
 */
static char* xstrdup(const char* s) {
    if (!s) return NULL;
    const size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

/**
 * @brief 创建一个新的 AST 节点，并初始化其动态子节点数组。
 *
 * 设计要点：
 * - 统一的 ASTNode 构造入口，避免各处手动初始化遗漏字段；
 * - text 采用深拷贝，防止外部 Token 缓冲区生命周期结束后悬空引用。
 *
 * @param kind 节点类别（语法结构类型）。
 * @param text 可选文本：通常用于 AST_TOKEN（保存 token 标签/文本）。
 * @return 成功返回新节点；失败返回 NULL。
 *
 * @note 返回的节点需使用 ast_free 递归释放。
 */
ASTNode *ast_new(ASTKind kind, const char* text) {
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
    if (!n) return NULL;
    n->kind = kind;
    n->text = text ? xstrdup(text) : NULL;
    n->children = NULL;
    n->child_count = 0;
    n->child_cap = 0;

    if (text && !n->text) {
        free(n);
        return NULL;
    }
    return n;
}

/**
 * @brief 将 child 追加到 parent 的 children 动态数组中。
 *
 * 采用“容量不足则倍增”的策略摊还 O(1) 追加，便于构建任意分叉度的树结构。
 *
 * @param parent 父节点（非 NULL）。
 * @param child  子节点（非 NULL）。
 * @return 追加成功返回 true；失败返回 false（此时不会释放 child）。
 */
bool ast_add_child(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) return false;
    if (parent->child_count == parent->child_cap) {
        size_t new_cap = (parent->child_cap == 0) ? 4 : (parent->child_cap * 2);
        ASTNode **new_child = (ASTNode**)realloc(parent->children, new_cap * sizeof(ASTNode*));
        if (!new_child) return false;
        parent->children = new_child;
        parent->child_cap = new_cap;
    }
    parent->children[parent->child_count++] = child;
    return true;
}

/**
 * @brief 递归释放一棵 AST（后序释放）。
 *
 * @param node 根节点；可为 NULL。
 *
 * @note 释放顺序：先递归 children，再释放 children 数组、text、节点本体。
 */
void ast_free(ASTNode* node) {
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);
    free(node->text);
    free(node);
}

/**
 * @brief 将 ASTKind 枚举转换为可读字符串（用于调试/序列化标签）。
 *
 * @param kind ASTKind 枚举值。
 * @return 对应的常量字符串指针（静态存储期）。
 */
const char* ast_kind_name(ASTKind kind) {
    switch (kind) {
        case AST_PROGRAM:   return "PROGRAM";
        case AST_FUNCTION:  return "FUNCTION";
        case AST_BLOCK:     return "BLOCK";
        case AST_IF:        return "IF";
        case AST_FOR:       return "FOR";
        case AST_WHILE:     return "WHILE";
        case AST_DO_WHILE:  return "DO_WHILE";
        case AST_SWITCH:    return "SWITCH";
        case AST_CASE:      return "CASE";
        case AST_DEFAULT:   return "DEFAULT";
        case AST_RETURN:    return "RETURN";
        case AST_BREAK:     return "BREAK";
        case AST_CONTINUE:  return "CONTINUE";
        case AST_STMT:      return "STMT";
        case AST_EXPR:      return "EXPR";
        case AST_TOKEN:     return "TOKEN";
        default:            return "UNKNOWN";
    }
}

/**
 * @brief 以缩进形式打印 AST（前序遍历），用于调试验证解析结果。
 *
 * @param node   当前节点；可为 NULL。
 * @param indent 当前缩进层级（每层两个空格）。
 */
void ast_dump(const ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i ++) printf("  ");
    if (node->kind == AST_TOKEN && node->text) {
        printf("%s: %s\n", ast_kind_name(node->kind), node->text);
    } else {
        printf("%s\n", ast_kind_name(node->kind));
    }
    for (size_t i = 0; i < node->child_count; i ++) {
        ast_dump(node->children[i], indent + 1);
    }
}