#ifndef JAVA_FRONTEND_H
#define JAVA_FRONTEND_H

#include "ast.h"

int javaFrontendParse(const char* source, ASTNode** root);

#endif
