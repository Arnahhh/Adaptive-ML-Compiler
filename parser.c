#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "error.h"

static Token currentToken;
static Token previousToken;

void advance(const char* source, int* position) {
    previousToken = currentToken;
    currentToken = getNextToken(source, position);
}

void parserError(const char* message) {
    reportError(ERROR_INVALID_STATEMENT, currentToken.line, message,
                currentToken.value, "valid token");
}

void expect(TokenType type, const char* source, int* position) {
    if (currentToken.type == type) { advance(source, position); return; }
    if (type == TOKEN_SEMICOLON) {
        reportError(ERROR_MISSING_SEMICOLON, previousToken.line,
                    "Missing semicolon at end of statement.", currentToken.value, ";");
    } else if (type == TOKEN_RPAREN) {
        reportError(ERROR_MISSING_RPAREN, currentToken.line,
                    "Missing closing parenthesis.", currentToken.value, ")");
    } else {
        char message[160];
        snprintf(message, sizeof(message), "Expected %s but found '%s'.",
                 tokenTypeToString(type), currentToken.value);
        reportError(ERROR_INVALID_STATEMENT, currentToken.line, message,
                    currentToken.value, tokenTypeToString(type));
    }
}

static ASTNode* binaryNode(ASTNode* left, const char* op, ASTNode* right) {
    ASTNode* n = createNode(AST_BINARY_OPERATION, op);
    n->left = left; n->right = right;
    return n;
}

ASTNode* parseFactor(const char* source, int* position) {
    ASTNode* node = NULL;
    if (currentToken.type == TOKEN_NUMBER) {
        node = createNode(AST_NUMBER, currentToken.value); advance(source, position);
    } else if (currentToken.type == TOKEN_FLOAT_LITERAL) {
        node = createNode(AST_FLOAT, currentToken.value); advance(source, position);
    } else if (currentToken.type == TOKEN_STRING_LITERAL) {
        node = createNode(AST_STRING, currentToken.value); advance(source, position);
    } else if (currentToken.type == TOKEN_CHAR_LITERAL) {
        node = createNode(AST_CHAR, currentToken.value); advance(source, position);
    } else if (currentToken.type == TOKEN_IDENTIFIER) {
        node = createNode(AST_IDENTIFIER, currentToken.value); advance(source, position);
    } else if (currentToken.type == TOKEN_LPAREN) {
        advance(source, position); node = parseExpression(source, position);
        expect(TOKEN_RPAREN, source, position);
    } else if (currentToken.type == TOKEN_TRUE || currentToken.type == TOKEN_FALSE) {
        node = createNode(AST_NUMBER, strcmp(currentToken.value, "true") == 0 ? "1" : "0");
        advance(source, position);
    } else {
        parserError("Expected expression value.");
    }
    return node;
}

ASTNode* parseTerm(const char* source, int* position) {
    ASTNode* left = parseFactor(source, position);
    while (currentToken.type == TOKEN_MULTIPLY || currentToken.type == TOKEN_DIVIDE || currentToken.type == TOKEN_MODULO) {
        char op[3] = { currentToken.value[0], currentToken.value[1], '\0' };
        advance(source, position);
        ASTNode* right = parseFactor(source, position);
        left = binaryNode(left, op, right);
    }
    return left;
}

ASTNode* parseAdditive(const char* source, int* position) {
    ASTNode* left = parseTerm(source, position);
    while (currentToken.type == TOKEN_PLUS || currentToken.type == TOKEN_MINUS) {
        char op[3] = { currentToken.value[0], currentToken.value[1], '\0' };
        advance(source, position);
        ASTNode* right = parseTerm(source, position);
        left = binaryNode(left, op, right);
    }
    return left;
}

ASTNode* parseComparison(const char* source, int* position) {
    ASTNode* left = parseAdditive(source, position);
    while (currentToken.type == TOKEN_EQUAL || currentToken.type == TOKEN_NOT_EQUAL ||
           currentToken.type == TOKEN_LESS || currentToken.type == TOKEN_GREATER ||
           currentToken.type == TOKEN_LESS_EQUAL || currentToken.type == TOKEN_GREATER_EQUAL) {
        char op[4] = { currentToken.value[0], currentToken.value[1], currentToken.value[2], '\0' };
        advance(source, position);
        ASTNode* right = parseAdditive(source, position);
        left = binaryNode(left, op, right);
    }
    return left;
}

ASTNode* parseLogicalAnd(const char* source, int* position) {
    ASTNode* left = parseComparison(source, position);
    while (currentToken.type == TOKEN_LOGICAL_AND) {
        advance(source, position); left = binaryNode(left, "&&", parseComparison(source, position));
    }
    return left;
}

ASTNode* parseLogicalOr(const char* source, int* position) {
    ASTNode* left = parseLogicalAnd(source, position);
    while (currentToken.type == TOKEN_LOGICAL_OR) {
        advance(source, position); left = binaryNode(left, "||", parseLogicalAnd(source, position));
    }
    return left;
}

ASTNode* parseExpression(const char* source, int* position) { return parseLogicalOr(source, position); }

ASTNode* parseDeclaration(const char* source, int* position) {
    TokenType type = currentToken.type;
    char typeName[64];
    strncpy(typeName, currentToken.value, sizeof(typeName)-1);
    typeName[sizeof(typeName)-1] = '\0';
    int line = currentToken.line;

    advance(source, position);

    if (currentToken.type != TOKEN_IDENTIFIER) {
        reportError(ERROR_MISSING_IDENTIFIER, currentToken.line,
                    "Expected identifier after type.", currentToken.value, "identifier");
        return NULL;
    }

    char name[256];
    snprintf(name, sizeof(name), "%s", currentToken.value);
    advance(source, position);

    /*
     * Minimal C/C++ function definition support:
     *   int main() { ... }
     *   int foo() { ... }
     * Parameters are intentionally deferred to the function/parameter
     * milestone; an empty parameter list is supported here.
     */
    if (currentToken.type == TOKEN_LPAREN) {
        advance(source, position);

        if (currentToken.type != TOKEN_RPAREN) {
            reportError(ERROR_INVALID_STATEMENT, currentToken.line,
                        "Only empty parameter lists are supported in the current C frontend milestone.",
                        currentToken.value, ")");
            return NULL;
        }
        advance(source, position);

        if (currentToken.type != TOKEN_LBRACE) {
            reportError(ERROR_INVALID_STATEMENT, currentToken.line,
                        "Expected function body after parameter list.",
                        currentToken.value, "{");
            return NULL;
        }

        ASTNode* fn = createTypedNode(AST_FUNCTION, name, typeName, line);
        fn->body = parseBlock(source, position);
        return fn;
    }

    ASTNode* declaration = createTypedNode(AST_DECLARATION, name, typeName, line);

    if (currentToken.type == TOKEN_ASSIGN) {
        advance(source, position);
        declaration->initializer = parseExpression(source, position);
        declaration->left = declaration->initializer;
    }

    expect(TOKEN_SEMICOLON, source, position);
    (void)type;
    return declaration;
}

ASTNode* parseAssignment(const char* source, int* position) {
    ASTNode* assignment = createTypedNode(AST_ASSIGNMENT, currentToken.value, NULL, currentToken.line);
    advance(source, position);
    expect(TOKEN_ASSIGN, source, position);
    assignment->left = parseExpression(source, position);
    expect(TOKEN_SEMICOLON, source, position);
    return assignment;
}

static void appendChild(ASTNode* parent, ASTNode* child) {
    if (!child) return;
    if (!parent->left) { parent->left = child; return; }
    ASTNode* p = parent->left;
    while (p->next) p = p->next;
    p->next = child;
}

ASTNode* parseBlock(const char* source, int* position) {
    ASTNode* block = createNode(AST_BLOCK, NULL);
    expect(TOKEN_LBRACE, source, position);
    while (currentToken.type != TOKEN_RBRACE && currentToken.type != TOKEN_EOF && getLastError() == ERROR_NONE) {
        appendChild(block, parseStatement(source, position));
    }
    expect(TOKEN_RBRACE, source, position);
    return block;
}

ASTNode* parseIf(const char* source, int* position) {
    int line = currentToken.line;
    ASTNode* node = createTypedNode(AST_IF, "if", NULL, line);

    advance(source, position);
    expect(TOKEN_LPAREN, source, position);
    node->condition = parseExpression(source, position);
    node->left = node->condition; /* compatibility */
    expect(TOKEN_RPAREN, source, position);

    node->thenBranch = parseStatement(source, position);

    if (currentToken.type == TOKEN_ELSE) {
        advance(source, position);
        node->elseBranch = parseStatement(source, position);
    }

    return node;
}

ASTNode* parseWhile(const char* source, int* position) {
    int line = currentToken.line;
    ASTNode* node = createTypedNode(AST_WHILE, "while", NULL, line);

    advance(source, position);
    expect(TOKEN_LPAREN, source, position);
    node->condition = parseExpression(source, position);
    node->left = node->condition; /* compatibility */
    expect(TOKEN_RPAREN, source, position);

    node->body = parseStatement(source, position);
    return node;
}

ASTNode* parseReturn(const char* source, int* position) {
    ASTNode* node = createTypedNode(AST_RETURN, "return", NULL, currentToken.line);
    advance(source, position);
    if (currentToken.type != TOKEN_SEMICOLON) node->left = parseExpression(source, position);
    expect(TOKEN_SEMICOLON, source, position);
    return node;
}

ASTNode* parseStatement(const char* source, int* position) {
    switch (currentToken.type) {
        case TOKEN_LBRACE: return parseBlock(source, position);
        case TOKEN_IF: return parseIf(source, position);
        case TOKEN_WHILE: return parseWhile(source, position);
        case TOKEN_RETURN: return parseReturn(source, position);
        case TOKEN_INT: case TOKEN_FLOAT: case TOKEN_DOUBLE: case TOKEN_CHAR:
        case TOKEN_BOOL: case TOKEN_STRING_KW: case TOKEN_VOID:
            return parseDeclaration(source, position);
        case TOKEN_IDENTIFIER: return parseAssignment(source, position);
        default:
            parserError("Invalid statement.");
            advance(source, position);
            return NULL;
    }
}

ASTNode* parseProgram(const char* source, int* position) {
    ASTNode* program = createNode(AST_PROGRAM, NULL);
    ASTNode* last = NULL;
    resetLexer();
    advance(source, position);
    while (currentToken.type != TOKEN_EOF && getLastError() == ERROR_NONE) {
        ASTNode* statement = parseStatement(source, position);
        if (!statement) continue;
        if (!program->left) program->left = statement;
        else last->next = statement;
        last = statement;
    }
    return program;
}
