#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

/* Perform constant folding on the AST */
void constantFold(ASTNode* node);

/* Perform constant propagation */
void constantPropagate(ASTNode* node);

/* Perform dead code elimination */
void deadCodeElimination(ASTNode* node);
/* Perform common subexpression elimination */
void commonSubexpressionElimination(ASTNode* node);

/* Free memory used by the constant table */
void freeConstantTable(void);

#endif