#ifndef FUNGCC_FRONTEND_TOKEN_H
#define FUNGCC_FRONTEND_TOKEN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TokenKind {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,

    TOKEN_KW_INT,
    TOKEN_KW_RETURN,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_WHILE,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_ASTERISK,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,

    TOKEN_UNKNOWN
} TokenKind;

typedef struct Token {
    TokenKind kind;
    const char *lexeme;
    size_t length;
    size_t line;
    size_t column;
} Token;

#ifdef __cplusplus
}
#endif

#endif /* FUNGCC_FRONTEND_TOKEN_H */
