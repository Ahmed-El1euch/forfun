#include <stdio.h>
#include <string.h>

#include "frontend/lexer.h"

static void dump_tokens(const char *source) {
    Lexer lexer;
    lexer_init(&lexer, source, strlen(source));

    for (;;) {
        Token token = lexer_next_token(&lexer);
        printf("%-16d line=%zu col=%zu lexeme='%.*s'\n",
               token.kind,
               token.line,
               token.column,
               (int)token.length,
               token.lexeme);
        if (token.kind == TOKEN_EOF) {
            break;
        }
    }
}

int main(void) {
    const char *demo = "int main() { return 42; }\n";
    puts("fungcc driver stub: lexing demo.");
    dump_tokens(demo);
    return 0;
}
