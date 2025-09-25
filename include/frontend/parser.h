#ifndef FUNGCC_FRONTEND_PARSER_H
#define FUNGCC_FRONTEND_PARSER_H

#include "frontend/ast.h"
#include "frontend/lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ParserStatus {
    PARSER_OK = 0,
    PARSER_ERROR
} ParserStatus;

typedef struct Parser {
    Lexer lexer;
    Token current;
    ParserStatus status;
} Parser;

void parser_init(Parser *parser, const char *source, size_t length);
AstNode *parser_parse_translation_unit(Parser *parser);
ParserStatus parser_status(const Parser *parser);

#ifdef __cplusplus
}
#endif

#endif /* FUNGCC_FRONTEND_PARSER_H */
