#include <stdio.h>

#include "frontend.h"
#include "parser.h"
#include "error.h"
#include "python_frontend.h"
#include "java_frontend.h"

const char* frontendStatusToString(FrontendStatus status) {
    switch (status) {
        case FRONTEND_SUCCESS: return "success";
        case FRONTEND_UNSUPPORTED: return "unsupported";
        case FRONTEND_ERROR: return "error";
        default: return "unknown";
    }
}

int frontendSupports(Language language) {
    /* Frontend support is subset-based and varies by language. */
    return language == LANGUAGE_C || language == LANGUAGE_CPP;
}

FrontendStatus frontendParse(
    Language language,
    const char* source,
    int* position,
    ASTNode** root
) {
    if (!source || !position || !root) {
        return FRONTEND_ERROR;
    }

    *root = NULL;
    *position = 0;

    switch (language) {
        case LANGUAGE_C:
        case LANGUAGE_CPP:
            *root = parseProgram(source, position);
            return getLastError() == ERROR_NONE
                ? FRONTEND_SUCCESS
                : FRONTEND_ERROR;

        case LANGUAGE_PYTHON:
            if (pythonFrontendParse(source, root)) return FRONTEND_SUCCESS;
            reportError(ERROR_INVALID_STATEMENT, 0,
                        "Python frontend could not parse the supported subset.",
                        "", "valid Python statement");
            return FRONTEND_ERROR;

        case LANGUAGE_JAVA:
            if (javaFrontendParse(source, root)) return FRONTEND_SUCCESS;
            if (getLastError() == ERROR_NONE) {
                reportError(ERROR_INVALID_STATEMENT, 0,
                            "Java frontend could not parse the supported subset.",
                            "", "valid Java statement");
            }
            return FRONTEND_ERROR;

        case LANGUAGE_UNKNOWN:
        default:
            return FRONTEND_UNSUPPORTED;
    }
}
