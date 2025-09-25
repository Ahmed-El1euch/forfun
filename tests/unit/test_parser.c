#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frontend/parser.h"

#define ASSERT_TRUE(cond, msg)                                                                   \
    do {                                                                                          \
        if (!(cond)) {                                                                            \
            fprintf(stderr, "Assertion failed: %s (line %d): %s\n", __FILE__, __LINE__, msg);   \
            return EXIT_FAILURE;                                                                  \
        }                                                                                         \
    } while (0)

static int test_parse_simple_function(void) {
    const char *source = "int main() { return 42; }";

    Parser parser;
    parser_init(&parser, source, strlen(source));

    AstNode *unit = parser_parse_translation_unit(&parser);
    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");
    ASSERT_TRUE(unit != NULL, "Translation unit should not be NULL");
    ASSERT_TRUE(unit->kind == AST_TRANSLATION_UNIT, "Root should be translation unit");
    ASSERT_TRUE(unit->value.translation_unit.function_count == 1, "Expect one function");

    AstNode *func = unit->value.translation_unit.functions[0];
    ASSERT_TRUE(func->kind == AST_FUNCTION_DECL, "Expect function decl");
    ASSERT_TRUE(func->value.function_decl.name.length == 4, "Function name length");
    ASSERT_TRUE(strncmp(func->value.function_decl.name.name, "main", 4) == 0, "Function name");

    AstNode *body = func->value.function_decl.body;
    ASSERT_TRUE(body && body->kind == AST_RETURN_STMT, "Expect return statement");

    AstNode *expr = body->value.return_stmt.expression;
    ASSERT_TRUE(expr && expr->kind == AST_NUMBER_LITERAL, "Expect number literal in return");
    ASSERT_TRUE(strncmp(expr->value.number_literal.lexeme, "42", 2) == 0, "Literal lexeme");

    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_identifier_return(void) {
    const char *source = "int foo() { return bar; }";

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");
    ASSERT_TRUE(unit->value.translation_unit.function_count == 1, "Function count");

    AstNode *func = unit->value.translation_unit.functions[0];
    AstNode *body = func->value.function_decl.body;
    AstNode *expr = body->value.return_stmt.expression;
    ASSERT_TRUE(expr->kind == AST_IDENTIFIER, "Return identifier");
    ASSERT_TRUE(strncmp(expr->value.identifier.name, "bar", 3) == 0, "Identifier name");

    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_failure_on_missing_semicolon(void) {
    const char *source = "int main() { return 42 }"; // missing semicolon

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_ERROR, "Parser should report error");
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_multiple_functions(void) {
    const char *source =
        "int main() { return 1; }"
        "int foo() { return bar; }";

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");
    ASSERT_TRUE(unit->value.translation_unit.function_count == 2, "Expect two functions");

    AstNode *first = unit->value.translation_unit.functions[0];
    ASSERT_TRUE(strncmp(first->value.function_decl.name.name, "main", 4) == 0, "First function name");

    AstNode *second = unit->value.translation_unit.functions[1];
    ASSERT_TRUE(strncmp(second->value.function_decl.name.name, "foo", 3) == 0, "Second function name");

    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_failure_on_bad_token(void) {
    const char *source = "int main( { return 0; }"; // missing ')'

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_ERROR, "Parser should report error");
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_failure_on_missing_expression(void) {
    const char *source = "int main() { return ; }";

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_ERROR, "Parser should report error for missing expr");
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_failure_on_missing_rbrace(void) {
    const char *source = "int main() { return 0;"; // missing closing brace

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_ERROR, "Parser should report error for missing '}'");
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_parse_failure_on_unexpected_keyword(void) {
    const char *source = "int return() { return 0; }"; // illegal identifier (keyword)

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_ERROR, "Parser should report error on unexpected keyword");
    ast_free(unit);
    return EXIT_SUCCESS;
}

int main(void) {
    typedef int (*test_fn)(void);
    struct {
        const char *name;
        test_fn fn;
    } tests[] = {
        {"parse_simple_function", test_parse_simple_function},
        {"parse_identifier_return", test_parse_identifier_return},
        {"parse_failure_on_missing_semicolon", test_parse_failure_on_missing_semicolon},
        {"parse_multiple_functions", test_parse_multiple_functions},
        {"parse_failure_on_bad_token", test_parse_failure_on_bad_token},
        {"parse_failure_on_missing_expression", test_parse_failure_on_missing_expression},
        {"parse_failure_on_missing_rbrace", test_parse_failure_on_missing_rbrace},
        {"parse_failure_on_unexpected_keyword", test_parse_failure_on_unexpected_keyword},
    };

    size_t count = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < count; ++i) {
        int result = tests[i].fn();
        if (result != EXIT_SUCCESS) {
            fprintf(stderr, "Test '%s' failed.\n", tests[i].name);
            return result;
        }
    }

    printf("All parser tests passed (%zu cases).\n", count);
    return EXIT_SUCCESS;
}
