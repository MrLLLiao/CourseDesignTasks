/**
 * 代码相似度检测系统 - 主程序 (UI美化版)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== 引入项目模块 ==========
#include "ast.h"
#include "ast_parser.h"
#include "ast_serial.h"
#include "edit_distance.c.h"
#include "tokenizer.h"
#include "std_token.h"

// ========== UI 美化宏定义 ==========
// ANSI 颜色代码
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"
#define WHITE   "\033[37m"

// 图标 (需终端支持UTF-8)
#define ICON_CHECK   "✔"
#define ICON_CROSS   "✖"
#define ICON_ARROW   "➤"
#define ICON_STAR    "★"
#define ICON_FILE    "📄"
#define ICON_CODE    "💻"

// ========== 工具函数 ==========

/**
 * 打印带颜色的进度条
 * @param label 当前操作描述
 * @param state 输出状态
 */
void print_step(const char* label, int state) {
    // 清除当前行 (防止残留字符)
    printf("\r                                                           \r");

    if (state == 0) {
        // 进行中：显示箭头和文本
        printf("  " BLUE ICON_ARROW " %s..." RESET, label);
        fflush(stdout); // 强制刷新缓冲区，确保文字立即显示
    } else if (state == 1) {
        // 完成：显示对号
        printf("  " GREEN ICON_CHECK " %-16s" RESET " " GREEN "OK" RESET "\n", label);
    } else {
        // 失败：显示叉号
        printf("  " RED ICON_CROSS " %-16s" RESET " " RED "FAILED" RESET "\n", label);
    }
}

/**
 * 打印分割线
 */
void print_separator() {
    printf(BLUE "  ────────────────────────────────────────────────────────────\n" RESET);
}

/**
 * 读取文件内容到字符串
 */
char* read_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("  " RED ICON_CROSS " [错误] 无法打开文件: %s" RESET "\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (!content) {
        printf("  " RED ICON_CROSS " [错误] 内存分配失败" RESET "\n");
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
 */
/**
 * 将源代码转换为Token数组 (修复了内存泄露隐患)
 */
Token** tokenize_code(const char* source, size_t* token_count) {
    Tokenizer tk;
    tokenizer_init(&tk, source);

    size_t capacity = 1024;
    size_t count = 0;
    Token** tokens = (Token**)malloc(capacity * sizeof(Token*));

    if (!tokens) return NULL;

    while (!tokenizer_is_eof(&tk)) {
        Token* tok = tokenizer_next_token(&tk);

        // 处理 tokenizer 返回 NULL 或 EOF 的情况
        if (!tok || tok->type == TOKEN_EOF) {
            if (tok) token_free(tok);
            break;
        }

        // 扩容检查
        if (count >= capacity) {
            size_t new_capacity = capacity * 2;
            Token** new_tokens = (Token**)realloc(tokens, new_capacity * sizeof(Token*));

            if (!new_tokens) {
                // [修复] 扩容失败时，必须释放所有已分配的 Token 和数组本身
                fprintf(stderr, RED "  [错误] 内存不足，Token数组扩容失败\n" RESET);
                for (size_t i = 0; i < count; i++) {
                    token_free(tokens[i]);
                }
                free(tokens);
                return NULL;
            }
            tokens = new_tokens;
            capacity = new_capacity;
        }

        tokens[count++] = tok;
    }

    *token_count = count;
    return tokens;
}

/**
 * 处理单个代码文件
 */
int process_code(const char* filename, const char* source, StrVec* out_vec) {
    printf("\n" BOLD WHITE "┌── 处理文件: %s" RESET "\n", filename);

    // --- 步骤 1: 词法分析 ---
    print_step("词法分析", 0);

    size_t token_count = 0;
    Token** tokens = tokenize_code(source, &token_count);

    // 情况 1: 内存分配完全失败 (tokens 为 NULL)
    if (!tokens) {
        print_step("词法分析", -1);
        return 0;
    }

    // 情况 2: 文件是空的 (tokens 不为 NULL，但数量为 0)
    // [修复点] 这个检查必须在 if (!tokens) 外面
    if (token_count == 0) {
        printf("  " YELLOW ICON_ARROW " [警告] 文件为空或无有效代码\n" RESET);
        print_step("词法分析", -1); // 标记为失败（因为无法进行后续步骤）
        free(tokens); // 释放刚才分配的空数组
        return 0;
    }

    // 成功
    print_step("词法分析", 1);

    // --- 步骤 2: 语法分析 ---
    print_step("构建语法树(AST)", 0);

    ASTNode* ast = ast_parse_tokens((Token* const*)tokens, token_count);

    if (!ast) {
        print_step("构建语法树(AST)", -1);
        // 清理资源
        for (size_t i = 0; i < token_count; i++) token_free(tokens[i]);
        free(tokens);
        return 0;
    }
    print_step("构建语法树(AST)", 1);

    // --- 步骤 3: 序列化 ---
    print_step("结构序列化", 0);

    sv_init(out_vec);
    int serial_success = ast_serialize_preorder(ast, out_vec);

    if (!serial_success) {
        print_step("结构序列化", -1);
        ast_free(ast);
        for (size_t i = 0; i < token_count; i++) token_free(tokens[i]);
        free(tokens);
        sv_free(out_vec);
        return 0;
    }
    print_step("结构序列化", 1);

    // --- 资源清理 ---
    // 先释放AST
    ast_free(ast);
    // 再释放Token (安全)
    for (size_t i = 0; i < token_count; i++) {
        token_free(tokens[i]);
    }
    free(tokens);

    // 总结输出
    printf("  " MAGENTA ICON_STAR " 特征提取完成:" RESET " 生成 %zu 个特征节点\n", out_vec->size);
    return 1;
}

/**
 * 绘制相似度可视化条
 */
void print_sim_bar(double similarity) {
    int bars = (int)(similarity * 30); // 30格长
    printf("║  可视化: [");
    for(int i=0; i<30; i++) {
        if(i < bars) {
            if(similarity > 0.8) printf(RED "█" RESET);
            else if(similarity > 0.5) printf(YELLOW "█" RESET);
            else printf(GREEN "█" RESET);
        } else {
            printf(WHITE "░" RESET);
        }
    }
    printf("]      ║\n");
}

// ========== 新增：UI 辅助函数 ==========

/**
 * 计算字符串在终端显示的视觉宽度
 * (ASCII算1，中文字符算2)
 */
int get_visual_width(const char* str) {
    int width = 0;
    int i = 0;
    while (str[i]) {
        unsigned char c = (unsigned char)str[i];
        if (c < 128) {
            width += 1; // ASCII
            i++;
        } else {
            // UTF-8 多字节字符 (通常中文字符占3字节，显示宽2格)
            // 简单的跳过后续字节逻辑
            width += 2;
            // 跳过 UTF-8 后续字节
            i++;
            while ((str[i] & 0xC0) == 0x80) i++;
        }
    }
    return width;
}

/**
 * 打印重复的字符
 */
void print_repeat(const char* ch, int count) {
    for (int i = 0; i < count; i++) printf("%s", ch);
}

/**
 * 打印居中文字行
 */
void print_centered_row(const char* text, int box_width, const char* border) {
    int text_w = get_visual_width(text);
    int padding = box_width - text_w;
    int pad_left = padding / 2;
    int pad_right = padding - pad_left;

    printf("%s", border);
    print_repeat(" ", pad_left);
    printf("%s", text);
    print_repeat(" ", pad_right);
    printf("%s\n", border);
}

/**
 * 比较两个代码文件的相似度
 */
void compare_files(const char* file1, const char* file2) {
    // 1. Banner
    system("cls"); // 清屏
    printf(CYAN BOLD "\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║             " ICON_CODE " 代码结构相似度检测系统 v3.0         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" RESET);

    char* source1 = read_file(file1);
    char* source2 = read_file(file2);

    if (!source1 || !source2) {
        if(source1) free(source1);
        if(source2) free(source2);
        return;
    }

    // 2. 处理代码 (process_code 内部已含步骤打印)
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

    // 3. 计算相似度
    size_t distance = levenshtein_strvec(&seq1, &seq2);
    double similarity = similarity_from_dist(distance, seq1.size, seq2.size);

    // ================= UI 动态绘制逻辑 =================

    // 1. 计算所需的最大宽度
    // 基础宽度 60，如果有更长的文件路径，则扩展宽度
    int min_width = 60;
    int content_width_1 = get_visual_width(file1) + 12; // "文件 A: " + 路径
    int content_width_2 = get_visual_width(file2) + 12; // "文件 B: " + 路径

    int box_width = min_width;
    if (content_width_1 > box_width) box_width = content_width_1 + 4; // +4 留一点余量
    if (content_width_2 > box_width) box_width = content_width_2 + 4;

    // 准备颜色和判定词
    char color_code[10];
    char result_text[64];

    if (similarity >= 0.9) {
        strcpy(color_code, RED BOLD);
        sprintf(result_text, "【高度相似】 极大可能存在抄袭");
    } else if (similarity >= 0.6) {
        strcpy(color_code, YELLOW BOLD);
        sprintf(result_text, "【中度相似】 建议人工审查逻辑");
    } else if (similarity >= 0.3) {
        strcpy(color_code, CYAN BOLD);
        sprintf(result_text, "【低度相似】 仅部分语法结构雷同");
    } else {
        strcpy(color_code, GREEN BOLD);
        sprintf(result_text, "【不相似】   代码结构差异显著");
    }

    printf("\n");

    // --- 顶部边框 ---
    printf(WHITE "╔");
    print_repeat("═", box_width);
    printf("╗\n");

    // --- 标题 ---
    print_centered_row("相似度分析报告", box_width, "║");

    // --- 分隔线 ---
    printf("╠");
    print_repeat("═", box_width);
    printf("╣\n" RESET);

    // --- 文件路径 (左对齐，动态计算右侧空格) ---
    // 宏定义用于简化打印逻辑
    #define PRINT_FILE_ROW(label, f_name) do { \
        printf(WHITE "║" RESET "  %s %s", label, f_name); \
        int _w = get_visual_width(label) + get_visual_width(f_name) + 2; \
        print_repeat(" ", box_width - _w); \
        printf(WHITE "║\n" RESET); \
    } while(0)

    PRINT_FILE_ROW("文件 A:", file1);
    PRINT_FILE_ROW("文件 B:", file2);

    // --- 分隔线 ---
    printf(WHITE "╠");
    print_repeat("═", box_width);
    printf("╣\n" RESET);

    // --- 相似度数值 ---
    printf(WHITE "║" RESET "  结构相似度: %s%6.2f%%%s", color_code, similarity * 100, RESET);
    int sim_text_w = 14 + 7; // "结构相似度: " + "xx.xx%"
    print_repeat(" ", box_width - sim_text_w - 2);
    printf(WHITE "║\n" RESET);

    // --- 可视化条 (自适应宽度) ---
    int bar_max_len = box_width - 16; // 减去 "  可视化: []  " 的长度
    int bar_fill = (int)(similarity * bar_max_len);

    printf(WHITE "║" RESET "  可视化: [");
    // 打印实心部分
    printf("%s", color_code); // 应用颜色
    for(int i=0; i<bar_max_len; i++) {
        if (i < bar_fill) printf("█");
        else printf(RESET WHITE "░"); // 未填充部分
    }
    printf(RESET "]  " WHITE "║\n" RESET);

    // --- 分隔线 ---
    printf(WHITE "╠");
    print_repeat("═", box_width);
    printf("╣\n" RESET);

    // --- 判定结果 (左对齐) ---
    printf(WHITE "║" RESET "  判定: %s%s%s", color_code, result_text, RESET);
    int res_w = 8 + get_visual_width(result_text); // "  判定: " + 文字
    print_repeat(" ", box_width - res_w - 2); // -2 是因为前面有两个空格
    printf(WHITE "║\n" RESET);

    // --- 底部边框 ---
    printf(WHITE "╚");
    print_repeat("═", box_width);
    printf("╝\n" RESET);
    printf("\n");

    // 清理
    sv_free(&seq1);
    sv_free(&seq2);
}

// ========== 主程序入口 ==========

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc != 3) {
        printf(YELLOW "\n用法: %s <文件1.c> <文件2.c>\n" RESET, argv[0]);
        printf("示例:\n");
        printf("  %s codes/original.c codes/copied.c\n\n", argv[0]);
        return 1;
    }

    // 设置控制台编码为 UTF-8 (针对 Windows)
    #ifdef _WIN32
    system("chcp 65001 > nul");
    #endif

    compare_files(argv[1], argv[2]);

    return 0;
}