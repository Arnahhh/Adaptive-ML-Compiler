#include <stdio.h>
#include <string.h>

#include "error.h"


/*
 * Store information about the most recent error.
 */

static ErrorType lastError = ERROR_NONE;

static int lastErrorLine = 0;

static char lastErrorMessage[256];

static char lastErrorFound[100];

static char lastErrorExpected[100];

#define MAX_DIAGNOSTICS 256
static ErrorDiagnostic diagnostics[MAX_DIAGNOSTICS];
static int diagnosticCount = 0;


/*
 * Convert error type to readable text.
 */
const char* errorTypeToString(ErrorType type) {

    switch (type) {

        case ERROR_NONE:
            return "NO_ERROR";

        case ERROR_MISSING_SEMICOLON:
            return "MISSING_SEMICOLON";

        case ERROR_MISSING_IDENTIFIER:
            return "MISSING_IDENTIFIER";

        case ERROR_MISSING_EXPRESSION:
            return "MISSING_EXPRESSION";

        case ERROR_MISSING_RPAREN:
            return "MISSING_RIGHT_PARENTHESIS";

        case ERROR_INVALID_STATEMENT:
            return "INVALID_STATEMENT";

        default:
            return "UNKNOWN_ERROR";
    }
}


/*
 * Report an error.
 */
void reportError(
    ErrorType type,
    int line,
    const char* message,
    const char* found,
    const char* expected
) {

    /*
     * Store error information.
     */

    lastError = type;

    lastErrorLine = line;


    if (message != NULL) {

        strncpy(
            lastErrorMessage,
            message,
            sizeof(lastErrorMessage) - 1
        );

        lastErrorMessage[
            sizeof(lastErrorMessage) - 1
        ] = '\0';

    }

    else {

        lastErrorMessage[0] = '\0';
    }


    if (found != NULL) {

        strncpy(
            lastErrorFound,
            found,
            sizeof(lastErrorFound) - 1
        );

        lastErrorFound[
            sizeof(lastErrorFound) - 1
        ] = '\0';

    }

    else {

        lastErrorFound[0] = '\0';
    }


    if (expected != NULL) {

        strncpy(
            lastErrorExpected,
            expected,
            sizeof(lastErrorExpected) - 1
        );

        lastErrorExpected[
            sizeof(lastErrorExpected) - 1
        ] = '\0';

    }

    else {

        lastErrorExpected[0] = '\0';
    }


    if (diagnosticCount < MAX_DIAGNOSTICS) {
        ErrorDiagnostic* d = &diagnostics[diagnosticCount++];
        d->type = type;
        d->line = line;
        snprintf(d->message, sizeof(d->message), "%s", lastErrorMessage);
        snprintf(d->found, sizeof(d->found), "%s", lastErrorFound);
        snprintf(d->expected, sizeof(d->expected), "%s", lastErrorExpected);
    }

    /* Diagnostics are stored for the compiler driver. */
}

/*
 * Return the most recent error.
 */
ErrorType getLastError() {

    return lastError;
}


/*
 * Return the line of the most recent error.
 */
int getLastErrorLine() {

    return lastErrorLine;
}


/*
 * Return error message.
 */
const char* getLastErrorMessage() {

    return lastErrorMessage;
}


/*
 * Return token that was found.
 */
const char* getLastErrorFound() {

    return lastErrorFound;
}


/*
 * Return token that was expected.
 */
const char* getLastErrorExpected() {

    return lastErrorExpected;
}


/*
 * Reset error information.
 */
int getErrorCount(void) {
    return diagnosticCount;
}

const ErrorDiagnostic* getErrorDiagnostic(int index) {
    if (index < 0 || index >= diagnosticCount) return NULL;
    return &diagnostics[index];
}

void clearError() {
    diagnosticCount = 0;

    lastError = ERROR_NONE;

    lastErrorLine = 0;

    lastErrorMessage[0] = '\0';

    lastErrorFound[0] = '\0';

    lastErrorExpected[0] = '\0';
}