#include "backend/codegen.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct LocalBinding {
    char *name;
    size_t length;
    long offset; /* positive offset from rbp (use -offset) */
} LocalBinding;

typedef struct LocalTable {
    LocalBinding *items;
    size_t count;
    size_t capacity;
} LocalTable;

typedef struct CodegenContext {
    FILE *out;
    LocalTable *locals;
    const char *return_label;
} CodegenContext;

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

static long local_table_find(const LocalTable *table, const char *name, size_t length) {
    for (size_t i = 0; i < table->count; ++i) {
        if (table->items[i].length == length && strncmp(table->items[i].name, name, length) == 0) {
            return table->items[i].offset;
        }
    }
    return -1;
}

static int local_table_add(LocalTable *table, const char *name, size_t length, long offset) {
    if (table->count == table->capacity) {
        size_t new_capacity = table->capacity ? table->capacity * 2 : 4;
        LocalBinding *resized = realloc(table->items, new_capacity * sizeof(LocalBinding));
        if (!resized) {
            return -1;
        }
        table->items = resized;
        table->capacity = new_capacity;
    }

    char *copy = NULL;
    if (copy_lexeme(name, length, &copy) != 0) {
        return -1;
    }

    table->items[table->count].name = copy;
    table->items[table->count].length = length;
    table->items[table->count].offset = offset;
    table->count += 1;
    return 0;
}

static void local_table_free(LocalTable *table) {
    for (size_t i = 0; i < table->count; ++i) {
        free(table->items[i].name);
    }
    free(table->items);
    table->items = NULL;
    table->count = table->capacity = 0;
}

static int emit_expression(const AstNode *node, CodegenContext *ctx);

static int emit_number_literal(const AstNode *node, CodegenContext *ctx) {
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

    int result = fprintf(ctx->out, "    movl $%ld, %%eax\n", value);
    free(literal);
    return (result < 0) ? -1 : 0;
}

static int emit_identifier(const AstNode *node, CodegenContext *ctx) {
    long offset = local_table_find(ctx->locals, node->value.identifier.name, node->value.identifier.length);
    if (offset >= 0) {
        if (fprintf(ctx->out, "    movl -%ld(%%rbp), %%eax\n", offset) < 0) {
            return -1;
        }
        return 0;
    }

    char *name = NULL;
    if (copy_lexeme(node->value.identifier.name, node->value.identifier.length, &name) != 0) {
        return -1;
    }

    int result = fprintf(ctx->out, "    mov %s(%%rip), %%eax\n", name);
    free(name);
    return (result < 0) ? -1 : 0;
}

static int emit_binary_expr(const AstNode *node, CodegenContext *ctx) {
    if (emit_expression(node->value.binary_expr.left, ctx) != 0) {
        return -1;
    }

    if (fprintf(ctx->out, "    push %%rax\n") < 0) {
        return -1;
    }

    if (emit_expression(node->value.binary_expr.right, ctx) != 0) {
        return -1;
    }

    if (fprintf(ctx->out, "    pop %%rcx\n") < 0) {
        return -1;
    }

    if (fprintf(ctx->out, "    mov %%eax, %%edx\n") < 0) {
        return -1;
    }

    if (fprintf(ctx->out, "    mov %%ecx, %%eax\n") < 0) {
        return -1;
    }

    const char *op_instr = (node->value.binary_expr.op == AST_BIN_ADD) ? "add" : "sub";
    if (fprintf(ctx->out, "    %s %%edx, %%eax\n", op_instr) < 0) {
        return -1;
    }

    return 0;
}

static int emit_unary_expr(const AstNode *node, CodegenContext *ctx) {
    if (emit_expression(node->value.unary_expr.operand, ctx) != 0) {
        return -1;
    }

    switch (node->value.unary_expr.op) {
    case AST_UNARY_PLUS:
        return 0;
    case AST_UNARY_MINUS:
        if (fprintf(ctx->out, "    neg %%eax\n") < 0) {
            return -1;
        }
        return 0;
    default:
        break;
    }

    return -1;
}

static int emit_expression(const AstNode *node, CodegenContext *ctx) {
    if (!node) {
        return -1;
    }

    switch (node->kind) {
    case AST_NUMBER_LITERAL:
        return emit_number_literal(node, ctx);
    case AST_IDENTIFIER:
        return emit_identifier(node, ctx);
    case AST_UNARY_EXPR:
        return emit_unary_expr(node, ctx);
    case AST_BINARY_EXPR:
        return emit_binary_expr(node, ctx);
    default:
        break;
    }

    return -1;
}

static int emit_statement(const AstNode *node, CodegenContext *ctx);

static int emit_return_stmt(const AstNode *node, CodegenContext *ctx) {
    const AstNode *expr = node->value.return_stmt.expression;
    if (emit_expression(expr, ctx) != 0) {
        return -1;
    }

    if (fprintf(ctx->out, "    jmp %s\n", ctx->return_label) < 0) {
        return -1;
    }

    return 0;
}

static int emit_statement(const AstNode *node, CodegenContext *ctx) {
    switch (node->kind) {
    case AST_RETURN_STMT:
        return emit_return_stmt(node, ctx);
    case AST_VAR_DECL: {
        long offset = local_table_find(ctx->locals,
                                       node->value.var_decl.name.name,
                                       node->value.var_decl.name.length);
        if (offset < 0) {
            fprintf(stderr, "Codegen error: declaration for %.*s not in local table\n",
                    (int)node->value.var_decl.name.length,
                    node->value.var_decl.name.name);
            return -1;
        }

        if (node->value.var_decl.initializer) {
            if (emit_expression(node->value.var_decl.initializer, ctx) != 0) {
                return -1;
            }
        } else {
            if (fprintf(ctx->out, "    movl $0, %%eax\n") < 0) {
                return -1;
            }
        }

        if (fprintf(ctx->out, "    movl %%eax, -%ld(%%rbp)\n", offset) < 0) {
            return -1;
        }
        return 0;
    }
    case AST_ASSIGNMENT: {
        long offset = local_table_find(ctx->locals,
                                       node->value.assignment.target.name,
                                       node->value.assignment.target.length);
        if (offset < 0) {
            fprintf(stderr, "Codegen error: assignment to undeclared identifier %.*s\n",
                    (int)node->value.assignment.target.length,
                    node->value.assignment.target.name);
            return -1;
        }

        if (emit_expression(node->value.assignment.value, ctx) != 0) {
            return -1;
        }

        if (fprintf(ctx->out, "    movl %%eax, -%ld(%%rbp)\n", offset) < 0) {
            return -1;
        }
        return 0;
    }
    case AST_BLOCK: {
        for (size_t i = 0; i < node->value.block.statement_count; ++i) {
            if (emit_statement(node->value.block.statements[i], ctx) != 0) {
                return -1;
            }
        }
        return 0;
    }
    default:
        fprintf(stderr, "Codegen error: unsupported statement kind %d\n", node->kind);
        return -1;
    }
}

static int collect_locals_block(const AstNode *block, LocalTable *table, long *offset) {
    for (size_t i = 0; i < block->value.block.statement_count; ++i) {
        const AstNode *stmt = block->value.block.statements[i];
        switch (stmt->kind) {
        case AST_VAR_DECL: {
            *offset += 8; /* reserve 8 bytes for 4-byte int to keep alignment simple */
            if (local_table_add(table,
                                stmt->value.var_decl.name.name,
                                stmt->value.var_decl.name.length,
                                *offset) != 0) {
                return -1;
            }
            break;
        }
        case AST_BLOCK:
            if (collect_locals_block(stmt, table, offset) != 0) {
                return -1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
}

static long align_to(long value, long alignment) {
    long remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }
    return value + (alignment - remainder);
}

static int emit_function(const AstNode *node, FILE *out) {
    char *name = NULL;
    if (copy_lexeme(node->value.function_decl.name.name, node->value.function_decl.name.length, &name) != 0) {
        return -1;
    }

    int status = 0;
    LocalTable locals = {0};
    long stack_usage = 0;

    if (node->value.function_decl.body && node->value.function_decl.body->kind == AST_BLOCK) {
        if (collect_locals_block(node->value.function_decl.body, &locals, &stack_usage) != 0) {
            status = -1;
            goto cleanup;
        }
    }

    long aligned_stack = align_to(stack_usage, 16);

    static int label_counter = 0;
    char return_label[64];
    snprintf(return_label, sizeof(return_label), ".Lreturn_%d", label_counter++);

    if (fprintf(out, ".globl %s\n%s:\n", name, name) < 0) {
        status = -1;
        goto cleanup;
    }

    if (fprintf(out, "    push %%rbp\n    mov %%rsp, %%rbp\n") < 0) {
        status = -1;
        goto cleanup;
    }

    if (aligned_stack > 0) {
        if (fprintf(out, "    sub $%ld, %%rsp\n", aligned_stack) < 0) {
            status = -1;
            goto cleanup;
        }
    }

    CodegenContext ctx = {
        .out = out,
        .locals = &locals,
        .return_label = return_label,
    };

    if (node->value.function_decl.body) {
        if (emit_statement(node->value.function_decl.body, &ctx) != 0) {
            status = -1;
            goto cleanup;
        }
    }

    if (fprintf(out, "%s:\n", return_label) < 0) {
        status = -1;
        goto cleanup;
    }

    if (fprintf(out, "    leave\n    ret\n\n") < 0) {
        status = -1;
        goto cleanup;
    }

cleanup:
    local_table_free(&locals);
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
