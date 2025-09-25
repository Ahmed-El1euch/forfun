#ifndef FUNGCC_FRONTEND_AST_H
#define FUNGCC_FRONTEND_AST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AstNodeKind {
    AST_TRANSLATION_UNIT = 0,
    AST_FUNCTION_DECL,
    AST_RETURN_STMT,
    AST_NUMBER_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY_EXPR
} AstNodeKind;

typedef struct AstNode AstNode;

typedef struct AstIdentifier {
    const char *name;
    size_t length;
} AstIdentifier;

typedef struct AstNumberLiteral {
    const char *lexeme;
    size_t length;
} AstNumberLiteral;

typedef struct AstReturnStmt {
    AstNode *expression;
} AstReturnStmt;

typedef enum AstBinaryOp {
    AST_BIN_ADD = 0,
    AST_BIN_SUB
} AstBinaryOp;

typedef struct AstBinaryExpr {
    AstBinaryOp op;
    AstNode *left;
    AstNode *right;
} AstBinaryExpr;

typedef struct AstFunctionDecl {
    AstIdentifier name;
    AstNode *body; /* Return statement only for now */
} AstFunctionDecl;

typedef struct AstTranslationUnit {
    AstNode **functions;
    size_t function_count;
} AstTranslationUnit;

typedef struct AstNode {
    AstNodeKind kind;
    union {
        AstTranslationUnit translation_unit;
        AstFunctionDecl function_decl;
        AstReturnStmt return_stmt;
        AstNumberLiteral number_literal;
        AstIdentifier identifier;
        AstBinaryExpr binary_expr;
    } value;
} AstNode;

void ast_free(AstNode *node);

#ifdef __cplusplus
}
#endif

#endif /* FUNGCC_FRONTEND_AST_H */
