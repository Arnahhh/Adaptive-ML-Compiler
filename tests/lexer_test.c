#include <stdio.h>
#include <stdlib.h>
#include "source.h"
#include "lexer.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    SourceFile input;
    if (!loadSourceFile(argv[1], &input)) {
        fprintf(stderr, "Could not read '%s'.\n", argv[1]);
        return 1;
    }

    int position = 0;
    resetLexer();

    printf("%-20s %-20s %-6s %-6s\n", "TOKEN", "VALUE", "LINE", "COLUMN");
    printf("--------------------------------------------------------------\n");

    while (1) {
        Token token = getNextToken(input.code, &position);
        printf("%-20s %-20s %-6d %-6d\n",
               tokenTypeToString(token.type),
               token.value,
               token.line,
               token.column);
        if (token.type == TOKEN_EOF)
            break;
    }

    freeSourceFile(&input);
    return 0;
}
