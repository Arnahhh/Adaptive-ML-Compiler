#ifndef COMMON_IR_H
#define COMMON_IR_H

#include <stddef.h>

/*
 * Language-neutral Intermediate Representation.
 *
 * The frontend of C/C++/Python/Java should eventually lower into this
 * representation. It deliberately uses strings for operands so that the
 * representation is independent of source-language token enums.
 */

typedef enum {
    CIR_NOP,
    CIR_ASSIGN,
    CIR_BINARY,
    CIR_UNARY,
    CIR_LABEL,
    CIR_JUMP,
    CIR_BRANCH,
    CIR_CALL,
    CIR_RETURN,
    CIR_DECLARE
} CommonIROp;

typedef struct {
    CommonIROp op;
    char* result;
    char* arg1;
    char* arg2;
    char* extra;
} CommonIRInstruction;

typedef struct {
    CommonIRInstruction* instructions;
    size_t count;
    size_t capacity;
    unsigned nextTemp;
    unsigned nextLabel;
} CommonIR;

void commonIRInit(CommonIR* ir);
void commonIRFree(CommonIR* ir);

int commonIRAppend(
    CommonIR* ir,
    CommonIROp op,
    const char* result,
    const char* arg1,
    const char* arg2,
    const char* extra
);

char* commonIRNewTemp(CommonIR* ir);
char* commonIRNewLabel(CommonIR* ir);

const char* commonIROpToString(CommonIROp op);
void commonIRPrint(const CommonIR* ir);

#endif
