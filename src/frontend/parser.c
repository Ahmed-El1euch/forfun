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

    fprintf(stderr, "Parser error at line %zu col %zu: unexpected token %d\n",
            token.line,
            token.column,
            token.kind);
    parser->status = PARSER_ERROR;
    return NULL;
}

static AstNode *parse_return_statement(Parser *parser) {
    parser_expect(parser, TOKEN_KW_RETURN, "'return'");
    AstNode *expr = parse_primary(parser);
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

static AstNode *parse_function_declaration(Parser *parser) {
    parser_expect(parser, TOKEN_KW_INT, "'int'");

    Token name = parser_peek(parser);
    parser_expect(parser, TOKEN_IDENTIFIER, "function name");

    parser_expect(parser, TOKEN_L_PAREN, "'('");
    parser_expect(parser, TOKEN_R_PAREN, "')'");

    parser_expect(parser, TOKEN_L_BRACE, "'{' ");

    AstNode *body = parse_return_statement(parser);

    parser_expect(parser, TOKEN_R_BRACE, "'}'");

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
