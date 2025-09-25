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

    const char *expected =
        ".text\n"
        ".globl main\n"
        "main:\n"
        "    push %rbp\n"
        "    mov %rsp, %rbp\n"
        "    mov $42, %eax\n"
        "    pop %rbp\n"
        "    ret\n\n"
        ".section .note.GNU-stack,\"\",@progbits\n";

    ASSERT_TRUE(strcmp(buffer, expected) == 0, "Assembly output mismatch");

    fclose(tmp);
    ast_free(unit);
    return EXIT_SUCCESS;
}

static int test_codegen_return_identifier_fails(void) {
    const char *source = "int foo() { return bar; }";
    Parser parser;
    parser_init(&parser, source, strlen(source));
    AstNode *unit = parser_parse_translation_unit(&parser);

    ASSERT_TRUE(parser_status(&parser) == PARSER_OK, "Parser should succeed");
    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL, "tmpfile should succeed");

    int status = codegen_emit_translation_unit(unit, tmp);
    ASSERT_TRUE(status != 0, "Codegen should fail for identifiers");

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
        {"codegen_return_identifier_fails", test_codegen_return_identifier_fails},
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
