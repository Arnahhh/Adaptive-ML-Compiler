#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

void advance(const char* source, int* position);
void parserError(const char* message);
void expect(TokenType type, const char* source, int* position);

ASTNode* parseFactor(const char* source, int* position);
ASTNode* parseTerm(const char* source, int* position);
ASTNode* parseAdditive(const char* source, int* position);
ASTNode* parseComparison(const char* source, int* position);
ASTNode* parseLogicalAnd(const char* source, int* position);
ASTNode* parseLogicalOr(const char* source, int* position);
ASTNode* parseExpression(const char* source, int* position);

ASTNode* parseDeclaration(const char* source, int* position);
ASTNode* parseAssignment(const char* source, int* position);
ASTNode* parseBlock(const char* source, int* position);
ASTNode* parseIf(const char* source, int* position);
ASTNode* parseWhile(const char* source, int* position);
ASTNode* parseReturn(const char* source, int* position);
ASTNode* parseStatement(const char* source, int* position);
ASTNode* parseProgram(const char* source, int* position);

#endif
