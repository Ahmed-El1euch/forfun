#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend/codegen.h"
#include "frontend/parser.h"

#define ASSERT_TRUE(cond, msg)                                                                   \
    do {                                                                                          \
        if (!(cond)) {                                                                            \
            fprintf(stderr, "Assertion failed: %s (line %d): %s\n", __FILE__, __LINE__, msg);   \
            return EXIT_FAILURE;                                                                  \
        }                                                                                         \
    } while (0)

static int read_file_to_buffer(FILE *file, char *buffer, size_t size) {
    if (fflush(file) != 0) {
        return -1;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        return -1;
    }
    size_t read = fread(buffer, 1, size - 1, file);
    buffer[read] = '\0';
    return (int)read;
}

static int test_codegen_return_literal(void) {
    const char *source = "int main() { return 42; }";

    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");

    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL, "tmpfile should succeed");

    ASSERT_TRUE(codegen_emit_translation_unit(unit, tmp) == 0, "Codegen should succeed");

    char buffer[512];
    int read = read_file_to_buffer(tmp, buffer, sizeof(buffer));
    ASSERT_TRUE(read > 0, "Expected codegen output");

    ASSERT_TRUE(strstr(buffer, ".globl main\nmain:") != NULL, "Missing function label");
    ASSERT_TRUE(strstr(buffer, "    movl $42, %eax\n") != NULL, "Missing literal load");
    ASSERT_TRUE(strstr(buffer, "    leave\n    ret\n") != NULL, "Missing leave/ret");

    fclose(tmp);
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_codegen_return_identifier(void) {
    const char *source = "int foo() { return bar; }";
    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");
    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL, "tmpfile should succeed");

    ASSERT_TRUE(codegen_emit_translation_unit(unit, tmp) == 0, "Codegen should succeed");

    char buffer[512];
    ASSERT_TRUE(read_file_to_buffer(tmp, buffer, sizeof(buffer)) > 0, "Expected output");
    ASSERT_TRUE(strstr(buffer, "mov bar(%rip), %eax") != NULL, "Expected identifier load");

    fclose(tmp);
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_codegen_binary_expression(void) {
    const char *source = "int main() { return 20 + 22 - 2; }";
    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);
    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");

    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL, "tmpfile should succeed");
    ASSERT_TRUE(codegen_emit_translation_unit(unit, tmp) == 0, "Codegen should succeed");

    char buffer[1024];
    ASSERT_TRUE(read_file_to_buffer(tmp, buffer, sizeof(buffer)) > 0, "Expected output");
    ASSERT_TRUE(strstr(buffer, "push %rax") != NULL, "Push missing");
    ASSERT_TRUE(strstr(buffer, "add %edx, %eax") != NULL, "Add instruction missing");
    ASSERT_TRUE(strstr(buffer, "sub %edx, %eax") != NULL, "Sub instruction missing");

    fclose(tmp);
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_codegen_unary_minus(void) {
    const char *source = "int foo() { return -5; }";
    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);
    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");

    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL, "tmpfile should succeed");
    ASSERT_TRUE(codegen_emit_translation_unit(unit, tmp) == 0, "Codegen should succeed");

    char buffer[512];
    ASSERT_TRUE(read_file_to_buffer(tmp, buffer, sizeof(buffer)) > 0, "Expected output");
    ASSERT_TRUE(strstr(buffer, "movl $5, %eax") != NULL, "Literal load missing");
    ASSERT_TRUE(strstr(buffer, "neg %eax") != NULL, "neg instruction missing");

    fclose(tmp);
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_codegen_locals(void) {
    const char *source = "int main() { int x = 1; x = x + 2; return x; }";
    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);
    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");

    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL, "tmpfile should succeed");
    ASSERT_TRUE(codegen_emit_translation_unit(unit, tmp) == 0, "Codegen should succeed");

    char buffer[2048];
    ASSERT_TRUE(read_file_to_buffer(tmp, buffer, sizeof(buffer)) > 0, "Expected output");
    ASSERT_TRUE(strstr(buffer, "sub $16, %rsp") != NULL || strstr(buffer, "sub $8, %rsp") != NULL,
                "Stack allocation missing");
    ASSERT_TRUE(strstr(buffer, "movl %eax, -" ) != NULL, "Store to local missing");
    ASSERT_TRUE(strstr(buffer, "movl -") != NULL, "Load from local missing");

    fclose(tmp);
    ast_free(unit);
    return EXIT_SUCCESS;
}

int main(void) {
    typedef int (*test_fn)(void);
    struct {
        const char *name;
        test_fn fn;
    } tests[] = {
        {"codegen_return_literal", test_codegen_return_literal},
        {"codegen_return_identifier", test_codegen_return_identifier},
        {"codegen_binary_expression", test_codegen_binary_expression},
        {"codegen_unary_minus", test_codegen_unary_minus},
        {"codegen_locals", test_codegen_locals},
    };

    size_t count = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < count; ++i) {
        int result = tests[i].fn();
        if (result != EXIT_SUCCESS) {
            fprintf(stderr, "Test '%s' failed.\n", tests[i].name);
            return result;
        }
    }

    printf("All codegen tests passed (%zu cases).\n", count);
    return EXIT_SUCCESS;
}
