#include "frontend/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token parser_advance(Parser *parser) {
    parser->current = lexer_next_token(&parser->lexer);
    return parser->current;
}

static Token parser_peek(Parser *parser) {
    return parser->current;
}

static int parser_match(Parser *parser, TokenKind kind) {
    if (parser->current.kind == kind) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

static void parser_expect(Parser *parser, TokenKind kind, const char *message) {
    if (!parser_match(parser, kind)) {
        fprintf(stderr, "Parser error at line %zu col %zu: expected %s\n",
                parser->current.line,
                parser->current.column,
                message);
        parser->status = PARSER_ERROR;
    }
}

static AstNode *ast_new_node(AstNodeKind kind) {
    AstNode *node = calloc(1, sizeof(AstNode));
    if (!node) {
        return NULL;
    }
    node->kind = kind;
    return node;
}

static AstNode *parse_expression(Parser *parser);
static AstNode *parse_unary(Parser *parser);
static AstNode *parse_statement(Parser *parser);
static AstNode *parse_block(Parser *parser);

static AstNode *parse_primary(Parser *parser) {
    Token token = parser_peek(parser);
    if (token.kind == TOKEN_NUMBER) {
        AstNode *literal = ast_new_node(AST_NUMBER_LITERAL);
        if (!literal) {
            parser->status = PARSER_ERROR;
            return NULL;
        }
        literal->value.number_literal.lexeme = token.lexeme;
        literal->value.number_literal.length = token.length;
        parser_advance(parser);
        return literal;
    }

    if (token.kind == TOKEN_IDENTIFIER) {
        AstNode *ident = ast_new_node(AST_IDENTIFIER);
        if (!ident) {
            parser->status = PARSER_ERROR;
            return NULL;
        }
        ident->value.identifier.name = token.lexeme;
        ident->value.identifier.length = token.length;
        parser_advance(parser);
        return ident;
    }

    if (token.kind == TOKEN_L_PAREN) {
        parser_advance(parser);
        AstNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_R_PAREN, "')'");
        if (parser->status == PARSER_ERROR) {
            ast_free(expr);
            return NULL;
        }
        return expr;
    }

    fprintf(stderr, "Parser error at line %zu col %zu: unexpected token %d\n",
            token.line,
            token.column,
            token.kind);
    parser->status = PARSER_ERROR;
    return NULL;
}

static AstNode *parse_unary(Parser *parser) {
    Token token = parser_peek(parser);
    if (token.kind == TOKEN_PLUS || token.kind == TOKEN_MINUS) {
        parser_advance(parser);
        AstNode *operand = parse_unary(parser);
        if (!operand) {
            return NULL;
        }

        AstNode *node = ast_new_node(AST_UNARY_EXPR);
        if (!node) {
            parser->status = PARSER_ERROR;
            ast_free(operand);
            return NULL;
        }

        node->value.unary_expr.op = (token.kind == TOKEN_PLUS) ? AST_UNARY_PLUS : AST_UNARY_MINUS;
        node->value.unary_expr.operand = operand;
        return node;
    }

    return parse_primary(parser);
}

static AstNode *parse_expression(Parser *parser) {
    AstNode *left = parse_unary(parser);
    if (!left) {
        return NULL;
    }

    while (parser->current.kind == TOKEN_PLUS || parser->current.kind == TOKEN_MINUS) {
        Token op = parser->current;
        parser_advance(parser);

        AstNode *right = parse_unary(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        AstNode *binary = ast_new_node(AST_BINARY_EXPR);
        if (!binary) {
            parser->status = PARSER_ERROR;
            ast_free(left);
            ast_free(right);
            return NULL;
        }

        binary->value.binary_expr.left = left;
        binary->value.binary_expr.right = right;
        binary->value.binary_expr.op = (op.kind == TOKEN_PLUS) ? AST_BIN_ADD : AST_BIN_SUB;
        left = binary;
    }

    return left;
}

static AstNode *parse_return_statement(Parser *parser) {
    parser_expect(parser, TOKEN_KW_RETURN, "'return'");
    AstNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON, "';'");
    if (parser->status == PARSER_ERROR) {
        ast_free(expr);
        return NULL;
    }

    AstNode *node = ast_new_node(AST_RETURN_STMT);
    if (!node) {
        parser->status = PARSER_ERROR;
        ast_free(expr);
        return NULL;
    }
    node->value.return_stmt.expression = expr;
    return node;
}

static AstNode *parse_var_declaration(Parser *parser) {
    parser_expect(parser, TOKEN_KW_INT, "'int'");

    Token name = parser_peek(parser);
    parser_expect(parser, TOKEN_IDENTIFIER, "identifier");

    AstNode *initializer = NULL;
    if (parser->current.kind == TOKEN_EQUAL) {
        parser_advance(parser);
        initializer = parse_expression(parser);
    }

    parser_expect(parser, TOKEN_SEMICOLON, "';'");

    if (parser->status == PARSER_ERROR) {
        ast_free(initializer);
        return NULL;
    }

    AstNode *node = ast_new_node(AST_VAR_DECL);
    if (!node) {
        parser->status = PARSER_ERROR;
        ast_free(initializer);
        return NULL;
    }

    node->value.var_decl.name.name = name.lexeme;
    node->value.var_decl.name.length = name.length;
    node->value.var_decl.initializer = initializer;
    return node;
}

static AstNode *parse_assignment_statement(Parser *parser) {
    Token name = parser_peek(parser);
    parser_expect(parser, TOKEN_IDENTIFIER, "identifier");
    parser_expect(parser, TOKEN_EQUAL, "'='");

    AstNode *value = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON, "';'");

    if (parser->status == PARSER_ERROR) {
        ast_free(value);
        return NULL;
    }

    AstNode *node = ast_new_node(AST_ASSIGNMENT);
    if (!node) {
        parser->status = PARSER_ERROR;
        ast_free(value);
        return NULL;
    }

    node->value.assignment.target.name = name.lexeme;
    node->value.assignment.target.length = name.length;
    node->value.assignment.value = value;
    return node;
}

static AstNode *parse_statement(Parser *parser) {
    switch (parser->current.kind) {
    case TOKEN_KW_INT:
        return parse_var_declaration(parser);
    case TOKEN_KW_RETURN:
        return parse_return_statement(parser);
    case TOKEN_IDENTIFIER:
        return parse_assignment_statement(parser);
    case TOKEN_L_BRACE:
        parser_advance(parser); /* consume '{' */
        return parse_block(parser);
    default:
        fprintf(stderr, "Parser error at line %zu col %zu: unexpected token %d in statement\n",
                parser->current.line,
                parser->current.column,
                parser->current.kind);
        parser->status = PARSER_ERROR;
        return NULL;
    }
}

static AstNode *parse_block(Parser *parser) {
    AstNode *block = ast_new_node(AST_BLOCK);
    if (!block) {
        parser->status = PARSER_ERROR;
        return NULL;
    }

    size_t capacity = 4;
    block->value.block.statements = calloc(capacity, sizeof(AstNode *));
    if (!block->value.block.statements) {
        parser->status = PARSER_ERROR;
        return block;
    }

    block->value.block.statement_count = 0;

    while (parser->current.kind != TOKEN_R_BRACE && parser->current.kind != TOKEN_EOF && parser->status == PARSER_OK) {
        AstNode *statement = parse_statement(parser);
        if (!statement) {
            break;
        }

        if (block->value.block.statement_count == capacity) {
            capacity *= 2;
            AstNode **resized = realloc(block->value.block.statements, capacity * sizeof(AstNode *));
            if (!resized) {
                parser->status = PARSER_ERROR;
                ast_free(statement);
                break;
            }
            block->value.block.statements = resized;
        }

        block->value.block.statements[block->value.block.statement_count++] = statement;
    }

    parser_expect(parser, TOKEN_R_BRACE, "'}'");
    return block;
}

static AstNode *parse_function_declaration(Parser *parser) {
    parser_expect(parser, TOKEN_KW_INT, "'int'");

    Token name = parser_peek(parser);
    parser_expect(parser, TOKEN_IDENTIFIER, "function name");

    parser_expect(parser, TOKEN_L_PAREN, "'('");
    parser_expect(parser, TOKEN_R_PAREN, "')'");

    parser_expect(parser, TOKEN_L_BRACE, "'{' ");
    AstNode *body = parse_block(parser);

    if (parser->status == PARSER_ERROR) {
        ast_free(body);
        return NULL;
    }

    AstNode *func = ast_new_node(AST_FUNCTION_DECL);
    if (!func) {
        parser->status = PARSER_ERROR;
        ast_free(body);
        return NULL;
    }

    func->value.function_decl.name.name = name.lexeme;
    func->value.function_decl.name.length = name.length;
    func->value.function_decl.body = body;
    return func;
}

static AstNode *parse_translation_unit(Parser *parser) {
    AstNode *unit = ast_new_node(AST_TRANSLATION_UNIT);
    if (!unit) {
        parser->status = PARSER_ERROR;
        return NULL;
    }

    const size_t initial_capacity = 4;
    unit->value.translation_unit.functions = calloc(initial_capacity, sizeof(AstNode *));
    unit->value.translation_unit.function_count = 0;
    size_t capacity = initial_capacity;

    while (parser->current.kind != TOKEN_EOF && parser->status == PARSER_OK) {
        if (unit->value.translation_unit.function_count == capacity) {
            capacity *= 2;
            AstNode **resized = realloc(unit->value.translation_unit.functions,
                                        capacity * sizeof(AstNode *));
            if (!resized) {
                parser->status = PARSER_ERROR;
                break;
            }
            unit->value.translation_unit.functions = resized;
        }

        AstNode *func = parse_function_declaration(parser);
        if (!func) {
            break;
        }
        unit->value.translation_unit.functions[unit->value.translation_unit.function_count++] = func;
    }

    return unit;
}

void parser_init(Parser *parser, const char *source, size_t length) {
    parser->status = PARSER_OK;
    lexer_init(&parser->lexer, source, length);
    parser_advance(parser);
}

AstNode *parser_parse_translation_unit(Parser *parser) {
    return parse_translation_unit(parser);
}

ParserStatus parser_status(const Parser *parser) {
    return parser->status;
}
