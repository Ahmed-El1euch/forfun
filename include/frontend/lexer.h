#ifndef FUNGCC_FRONTEND_LEXER_H
#define FUNGCC_FRONTEND_LEXER_H

#include <stddef.h>

#include "frontend/token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Lexer {
    const char *source;
    size_t length;
    size_t index;
    size_t line;
    size_t column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source, size_t length);
Token lexer_peek_token(const Lexer *lexer);
Token lexer_next_token(Lexer *lexer);

#ifdef __cplusplus
}
#endif

#endif /* FUNGCC_FRONTEND_LEXER_H */
