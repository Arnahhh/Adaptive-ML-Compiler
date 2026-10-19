#ifndef FRONTEND_H
#define FRONTEND_H

#include "language.h"
#include "ast.h"

/*
 * A frontend is responsible for turning one source language into
 * the compiler's common AST representation.
 *
 * Only the C/C++ frontend is connected to the existing parser today.
 * Python and Java frontends are deliberately explicit stubs so that
 * unsupported languages never get misinterpreted as C.
 */
typedef enum {
    FRONTEND_SUCCESS = 0,
    FRONTEND_UNSUPPORTED,
    FRONTEND_ERROR
} FrontendStatus;

const char* frontendStatusToString(FrontendStatus status);

int frontendSupports(Language language);

/*
 * Parse source using the frontend selected by language.
 * position is reset/managed by the selected frontend.
 */
FrontendStatus frontendParse(
    Language language,
    const char* source,
    int* position,
    ASTNode** root
);

#endif
