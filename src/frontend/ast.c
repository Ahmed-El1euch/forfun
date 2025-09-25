#include "frontend/ast.h"

#include <stdlib.h>

void ast_free(AstNode *node) {
    if (!node) {
        return;
    }

    switch (node->kind) {
    case AST_TRANSLATION_UNIT:
        for (size_t i = 0; i < node->value.translation_unit.function_count; ++i) {
            ast_free(node->value.translation_unit.functions[i]);
        }
        free(node->value.translation_unit.functions);
        break;
    case AST_FUNCTION_DECL:
        ast_free(node->value.function_decl.body);
        break;
    case AST_RETURN_STMT:
        ast_free(node->value.return_stmt.expression);
        break;
    case AST_NUMBER_LITERAL:
    case AST_IDENTIFIER:
        break;
    case AST_BINARY_EXPR:
        ast_free(node->value.binary_expr.left);
        ast_free(node->value.binary_expr.right);
        break;
    }

    free(node);
}
