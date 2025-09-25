#include "frontend/lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static char lexer_peek_char(const Lexer *lexer, size_t offset) {
    size_t pos = lexer->index + offset;
    if (pos >= lexer->length) {
        return '\0';
    }
    return lexer->source[pos];
}

static char lexer_current_char(const Lexer *lexer) {
    return lexer_peek_char(lexer, 0);
}

static void lexer_advance(Lexer *lexer) {
    if (lexer->index >= lexer->length) {
        return;
    }

    char c = lexer->source[lexer->index];
    lexer->index += 1;

    if (c == '\n') {
        lexer->line += 1;
        lexer->column = 1;
    } else {
        lexer->column += 1;
    }
}

static bool is_identifier_start(char c) {
    return (c == '_') || isalpha((unsigned char)c);
}

static bool is_identifier_part(char c) {
    return (c == '_') || isalnum((unsigned char)c);
}

static TokenKind keyword_lookup(const char *start, size_t length) {
    if (length == 0) {
        return TOKEN_IDENTIFIER;
    }

    switch (start[0]) {
    case 'e':
        if (length == 4 && strncmp(start, "else", length) == 0) {
            return TOKEN_KW_ELSE;
        }
        break;
    case 'i':
        if (length == 2 && strncmp(start, "if", length) == 0) {
            return TOKEN_KW_IF;
        }
        if (length == 3 && strncmp(start, "int", length) == 0) {
            return TOKEN_KW_INT;
        }
        break;
    case 'r':
        if (length == 6 && strncmp(start, "return", length) == 0) {
            return TOKEN_KW_RETURN;
        }
        break;
    case 'w':
        if (length == 5 && strncmp(start, "while", length) == 0) {
            return TOKEN_KW_WHILE;
        }
        break;
    default:
        break;
    }

    return TOKEN_IDENTIFIER;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    for (;;) {
        char c = lexer_current_char(lexer);
        if (c == '\0') {
            return;
        }

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lexer_advance(lexer);
            continue;
        }

        if (c == '/' && lexer_peek_char(lexer, 1) == '/') {
            while (lexer_current_char(lexer) != '\0' && lexer_current_char(lexer) != '\n') {
                lexer_advance(lexer);
            }
            continue;
        }

        if (c == '/' && lexer_peek_char(lexer, 1) == '*') {
            lexer_advance(lexer); // '/'
            lexer_advance(lexer); // '*'
            while (lexer_current_char(lexer) != '\0') {
                if (lexer_current_char(lexer) == '*' && lexer_peek_char(lexer, 1) == '/') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    break;
                }
                lexer_advance(lexer);
            }
            continue;
        }

        break;
    }
}

static Token make_token(const Lexer *lexer, TokenKind kind, size_t start_index, size_t start_line, size_t start_column) {
    size_t end_index = lexer->index;
    Token token;
    token.kind = kind;
    token.lexeme = lexer->source + start_index;
    token.length = end_index - start_index;
    token.line = start_line;
    token.column = start_column;
    return token;
}

static Token scan_identifier_or_keyword(Lexer *lexer, size_t start_index, size_t start_line, size_t start_column) {
    while (is_identifier_part(lexer_current_char(lexer))) {
        lexer_advance(lexer);
    }

    size_t length = lexer->index - start_index;
    TokenKind kind = keyword_lookup(lexer->source + start_index, length);
    return make_token(lexer, kind, start_index, start_line, start_column);
}

static Token scan_number(Lexer *lexer, size_t start_index, size_t start_line, size_t start_column) {
    while (isdigit((unsigned char)lexer_current_char(lexer))) {
        lexer_advance(lexer);
    }

    if (lexer_current_char(lexer) == '.' && isdigit((unsigned char)lexer_peek_char(lexer, 1))) {
        lexer_advance(lexer);
        while (isdigit((unsigned char)lexer_current_char(lexer))) {
            lexer_advance(lexer);
        }
    }

    return make_token(lexer, TOKEN_NUMBER, start_index, start_line, start_column);
}

static Token scan_string(Lexer *lexer, size_t start_index, size_t start_line, size_t start_column) {
    while (lexer_current_char(lexer) != '\0') {
        if (lexer_current_char(lexer) == '"') {
            lexer_advance(lexer);
            break;
        }

        if (lexer_current_char(lexer) == '\\' && lexer_peek_char(lexer, 1) != '\0') {
            lexer_advance(lexer);
        }
        lexer_advance(lexer);
    }

    return make_token(lexer, TOKEN_STRING, start_index, start_line, start_column);
}

void lexer_init(Lexer *lexer, const char *source, size_t length) {
    if (!lexer) {
        return;
    }

    lexer->source = source;
    lexer->length = length;
    lexer->index = 0;
    lexer->line = 1;
    lexer->column = 1;
}

Token lexer_peek_token(const Lexer *lexer) {
    Lexer lookahead = *lexer;
    return lexer_next_token(&lookahead);
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    size_t start_index = lexer->index;
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    char c = lexer_current_char(lexer);

    if (c == '\0') {
        return make_token(lexer, TOKEN_EOF, start_index, start_line, start_column);
    }

    if (is_identifier_start(c)) {
        lexer_advance(lexer);
        return scan_identifier_or_keyword(lexer, start_index, start_line, start_column);
    }

    if (isdigit((unsigned char)c)) {
        lexer_advance(lexer);
        return scan_number(lexer, start_index, start_line, start_column);
    }

    if (c == '"') {
        lexer_advance(lexer);
        return scan_string(lexer, start_index, start_line, start_column);
    }

    lexer_advance(lexer);

    switch (c) {
    case '(':
        return make_token(lexer, TOKEN_L_PAREN, start_index, start_line, start_column);
    case ')':
        return make_token(lexer, TOKEN_R_PAREN, start_index, start_line, start_column);
    case '{':
        return make_token(lexer, TOKEN_L_BRACE, start_index, start_line, start_column);
    case '}':
        return make_token(lexer, TOKEN_R_BRACE, start_index, start_line, start_column);
    case ';':
        return make_token(lexer, TOKEN_SEMICOLON, start_index, start_line, start_column);
    case ',':
        return make_token(lexer, TOKEN_COMMA, start_index, start_line, start_column);
    case '*':
        return make_token(lexer, TOKEN_ASTERISK, start_index, start_line, start_column);
    case '+':
        return make_token(lexer, TOKEN_PLUS, start_index, start_line, start_column);
    case '-':
        return make_token(lexer, TOKEN_MINUS, start_index, start_line, start_column);
    case '/':
        return make_token(lexer, TOKEN_SLASH, start_index, start_line, start_column);
    case '=':
        if (lexer_current_char(lexer) == '=') {
            lexer_advance(lexer);
            return make_token(lexer, TOKEN_EQUAL_EQUAL, start_index, start_line, start_column);
        }
        return make_token(lexer, TOKEN_EQUAL, start_index, start_line, start_column);
    default:
        break;
    }

    return make_token(lexer, TOKEN_UNKNOWN, start_index, start_line, start_column);
}
