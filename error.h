#ifndef ERROR_H
#define ERROR_H

typedef enum {

    ERROR_NONE,

    ERROR_MISSING_SEMICOLON,
    ERROR_MISSING_IDENTIFIER,
    ERROR_MISSING_EXPRESSION,
    ERROR_MISSING_RPAREN,
    ERROR_INVALID_STATEMENT

} ErrorType;


/* Convert error type to readable text */
const char* errorTypeToString(ErrorType type);


/* Report an error */
void reportError(
    ErrorType type,
    int line,
    const char* message,
    const char* found,
    const char* expected
);


/* Get latest error information */
ErrorType getLastError();

int getLastErrorLine();

const char* getLastErrorMessage();

const char* getLastErrorFound();

const char* getLastErrorExpected();



typedef struct {
    ErrorType type;
    int line;
    char message[256];
    char found[100];
    char expected[100];
} ErrorDiagnostic;

int getErrorCount(void);
const ErrorDiagnostic* getErrorDiagnostic(int index);

/* Clear error information */
void clearError();

#endif