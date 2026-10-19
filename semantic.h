#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol.h"

typedef struct {
    int errors;
    int warnings;
    SymbolTable symbols;
} SemanticResult;

void semanticResultInit(SemanticResult* result);
void semanticResultFree(SemanticResult* result);
int semanticAnalyze(ASTNode* root, SemanticResult* result);

#endif
