/**
 * 代码相似度检测系统 - 主程序
 *
 * 功能流程：
 * 1. 读取两个C源代码文件
 * 2. 词法分析：将代码转换为Token序列
 * 3. 语法分析：将Token序列构建为AST（抽象语法树）
 * 4. AST序列化：将树结构转换为线性字符串序列
 * 5. 相似度计算：使用编辑距离算法比较两个序列
 * 6. 输出结果：显示相似度百分比
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== 引入项目模块 ==========
#include "ast.h"              // AST节点定义
#include "ast_parser.h"       // Token → AST
#include "ast_serial.h"       // AST → 字符串序列
#include "edit_distance.c.h"  // 编辑距离计算

// 引入 Tokenizer 模块（在上层目录）
#include "tokenizer.h"        // 词法分析器
#include "std_token.h"        // Token 定义

// ========== 工具函数 ==========

/**
 * 读取文件内容到字符串
 * @param filename 文件路径
 * @return 文件内容字符串（需要调用者释放），失败返回NULL
 */
char* read_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[错误] 无法打开文件: %s\n", filename);
        return NULL;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 分配内存并读取
    char* content = (char*)malloc(size + 1);
    if (!content) {
        fprintf(stderr, "[错误] 内存分配失败\n");
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(content, 1, size, fp);
    content[read_size] = '\0';
    fclose(fp);

    return content;
}

/**
 * 将源代码转换为Token数组
 * @param source 源代码字符串
 * @param token_count 输出参数：Token数量
 * @return Token指针数组（需要调用者释放）
 */
Token** tokenize_code(const char* source, size_t* token_count) {
    printf("  [步骤1] 词法分析中...\n");

    Tokenizer tk;
    tokenizer_init(&tk, source);

    // 动态数组存储Token
    size_t capacity = 1024;
    size_t count = 0;
    Token** tokens = (Token**)malloc(capacity * sizeof(Token*));

    if (!tokens) {
        fprintf(stderr, "[错误] Token数组内存分配失败\n");
        return NULL;
    }

    // 逐个读取Token
    while (!tokenizer_is_eof(&tk)) {
        Token* tok = tokenizer_next_token(&tk);

        if (!tok || tok->type == TOKEN_EOF) {
            if (tok) token_free(tok);
            break;
        }

        // 扩容
        if (count >= capacity) {
            capacity *= 2;
            Token** new_tokens = (Token**)realloc(tokens, capacity * sizeof(Token*));
            if (!new_tokens) {
                fprintf(stderr, "[错误] Token数组扩容失败\n");
                for (size_t i = 0; i < count; i++) {
                    token_free(tokens[i]);
                }
                free(tokens);
                return NULL;
            }
            tokens = new_tokens;
        }

        tokens[count++] = tok;
    }

    *token_count = count;
    printf("  ? 共识别 %zu 个Token\n", count);
    return tokens;
}

/**
 * 处理单个代码文件：源代码 → Token → AST → 序列化
 * @param filename 文件名（用于显示）
 * @param source 源代码字符串
 * @param out_vec 输出的序列化结果
 * @return 成功返回true，失败返回false
 */
int process_code(const char* filename, const char* source, StrVec* out_vec) {
    printf("\n=== 处理文件: %s ===\n", filename);

    // 步骤1: 词法分析
    size_t token_count = 0;
    Token** tokens = tokenize_code(source, &token_count);
    if (!tokens || token_count == 0) {
        fprintf(stderr, "[错误] 词法分析失败\n");
        return 0;
    }

    // 步骤2: 语法分析（构建AST）
    printf("  [步骤2] 语法分析中...\n");
    ASTNode* ast = ast_parse_tokens((Token* const*)tokens, token_count);

    // 释放Token数组（AST不需要它们了）
    for (size_t i = 0; i < token_count; i++) {
        token_free(tokens[i]);
    }
    free(tokens);

    if (!ast) {
        fprintf(stderr, "[错误] 语法分析失败\n");
        return 0;
    }
    printf("  ? AST构建成功\n");

    // 步骤3: AST序列化
    printf("  [步骤3] AST序列化中...\n");
    sv_init(out_vec);
    if (!ast_serialize_preorder(ast, out_vec)) {
        fprintf(stderr, "[错误] AST序列化失败\n");
        ast_free(ast);
        return 0;
    }
    printf("  ? 序列化完成，生成 %zu 个标记\n", out_vec->size);

    // 可选：打印AST结构（调试用）
    // printf("\n  [调试] AST结构:\n");
    // ast_dump(ast, 2);

    ast_free(ast);
    return 1;
}

/**
 * 比较两个代码文件的相似度
 * @param file1 第一个文件路径
 * @param file2 第二个文件路径
 */
void compare_files(const char* file1, const char* file2) {
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║      代码相似度检测系统 v1.0                 ║\n");
    printf("╚════════════════════════════════════════════════╝\n");

    // 读取文件
    printf("\n[阶段1] 读取源文件...\n");
    char* source1 = read_file(file1);
    char* source2 = read_file(file2);

    if (!source1 || !source2) {
        free(source1);
        free(source2);
        return;
    }

    printf("? 文件读取成功\n");
    printf("  - %s: %zu 字节\n", file1, strlen(source1));
    printf("  - %s: %zu 字节\n", file2, strlen(source2));

    // 处理两个文件
    printf("\n[阶段2] 代码结构分析...\n");

    StrVec seq1, seq2;
    int success1 = process_code(file1, source1, &seq1);
    int success2 = process_code(file2, source2, &seq2);

    free(source1);
    free(source2);

    if (!success1 || !success2) {
        if (success1) sv_free(&seq1);
        if (success2) sv_free(&seq2);
        return;
    }

    // 计算相似度
    printf("\n[阶段3] 计算相似度...\n");
    printf("  [步骤4] 计算编辑距离...\n");

    size_t distance = levenshtein_strvec(&seq1, &seq2);
    double similarity = similarity_from_dist(distance, seq1.size, seq2.size);

    printf("  ? 编辑距离: %zu\n", distance);
    printf("  ? 序列长度: A=%zu, B=%zu\n", seq1.size, seq2.size);

    // 输出结果
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║               相似度分析结果                   ║\n");
    printf("╠════════════════════════════════════════════════╣\n");
    printf("║  文件1: %-38s ║\n", file1);
    printf("║  文件2: %-38s ║\n", file2);
    printf("╠════════════════════════════════════════════════╣\n");
    printf("║  结构相似度: %.2f%%                           ║\n", similarity * 100);
    printf("╠════════════════════════════════════════════════╣\n");

    if (similarity >= 0.9) {
        printf("║  判定: 【高度相似】可能存在抄袭              ║\n");
    } else if (similarity >= 0.6) {
        printf("║  判定: 【中度相似】需人工审查                ║\n");
    } else if (similarity >= 0.3) {
        printf("║  判定: 【低度相似】部分结构相同              ║\n");
    } else {
        printf("║  判定: 【不相似】代码结构差异大              ║\n");
    }

    printf("╚════════════════════════════════════════════════╝\n");

    // 清理
    sv_free(&seq1);
    sv_free(&seq2);
}

// ========== 主程序入口 ==========

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc != 3) {
        printf("用法: %s <文件1.c> <文件2.c>\n", argv[0]);
        printf("\n示例:\n");
        printf("  %s codes/code1.c codes/code2.c\n", argv[0]);
        return 1;
    }

    // 执行相似度检测
    compare_files(argv[1], argv[2]);

    return 0;
}