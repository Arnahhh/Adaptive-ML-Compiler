#ifndef DATAFLOW_H
#define DATAFLOW_H

#include <stddef.h>
#include "common_ir.h"
#include "cfg.h"

typedef struct {
    size_t blockCount;
    size_t definitionCount;
    size_t variableCount;
    char** variables;

    /* Reaching definitions: flattened [block][definition]. */
    unsigned char* reachingIn;
    unsigned char* reachingOut;

    /* Liveness: flattened [block][variable]. */
    unsigned char* liveIn;
    unsigned char* liveOut;

    size_t reachingIterations;
    size_t livenessIterations;
} DataFlowAnalysis;

void dataFlowInit(DataFlowAnalysis* df);
void dataFlowFree(DataFlowAnalysis* df);

/* Compute classic forward reaching-definitions and backward liveness. */
int dataFlowAnalyze(const CommonIR* ir, const ControlFlowGraph* cfg,
                    DataFlowAnalysis* df);

void dataFlowPrint(const CommonIR* ir, const ControlFlowGraph* cfg,
                   const DataFlowAnalysis* df);

#endif
