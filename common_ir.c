#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_ir.h"

static char* copyString(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* out = (char*)malloc(n);
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

static int ensureCapacity(CommonIR* ir) {
    if (ir->count < ir->capacity) return 1;

    size_t newCapacity = ir->capacity ? ir->capacity * 2 : 16;
    CommonIRInstruction* p = (CommonIRInstruction*)realloc(
        ir->instructions, newCapacity * sizeof(CommonIRInstruction)
    );
    if (!p) return 0;

    ir->instructions = p;
    ir->capacity = newCapacity;
    return 1;
}

void commonIRInit(CommonIR* ir) {
    if (!ir) return;
    ir->instructions = NULL;
    ir->count = 0;
    ir->capacity = 0;
    ir->nextTemp = 0;
    ir->nextLabel = 0;
}

void commonIRFree(CommonIR* ir) {
    if (!ir) return;

    for (size_t i = 0; i < ir->count; ++i) {
        free(ir->instructions[i].result);
        free(ir->instructions[i].arg1);
        free(ir->instructions[i].arg2);
        free(ir->instructions[i].extra);
    }

    free(ir->instructions);
    ir->instructions = NULL;
    ir->count = 0;
    ir->capacity = 0;
}

int commonIRAppend(
    CommonIR* ir,
    CommonIROp op,
    const char* result,
    const char* arg1,
    const char* arg2,
    const char* extra
) {
    if (!ir || !ensureCapacity(ir)) return 0;

    CommonIRInstruction* ins = &ir->instructions[ir->count];
    ins->op = op;
    ins->result = copyString(result);
    ins->arg1 = copyString(arg1);
    ins->arg2 = copyString(arg2);
    ins->extra = copyString(extra);

    if ((result && !ins->result) || (arg1 && !ins->arg1) ||
        (arg2 && !ins->arg2) || (extra && !ins->extra)) {
        free(ins->result);
        free(ins->arg1);
        free(ins->arg2);
        free(ins->extra);
        return 0;
    }

    ir->count++;
    return 1;
}

char* commonIRNewTemp(CommonIR* ir) {
    if (!ir) return NULL;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "t%u", ++ir->nextTemp);
    return copyString(buffer);
}

char* commonIRNewLabel(CommonIR* ir) {
    if (!ir) return NULL;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "L%u", ++ir->nextLabel);
    return copyString(buffer);
}

const char* commonIROpToString(CommonIROp op) {
    switch (op) {
        case CIR_NOP: return "NOP";
        case CIR_ASSIGN: return "ASSIGN";
        case CIR_BINARY: return "BINARY";
        case CIR_UNARY: return "UNARY";
        case CIR_LABEL: return "LABEL";
        case CIR_JUMP: return "JUMP";
        case CIR_BRANCH: return "BRANCH";
        case CIR_CALL: return "CALL";
        case CIR_RETURN: return "RETURN";
        case CIR_DECLARE: return "DECLARE";
        default: return "UNKNOWN";
    }
}

static void printOperand(const char* s) {
    if (s) printf("%s", s);
}

void commonIRPrint(const CommonIR* ir) {
    if (!ir) return;

    printf("Common IR (%zu instruction(s)):\n", ir->count);

    for (size_t i = 0; i < ir->count; ++i) {
        const CommonIRInstruction* x = &ir->instructions[i];

        printf("  %zu: %-7s ", i, commonIROpToString(x->op));
        printOperand(x->result);

        if (x->arg1) {
            printf(" <- ");
            printOperand(x->arg1);
        }
        if (x->arg2) {
            printf(", ");
            printOperand(x->arg2);
        }
        if (x->extra) {
            printf(" [%s]", x->extra);
        }
        printf("\n");
    }
}
