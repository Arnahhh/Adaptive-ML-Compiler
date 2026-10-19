#ifndef CFG_H
#define CFG_H

#include "common_ir.h"
#include <stddef.h>

typedef struct {
    size_t start;       /* inclusive instruction index */
    size_t end;         /* exclusive instruction index */
} BasicBlock;

typedef struct {
    size_t from;
    size_t to;
} CFGEdge;

typedef struct {
    BasicBlock* blocks;
    size_t blockCount;
    size_t blockCapacity;

    CFGEdge* edges;
    size_t edgeCount;
    size_t edgeCapacity;
} ControlFlowGraph;

void cfgInit(ControlFlowGraph* cfg);
void cfgFree(ControlFlowGraph* cfg);

/* Build basic blocks and successor edges from Common IR. */
int cfgBuild(const CommonIR* ir, ControlFlowGraph* cfg);

/* Human-readable CFG dump for compiler diagnostics/debugging. */
void cfgPrint(const CommonIR* ir, const ControlFlowGraph* cfg);

#endif
