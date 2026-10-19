#ifndef PYTHON_FRONTEND_H
#define PYTHON_FRONTEND_H

#include "ast.h"

int pythonFrontendParse(const char* source, ASTNode** root);

#endif
