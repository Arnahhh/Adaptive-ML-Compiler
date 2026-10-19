#ifndef COMMON_IR_LOWER_H
#define COMMON_IR_LOWER_H

#include "ast.h"
#include "common_ir.h"

int lowerASTToCommonIR(const ASTNode* root, CommonIR* ir);

#endif
