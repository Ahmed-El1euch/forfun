#include <stdio.h>
#include <string.h>

#include "backend/codegen.h"
#include "frontend/parser.h"

static void dump_block(const AstNode *block, int indent);

static void dump_function(const AstNode *func) {
    if (!func || func->kind != AST_FUNCTION_DECL) {
        puts("<not a function>");
        return;
    }

    printf("Function: %.*s\n", (int)func->value.function_decl.name.length, func->value.function_decl.name.name);

    const AstNode *body = func->value.function_decl.body;
    if (!body || body->kind != AST_BLOCK) {
        puts("  <body not parsed>");
        return;
    }

    dump_block(body, 2);
}

static void dump_expression_summary(const AstNode *expr) {
    if (!expr) {
        printf("<empty>");
        return;
    }

    switch (expr->kind) {
    case AST_NUMBER_LITERAL:
        printf("literal %.*s", (int)expr->value.number_literal.length, expr->value.number_literal.lexeme);
        break;
    case AST_IDENTIFIER:
        printf("identifier %.*s", (int)expr->value.identifier.length, expr->value.identifier.name);
        break;
    case AST_UNARY_EXPR:
        printf("unary %s ", expr->value.unary_expr.op == AST_UNARY_MINUS ? "-" : "+");
        dump_expression_summary(expr->value.unary_expr.operand);
        break;
    case AST_BINARY_EXPR:
        dump_expression_summary(expr->value.binary_expr.left);
        printf(" %s ", expr->value.binary_expr.op == AST_BIN_ADD ? "+" : "-");
        dump_expression_summary(expr->value.binary_expr.right);
        break;
    default:
        printf("<expr>");
        break;
    }
}

static void dump_block(const AstNode *block, int indent) {
    if (!block || block->kind != AST_BLOCK) {
        return;
    }

    for (size_t i = 0; i < block->value.block.statement_count; ++i) {
        const AstNode *stmt = block->value.block.statements[i];
        printf("%*s- ", indent, "");
        switch (stmt->kind) {
        case AST_VAR_DECL:
            printf("var %.*s = ", (int)stmt->value.var_decl.name.length, stmt->value.var_decl.name.name);
            dump_expression_summary(stmt->value.var_decl.initializer);
            printf("\n");
            break;
        case AST_ASSIGNMENT:
            printf("assign %.*s = ", (int)stmt->value.assignment.target.length, stmt->value.assignment.target.name);
            dump_expression_summary(stmt->value.assignment.value);
            printf("\n");
            break;
        case AST_RETURN_STMT:
            printf("return ");
            dump_expression_summary(stmt->value.return_stmt.expression);
            printf("\n");
            break;
        case AST_BLOCK:
            printf("block\n");
            dump_block(stmt, indent + 2);
            break;
        default:
            printf("stmt kind %d\n", stmt->kind);
            break;
        }
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
