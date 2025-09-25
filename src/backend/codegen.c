#include "backend/codegen.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int copy_lexeme(const char *lexeme, size_t length, char **out_copy) {
    char *buffer = malloc(length + 1);
    if (!buffer) {
        return -1;
    }
    memcpy(buffer, lexeme, length);
    buffer[length] = '\0';
    *out_copy = buffer;
    return 0;
}

static int emit_expression(const AstNode *node, FILE *out);

static int emit_number_literal(const AstNode *node, FILE *out) {
    char *literal = NULL;
    if (copy_lexeme(node->value.number_literal.lexeme, node->value.number_literal.length, &literal) != 0) {
        return -1;
    }

    char *endptr = NULL;
    long value = strtol(literal, &endptr, 10);
    if (endptr == literal || *endptr != '\0') {
        free(literal);
        return -1;
    }

    int result = fprintf(out, "    mov $%ld, %%eax\n", value);
    free(literal);
    return (result < 0) ? -1 : 0;
}

static int emit_identifier(const AstNode *node, FILE *out) {
    char *name = NULL;
    if (copy_lexeme(node->value.identifier.name, node->value.identifier.length, &name) != 0) {
        return -1;
    }

    int result = fprintf(out, "    mov %s(%%rip), %%eax\n", name);
    free(name);
    return (result < 0) ? -1 : 0;
}

static int emit_binary_expr(const AstNode *node, FILE *out) {
    if (emit_expression(node->value.binary_expr.left, out) != 0) {
        return -1;
    }

    if (fprintf(out, "    push %%rax\n") < 0) {
        return -1;
    }

    if (emit_expression(node->value.binary_expr.right, out) != 0) {
        return -1;
    }

    if (fprintf(out, "    pop %%rcx\n") < 0) {
        return -1;
    }

    if (fprintf(out, "    mov %%eax, %%edx\n") < 0) {
        return -1;
    }

    if (fprintf(out, "    mov %%ecx, %%eax\n") < 0) {
        return -1;
    }

    const char *op_instr = (node->value.binary_expr.op == AST_BIN_ADD) ? "add" : "sub";
    if (fprintf(out, "    %s %%edx, %%eax\n", op_instr) < 0) {
        return -1;
    }

    return 0;
}

static int emit_expression(const AstNode *node, FILE *out) {
    if (!node) {
        return -1;
    }

    switch (node->kind) {
    case AST_NUMBER_LITERAL:
        return emit_number_literal(node, out);
    case AST_IDENTIFIER:
        return emit_identifier(node, out);
    case AST_BINARY_EXPR:
        return emit_binary_expr(node, out);
    default:
        break;
    }

    return -1;
}

static int emit_return_stmt(const AstNode *node, FILE *out) {
    const AstNode *expr = node->value.return_stmt.expression;
    return emit_expression(expr, out);
}

static int emit_function(const AstNode *node, FILE *out) {
    char *name = NULL;
    if (copy_lexeme(node->value.function_decl.name.name, node->value.function_decl.name.length, &name) != 0) {
        return -1;
    }

    int status = 0;

    if (fprintf(out, ".globl %s\n%s:\n", name, name) < 0) {
        status = -1;
        goto cleanup;
    }

    if (fprintf(out, "    push %%rbp\n    mov %%rsp, %%rbp\n") < 0) {
        status = -1;
        goto cleanup;
    }

    if (emit_return_stmt(node->value.function_decl.body, out) != 0) {
        status = -1;
        goto cleanup;
    }

    if (fprintf(out, "    pop %%rbp\n    ret\n\n") < 0) {
        status = -1;
        goto cleanup;
    }

cleanup:
    free(name);
    return status;
}

int codegen_emit_translation_unit(const AstNode *unit, FILE *out) {
    if (!unit || unit->kind != AST_TRANSLATION_UNIT || !out) {
        return -1;
    }

    if (fprintf(out, ".text\n") < 0) {
        return -1;
    }

    for (size_t i = 0; i < unit->value.translation_unit.function_count; ++i) {
        const AstNode *func = unit->value.translation_unit.functions[i];
        if (!func || func->kind != AST_FUNCTION_DECL) {
            return -1;
        }
        if (emit_function(func, out) != 0) {
            return -1;
        }
    }

    if (fprintf(out, ".section .note.GNU-stack,\"\",@progbits\n") < 0) {
        return -1;
    }

    return 0;
}
