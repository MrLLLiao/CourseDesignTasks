#pragma once

#ifndef COURSEDESIGNTASKS_TOKENIZER_H
#define COURSEDESIGNTASKS_TOKENIZER_H

#include "std_token.h"
#include <stdbool.h>

typedef struct {
    const char *source;     // 源代码
    const char *current;    // 当前位置
    int line;               // 当前行号
    int col;                // 当前列号
    int ident_counter;      // 标识符计数器，用于归一化
} Tokenizer;

// 初始化tokenizer
void tokenizer_init(Tokenizer *tk, const char *source);

// 获取下一个token
Token* tokenizer_next_token(Tokenizer *tk);

// 检查是否到达文件末尾
bool tokenizer_is_eof(Tokenizer *tk);

#endif //COURSEDESIGNTASKS_TOKENIZER_H
