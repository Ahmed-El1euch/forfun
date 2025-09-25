#include <stdio.h>
#include <string.h>

#include "backend/codegen.h"
#include "frontend/parser.h"

static void dump_function(const AstNode *func) {
    if (!func || func->kind != AST_FUNCTION_DECL) {
        puts("<not a function>");
        return;
    }

    printf("Function: %.*s\n", (int)func->value.function_decl.name.length, func->value.function_decl.name.name);

    const AstNode *body = func->value.function_decl.body;
    if (!body || body->kind != AST_RETURN_STMT) {
        puts("  <body not parsed>");
        return;
    }

    const AstNode *expr = body->value.return_stmt.expression;
    if (expr && expr->kind == AST_NUMBER_LITERAL) {
        printf("  return literal: %.*s\n", (int)expr->value.number_literal.length, expr->value.number_literal.lexeme);
    } else if (expr && expr->kind == AST_IDENTIFIER) {
        printf("  return identifier: %.*s\n", (int)expr->value.identifier.length, expr->value.identifier.name);
    } else {
        puts("  return <unknown>");
    }
}

int main(void) {
    const char *demo = "int main() { return 42; }\n";

    Parser parser;
    parser_init(&parser, demo, strlen(demo));

    AstNode *unit = parser_parse_translation_unit(&parser);
    if (parser_status(&parser) != PARSER_OK) {
        fputs("Parse failed.\n", stderr);
        ast_free(unit);
        return 1;
    }

    puts("fungcc parser demo:");
    for (size_t i = 0; i < unit->value.translation_unit.function_count; ++i) {
        dump_function(unit->value.translation_unit.functions[i]);
    }

    const char *asm_path = "build/fungcc_output.s";
    FILE *asm_file = fopen(asm_path, "w");
    if (!asm_file) {
        perror("fopen");
        ast_free(unit);
        return 1;
    }

    if (codegen_emit_translation_unit(unit, asm_file) != 0) {
        fputs("Code generation failed.\n", stderr);
        fclose(asm_file);
        ast_free(unit);
        return 1;
    }

    fclose(asm_file);
    printf("Assembly written to %s\n", asm_path);

    ast_free(unit);
    return 0;
}
