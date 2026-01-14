#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// 关键字映射表
typedef struct {
    const char *word;
    KeywordKind kind;
} KeywordMap;

static KeywordMap keyword_table[] = {
    {"if", KW_IF}, {"else", KW_ELSE}, {"for", KW_FOR}, {"while", KW_WHILE},
    {"do", KW_DO}, {"switch", KW_SWITCH}, {"case", KW_CASE}, {"default", KW_DEFAULT},
    {"return", KW_RETURN}, {"break", KW_BREAK}, {"continue", KW_CONTINUE},
    {"int", KW_INT}, {"char", KW_CHAR}, {"float", KW_FLOAT}, {"double", KW_DOUBLE},
    {"void", KW_VOID}, {"struct", KW_STRUCT}, {"typedef", KW_TYPEDEF},
    {NULL, KW_UNKNOWN}
};

// 辅助函数：创建token
static Token* create_token(TokenType type, const char *raw, const char *lex, int line, int col) {
    Token *tok = (Token*)malloc(sizeof(Token));
    tok->type = type;
    tok->kw = KW_UNKNOWN;
    tok->raw = strdup(raw);
    tok->lex = strdup(lex);
    tok->line = line;
    tok->col = col;
    return tok;
}

// 辅助函数：检查是否是关键字
static KeywordKind check_keyword(const char *word) {
    for (int i = 0; keyword_table[i].word != NULL; i++) {
        if (strcmp(word, keyword_table[i].word) == 0) {
            return keyword_table[i].kind;
        }
    }
    return KW_UNKNOWN;
}

// 跳过空白字符和注释
static void skip_whitespace_and_comments(Tokenizer *tk) {
    while (*tk->current) {
        // 跳过空白字符
        if (isspace(*tk->current)) {
            if (*tk->current == '\n') {
                tk->line++;
                tk->col = 1;
            } else {
                tk->col++;
            }
            tk->current++;
        }
        // 跳过单行注释
        else if (tk->current[0] == '/' && tk->current[1] == '/') {
            while (*tk->current && *tk->current != '\n') {
                tk->current++;
            }
        }
        // 跳过多行注释
        else if (tk->current[0] == '/' && tk->current[1] == '*') {
            tk->current += 2;
            tk->col += 2;
            while (*tk->current) {
                if (tk->current[0] == '*' && tk->current[1] == '/') {
                    tk->current += 2;
                    tk->col += 2;
                    break;
                }
                if (*tk->current == '\n') {
                    tk->line++;
                    tk->col = 1;
                } else {
                    tk->col++;
                }
                tk->current++;
            }
        }
        else {
            break;
        }
    }
}

// 读取标识符或关键字
static Token* read_identifier(Tokenizer *tk) {
    int start_line = tk->line;
    int start_col = tk->col;
    const char *start = tk->current;

    while (isalnum(*tk->current) || *tk->current == '_') {
        tk->current++;
        tk->col++;
    }

    int len = tk->current - start;
    char *word = (char*)malloc(len + 1);
    strncpy(word, start, len);
    word[len] = '\0';

    // 检查是否是关键字
    KeywordKind kw = check_keyword(word);

    Token *tok;
    if (kw != KW_UNKNOWN) {
        // 关键字：归一化后也是关键字本身
        tok = create_token(TK_KEYWORD, word, word, start_line, start_col);
        tok->kw = kw;
    } else {
        // 标识符：归一化为 var_N
        char normalized[32];
        sprintf(normalized, "var_%d", tk->ident_counter++);
        tok = create_token(TK_IDENT, word, normalized, start_line, start_col);
    }

    free(word);
    return tok;
}

// 读取数字
static Token* read_number(Tokenizer *tk) {
    int start_line = tk->line;
    int start_col = tk->col;
    const char *start = tk->current;

    // 处理十六进制
    if (tk->current[0] == '0' && (tk->current[1] == 'x' || tk->current[1] == 'X')) {
        tk->current += 2;
        tk->col += 2;
        while (isxdigit(*tk->current)) {
            tk->current++;
            tk->col++;
        }
    }
    // 处理十进制和浮点数
    else {
        while (isdigit(*tk->current)) {
            tk->current++;
            tk->col++;
        }

        // 浮点数
        if (*tk->current == '.') {
            tk->current++;
            tk->col++;
            while (isdigit(*tk->current)) {
                tk->current++;
                tk->col++;
            }
        }

        // 科学计数法
        if (*tk->current == 'e' || *tk->current == 'E') {
            tk->current++;
            tk->col++;
            if (*tk->current == '+' || *tk->current == '-') {
                tk->current++;
                tk->col++;
            }
            while (isdigit(*tk->current)) {
                tk->current++;
                tk->col++;
            }
        }
    }

    // 后缀 (L, U, F等)
    while (*tk->current == 'L' || *tk->current == 'l' ||
           *tk->current == 'U' || *tk->current == 'u' ||
           *tk->current == 'F' || *tk->current == 'f') {
        tk->current++;
        tk->col++;
    }

    int len = tk->current - start;
    char *num = (char*)malloc(len + 1);
    strncpy(num, start, len);
    num[len] = '\0';

    // 数字归一化为 "NUM"
    Token *tok = create_token(TK_NUMBER, num, "NUM", start_line, start_col);
    free(num);
    return tok;
}

// 读取字符串
static Token* read_string(Tokenizer *tk) {
    int start_line = tk->line;
    int start_col = tk->col;
    const char *start = tk->current;

    tk->current++; // 跳过开始的引号
    tk->col++;

    while (*tk->current && *tk->current != '"') {
        if (*tk->current == '\\') {
            tk->current++;
            tk->col++;
            if (*tk->current) {
                tk->current++;
                tk->col++;
            }
        } else {
            if (*tk->current == '\n') {
                tk->line++;
                tk->col = 1;
            } else {
                tk->col++;
            }
            tk->current++;
        }
    }

    if (*tk->current == '"') {
        tk->current++;
        tk->col++;
    }

    int len = tk->current - start;
    char *str = (char*)malloc(len + 1);
    strncpy(str, start, len);
    str[len] = '\0';

    // 字符串归一化为 "STR"
    Token *tok = create_token(TK_STRING, str, "STR", start_line, start_col);
    free(str);
    return tok;
}

// 读取字符常量
static Token* read_char(Tokenizer *tk) {
    int start_line = tk->line;
    int start_col = tk->col;
    const char *start = tk->current;

    tk->current++; // 跳过开始的单引号
    tk->col++;

    if (*tk->current == '\\') {
        tk->current++;
        tk->col++;
    }
    if (*tk->current) {
        tk->current++;
        tk->col++;
    }

    if (*tk->current == '\'') {
        tk->current++;
        tk->col++;
    }

    int len = tk->current - start;
    char *ch = (char*)malloc(len + 1);
    strncpy(ch, start, len);
    ch[len] = '\0';

    // 字符归一化为 "CHAR"
    Token *tok = create_token(TK_CHAR, ch, "CHAR", start_line, start_col);
    free(ch);
    return tok;
}

// 读取运算符
static Token* read_operator(Tokenizer *tk) {
    int start_line = tk->line;
    int start_col = tk->col;
    const char *start = tk->current;

    char c = *tk->current;
    tk->current++;
    tk->col++;

    // 检查双字符运算符
    if (c == '=' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '!' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '<' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '>' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '&' && *tk->current == '&') {
        tk->current++; tk->col++;
    } else if (c == '|' && *tk->current == '|') {
        tk->current++; tk->col++;
    } else if (c == '+' && *tk->current == '+') {
        tk->current++; tk->col++;
    } else if (c == '-' && *tk->current == '-') {
        tk->current++; tk->col++;
    } else if (c == '+' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '-' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '*' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '/' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '%' && *tk->current == '=') {
        tk->current++; tk->col++;
    } else if (c == '<' && *tk->current == '<') {
        tk->current++; tk->col++;
    } else if (c == '>' && *tk->current == '>') {
        tk->current++; tk->col++;
    } else if (c == '-' && *tk->current == '>') {
        tk->current++; tk->col++;
    }

    int len = tk->current - start;
    char *op = (char*)malloc(len + 1);
    strncpy(op, start, len);
    op[len] = '\0';

    // 运算符归一化为自身
    Token *tok = create_token(TK_OPERATOR, op, op, start_line, start_col);
    free(op);
    return tok;
}

// 读取标点符号
static Token* read_punctuation(Tokenizer *tk) {
    int start_line = tk->line;
    int start_col = tk->col;
    char punct[2] = {*tk->current, '\0'};

    tk->current++;
    tk->col++;

    // 标点符号归一化为自身
    return create_token(TK_PUNCTUATION, punct, punct, start_line, start_col);
}

// 初始化tokenizer
void tokenizer_init(Tokenizer *tk, const char *source) {
    tk->source = source;
    tk->current = source;
    tk->line = 1;
    tk->col = 1;
    tk->ident_counter = 0;
}

// 获取下一个token
Token* tokenizer_next_token(Tokenizer *tk) {
    skip_whitespace_and_comments(tk);

    if (*tk->current == '\0') {
        return create_token(TOKEN_EOF, "", "", tk->line, tk->col);
    }

    // 标识符或关键字
    if (isalpha(*tk->current) || *tk->current == '_') {
        return read_identifier(tk);
    }

    // 数字
    if (isdigit(*tk->current)) {
        return read_number(tk);
    }

    // 字符串
    if (*tk->current == '"') {
        return read_string(tk);
    }

    // 字符常量
    if (*tk->current == '\'') {
        return read_char(tk);
    }

    // 运算符
    if (strchr("+-*/%=!<>&|^~", *tk->current)) {
        return read_operator(tk);
    }

    // 标点符号
    if (strchr("(){}[];,.", *tk->current)) {
        return read_punctuation(tk);
    }

    // 未知字符，跳过
    tk->current++;
    tk->col++;
    return tokenizer_next_token(tk);
}

// 检查是否到达文件末尾
bool tokenizer_is_eof(Tokenizer *tk) {
    skip_whitespace_and_comments(tk);
    return *tk->current == '\0';
}
