#include "std_token.h"
#include <stdlib.h>

void token_free(Token *tok) {
    if (tok == NULL) {
        return;
    }

    if (tok->lex != NULL) {
        free(tok->lex);
    }

    if (tok->raw != NULL) {
        free(tok->raw);
    }

    free(tok);
}
