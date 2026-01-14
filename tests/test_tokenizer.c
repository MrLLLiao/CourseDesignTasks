#include <stdio.h>
#include "tokenizer.h"

// 打印token类型名称
const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TK_IDENT: return "IDENT";
        case TK_KEYWORD: return "KEYWORD";
        case TK_NUMBER: return "NUMBER";
        case TK_STRING: return "STRING";
        case TK_CHAR: return "CHAR";
        case TK_OPERATOR: return "OPERATOR";
        case TK_PUNCTUATION: return "PUNCTUATION";
        default: return "UNKNOWN";
    }
}

int main() {
    // 测试用例1: int a = 1;
    const char *source1 = "int a = 1;";
    printf("Test 1: %s\n", source1);
    printf("Tokens:\n");

    Tokenizer tk;
    tokenizer_init(&tk, source1);

    while (!tokenizer_is_eof(&tk)) {
        Token *token = tokenizer_next_token(&tk);
        if (token->type == TOKEN_EOF) {
            token_free(token);
            break;
        }

        printf("  [%s] raw='%s', lex='%s', line=%d, col=%d\n",
               token_type_name(token->type),
               token->raw,
               token->lex,
               token->line,
               token->col);

        token_free(token);
    }

    printf("\n");

    // 测试用例2: 更复杂的代码
    const char *source2 =
        "int main() {\n"
        "    int x = 10;\n"
        "    if (x > 5) {\n"
        "        printf(\"Hello\");\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    printf("Test 2: Complex code\n");
    printf("Tokens:\n");

    tokenizer_init(&tk, source2);

    while (!tokenizer_is_eof(&tk)) {
        Token *token = tokenizer_next_token(&tk);
        if (token->type == TOKEN_EOF) {
            token_free(token);
            break;
        }

        printf("  [%s] raw='%s', lex='%s'\n",
               token_type_name(token->type),
               token->raw,
               token->lex);

        token_free(token);
    }

    printf("\n");

    // 测试用例3: 带注释的代码
    const char *source3 =
        "// This is a comment\n"
        "int a = 1; /* inline comment */ int b = 2;\n";

    printf("Test 3: Code with comments\n");
    printf("Tokens:\n");

    tokenizer_init(&tk, source3);

    while (!tokenizer_is_eof(&tk)) {
        Token *token = tokenizer_next_token(&tk);
        if (token->type == TOKEN_EOF) {
            token_free(token);
            break;
        }

        printf("  [%s] raw='%s', lex='%s'\n",
               token_type_name(token->type),
               token->raw,
               token->lex);

        token_free(token);
    }

    return 0;
}
