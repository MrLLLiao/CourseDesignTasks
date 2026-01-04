#include "../include/ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char* xstrdup(const char* s) {
    if (!s) return NULL;
    const size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

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

void ast_free(ASTNode* node) {
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);
    free(node->text);
    free(node);
}

const char* ast_kind_name(ASTKind kind) {
    switch (kind) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_FUNCTION: return "FUNCTION";
        case AST_BLOCK: return "BLOCK";
        case AST_IF: return "IF";
        case AST_FOR: return "FOR";
        case AST_WHILE: return "WHILE";
        case AST_DO_WHILE: return "DO_WHILE";
        case AST_SWITCH: return "SWITCH";
        case AST_CASE: return "CASE";
        case AST_DEFAULT: return "DEFAULT";
        case AST_RETURN: return "RETURN";
        case AST_BREAK: return "BREAK";
        case AST_CONTINUE: return "CONTINUE";
        case AST_STMT: return "STMT";
        case AST_EXPR: return "EXPR";
        case AST_TOKEN: return "TOKEN";
        default: return "UNKNOWN";
    }
}

void ast_dump(const ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) printf("  ");
    if (node->kind == AST_TOKEN && node->text) {
        printf("%s: %s\n", ast_kind_name(node->kind), node->text);
    } else {
        printf("%s\n", ast_kind_name(node->kind));
    }
    for (size_t i = 0; i < node->child_count; ++i) {
        ast_dump(node->children[i], indent + 1);
    }
}