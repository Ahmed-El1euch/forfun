#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frontend/lexer.h"

#define ASSERT_TRUE(cond, msg)                                                                   \
    do {                                                                                          \
        if (!(cond)) {                                                                            \
            fprintf(stderr, "Assertion failed: %s (line %d): %s\n", __FILE__, __LINE__, msg);   \
            return EXIT_FAILURE;                                                                  \
        }                                                                                         \
    } while (0)

#define ASSERT_EQ_INT(actual, expected, msg)                                                      \
    do {                                                                                          \
        if ((actual) != (expected)) {                                                             \
            fprintf(stderr, "Assertion failed: %s (line %d): expected %d, got %d\n",             \
                    __FILE__, __LINE__, (expected), (actual));                                    \
            fprintf(stderr, "%s\n", msg);                                                       \
            return EXIT_FAILURE;                                                                  \
        }                                                                                         \
    } while (0)

static int test_skips_whitespace_and_comments(void) {
    const char *source =
        "\n\t  int // line comment\n"
        "main/*block\ncomment*/( ) ;";

    Lexer lexer;
    lexer_init(&lexer, source, strlen(source));

    TokenKind expected[] = {
        TOKEN_KW_INT,
        TOKEN_IDENTIFIER,
        TOKEN_L_PAREN,
        TOKEN_R_PAREN,
        TOKEN_SEMICOLON,
        TOKEN_EOF,
    };

    size_t expected_count = sizeof(expected) / sizeof(expected[0]);

    for (size_t i = 0; i < expected_count; ++i) {
        Token token = lexer_next_token(&lexer);
        char message[128];
        snprintf(message, sizeof(message), "Mismatch at index %zu", i);
        ASSERT_EQ_INT(token.kind, expected[i], message);
    }

    return EXIT_SUCCESS;
}

static int test_number_tokens(void) {
    const char *source = "42 3.1415";

    Lexer lexer;
    lexer_init(&lexer, source, strlen(source));

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ_INT(token.kind, TOKEN_NUMBER, "Integral literal not tokenized as NUMBER");
    ASSERT_TRUE(token.length == 2 && strncmp(token.lexeme, "42", token.length) == 0,
                "Incorrect integer lexeme");

    token = lexer_next_token(&lexer);
    ASSERT_EQ_INT(token.kind, TOKEN_NUMBER, "Floating literal not tokenized as NUMBER");
    ASSERT_TRUE(token.length == 6 && strncmp(token.lexeme, "3.1415", token.length) == 0,
                "Incorrect floating lexeme");

    token = lexer_next_token(&lexer);
    ASSERT_EQ_INT(token.kind, TOKEN_EOF, "Expected EOF after number literals");

    return EXIT_SUCCESS;
}

static int test_string_literal(void) {
    const char *source = "\"fungcc\\n\"";

    Lexer lexer;
    lexer_init(&lexer, source, strlen(source));

    Token token = lexer_next_token(&lexer);
    ASSERT_EQ_INT(token.kind, TOKEN_STRING, "String literal not tokenized as STRING");
    ASSERT_TRUE(token.length == strlen(source), "String token length mismatch");

    token = lexer_next_token(&lexer);
    ASSERT_EQ_INT(token.kind, TOKEN_EOF, "Expected EOF after string literal");

    return EXIT_SUCCESS;
}

int main(void) {
    typedef int (*test_fn)(void);

    struct {
        const char *name;
        test_fn fn;
    } tests[] = {
        {"skips_whitespace_and_comments", test_skips_whitespace_and_comments},
        {"number_tokens", test_number_tokens},
        {"string_literal", test_string_literal},
    };

    size_t test_count = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < test_count; ++i) {
        int result = tests[i].fn();
        if (result != EXIT_SUCCESS) {
            fprintf(stderr, "Test '%s' failed.\n", tests[i].name);
            return result;
        }
    }

    printf("All lexer tests passed (%zu cases).\n", test_count);
    return EXIT_SUCCESS;
}
