/**
* @file ast_serial.c
 * @brief 将 AST 序列化为可比较的字符串序列（前序遍历）。
 *
 * 通过 <KIND> 与 </KIND> 标签保留树结构，并在叶子处写入 token 标签，
 * 便于后续使用编辑距离等序列相似算法进行比较。
 */

#include "../include/ast_serial.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 内部 strdup：为序列化输出向量复制字符串。
 *
 * @param s 以 '\0' 结尾的 C 字符串；可为 NULL。
 * @return malloc 得到的拷贝；失败返回 NULL。
 */
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

/**
 * @brief 初始化字符串向量（StrVec）。
 *
 * @param v 待初始化向量（非 NULL）。
 */
void sv_init(StrVec* v) {
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

/**
 * @brief 向 StrVec 追加一条字符串（内部会复制一份）。
 *
 * @param v 目标向量。
 * @param s 需要追加的字符串（非 NULL）。
 * @return 成功返回 true；失败返回 false（并保证不泄露本次分配）。
 *
 * @note StrVec 持有字符串所有权，需通过 sv_free 统一释放。
 */
bool sv_push(StrVec* v, const char* s) {
    if (!v || !s) return false;

    char* dup = xstrdup(s);
    if (!dup) return false;

    if (v->size == v->cap) {
        size_t nc = (v->cap == 0) ? 16 : v->cap * 2;
        char** p = (char**)realloc(v->data, nc * sizeof(char*));
        if (!p) {
            free(dup);
            return false;
        }
        v->data = p;
        v->cap = nc;
    }

    v->data[v->size++] = dup;
    return true;
}

/**
 * @brief 释放 StrVec 及其内部保存的所有字符串。
 *
 * @param v 目标向量；可为 NULL。
 */
void sv_free(StrVec* v) {
    if (!v) return;
    for (size_t i = 0; i < v->size; ++i) free(v->data[i]);
    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

/**
 * @brief 将 AST 以“前序遍历 + 进/出栈标签”的方式序列化为字符串序列。
 *
 * 输出格式示例（伪 XML）：
 *   <IF> <EXPR> ... </EXPR> <BLOCK> ... </BLOCK> </IF>
 *
 * @param n   当前节点；可为 NULL（视为成功，不输出）。
 * @param out 输出向量（StrVec），保存序列化后的 token 序列。
 * @return 成功返回 true；失败返回 false。
 */
static bool emit_node(const ASTNode* n, StrVec* out) {
    if (!n) return true;

    char buf[64];
    snprintf(buf, sizeof(buf), "<%s>", ast_kind_name(n->kind));
    if (!sv_push(out, buf)) return false;

    if (n->kind == AST_TOKEN && n->text) {
        if (!sv_push(out, n->text)) return false;
    }

    for (size_t i = 0; i < n->child_count; i ++) {
        if (!emit_node(n->children[i], out)) return false;
    }

    snprintf(buf, sizeof(buf), "</%s>", ast_kind_name(n->kind));
    if (!sv_push(out, buf)) return false;

    return true;
}

/**
 * @brief 对整棵 AST 做前序序列化（包装函数）。
 *
 * @param root AST 根节点。
 * @param out  输出向量（需已初始化）。
 * @return 成功返回 true；失败返回 false。
 */
bool ast_serialize_preorder(const ASTNode* root, StrVec* out) {
    if (!root || !out) return false;
    return emit_node(root, out);
}