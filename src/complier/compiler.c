#include "compiler.h"
#include "scanner/scanner.h"
// added for free token
#include <stdlib.h>
// added for print token
#include <stdio.h>

void compile(const char *source) {
    Scanner scanner;
    init_scanner(source, &scanner);
    for (int line = 0, end = 0; !end;) {
        Token *token = scan_token(&scanner);
        // format line info
        if (token->location.line != line) {
            fprintf(stdout, "%4d", token->location.line);
            line = token->location.line;
        } else fprintf(stdout, "   |");

        // stringify token 
        fprintf(stdout, " %2d '%.*s'\n", token->type, token->length, token->lexeme);

        if (token->type == CLOX_TOKEN_EOF) end = 1;
        free(token);
    }
}