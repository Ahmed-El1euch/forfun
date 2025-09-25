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
    AST_UNARY_EXPR,
    AST_BINARY_EXPR,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_ASSIGNMENT
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

typedef enum AstUnaryOp {
    AST_UNARY_PLUS = 0,
    AST_UNARY_MINUS
} AstUnaryOp;

typedef struct AstUnaryExpr {
    AstUnaryOp op;
    AstNode *operand;
} AstUnaryExpr;

typedef struct AstBinaryExpr {
    AstBinaryOp op;
    AstNode *left;
    AstNode *right;
} AstBinaryExpr;

typedef struct AstBlock {
    AstNode **statements;
    size_t statement_count;
} AstBlock;

typedef struct AstVarDecl {
    AstIdentifier name;
    AstNode *initializer; /* optional */
} AstVarDecl;

typedef struct AstAssignment {
    AstIdentifier target;
    AstNode *value;
} AstAssignment;

typedef struct AstFunctionDecl {
    AstIdentifier name;
    AstNode *body; /* AST_BLOCK */
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
        AstUnaryExpr unary_expr;
        AstBinaryExpr binary_expr;
        AstBlock block;
        AstVarDecl var_decl;
        AstAssignment assignment;
    } value;
} AstNode;

void ast_free(AstNode *node);

#ifdef __cplusplus
}
#endif

#endif /* FUNGCC_FRONTEND_AST_H */
