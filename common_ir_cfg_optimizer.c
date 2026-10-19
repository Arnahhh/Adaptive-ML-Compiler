#include <stdlib.h>
#include <string.h>
#include "common_ir_cfg_optimizer.h"
#include "cfg.h"

static char* dupstr(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static int isInteger(const char* s) {
    if (!s || !*s) return 0;
    if (*s == '+' || *s == '-') s++;
    if (!*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

static void freeInstruction(CommonIRInstruction* x) {
    free(x->result);
    free(x->arg1);
    free(x->arg2);
    free(x->extra);
    x->result = x->arg1 = x->arg2 = x->extra = NULL;
}

static int replaceWithJump(CommonIRInstruction* x) {
    char* target = x->result ? dupstr(x->result) : NULL;
    if (x->result && !target) return 0;
    free(x->arg1);
    free(x->arg2);
    free(x->extra);
    x->arg1 = x->arg2 = x->extra = NULL;
    x->op = CIR_JUMP;
    /* result is already the target. Keep the original allocation. */
    free(target);
    return 1;
}

/*
 * If a branch condition is an integer constant, its outcome is known:
 *   BRANCH L, 0, false  -> JUMP L
 *   BRANCH L, nonzero, false -> NOP
 * This exposes unreachable blocks to the CFG pass below.
 */
static int simplifyConstantBranches(CommonIR* ir) {
    int changes = 0;
    for (size_t i = 0; i < ir->count; ++i) {
        CommonIRInstruction* x = &ir->instructions[i];
        if (x->op != CIR_BRANCH || !isInteger(x->arg1)) continue;
        long long v = strtoll(x->arg1, NULL, 10);
        if (v == 0) {
            if (!replaceWithJump(x)) return changes;
        } else {
            freeInstruction(x);
            x->op = CIR_NOP;
        }
        changes++;
    }
    return changes;
}

static int markReachable(const ControlFlowGraph* cfg, unsigned char* seen) {
    if (!cfg || !seen || cfg->blockCount == 0) return 1;
    size_t* stack = (size_t*)malloc(cfg->blockCount * sizeof(size_t));
    if (!stack) return 0;
    size_t top = 0;
    stack[top++] = 0;
    seen[0] = 1;

    while (top) {
        size_t b = stack[--top];
        for (size_t e = 0; e < cfg->edgeCount; ++e) {
            if (cfg->edges[e].from != b) continue;
            size_t to = cfg->edges[e].to;
            if (!seen[to]) {
                seen[to] = 1;
                stack[top++] = to;
            }
        }
    }
    free(stack);
    return 1;
}

static int removeUnreachableBlocks(CommonIR* ir, const ControlFlowGraph* cfg,
                                   const unsigned char* reachable) {
    if (!ir || !cfg || !reachable) return 0;

    size_t keepCount = 0;
    size_t removedBlocks = 0;
    for (size_t b = 0; b < cfg->blockCount; ++b) {
        if (reachable[b]) keepCount += cfg->blocks[b].end - cfg->blocks[b].start;
        else removedBlocks++;
    }
    if (removedBlocks == 0) return 0;

    CommonIRInstruction* kept = (CommonIRInstruction*)malloc(
        keepCount * sizeof(CommonIRInstruction));
    if (!kept) return 0;

    size_t write = 0;
    for (size_t b = 0; b < cfg->blockCount; ++b) {
        if (!reachable[b]) continue;
        for (size_t i = cfg->blocks[b].start; i < cfg->blocks[b].end; ++i)
            kept[write++] = ir->instructions[i];
    }

    /* Only unreachable instructions still own allocations in the old array. */
    for (size_t b = 0; b < cfg->blockCount; ++b) {
        if (reachable[b]) continue;
        for (size_t i = cfg->blocks[b].start; i < cfg->blocks[b].end; ++i)
            freeInstruction(&ir->instructions[i]);
    }

    free(ir->instructions);
    ir->instructions = kept;
    ir->count = keepCount;
    ir->capacity = keepCount;
    return (int)removedBlocks;
}

int commonIRCFGOptimize(CommonIR* ir) {
    if (!ir || ir->count == 0) return 0;
    int changes = simplifyConstantBranches(ir);

    ControlFlowGraph cfg;
    if (!cfgBuild(ir, &cfg)) return changes;

    unsigned char* reachable = (unsigned char*)calloc(cfg.blockCount, 1);
    if (!reachable) {
        cfgFree(&cfg);
        return changes;
    }

    if (markReachable(&cfg, reachable))
        changes += removeUnreachableBlocks(ir, &cfg, reachable);

    free(reachable);
    cfgFree(&cfg);
    return changes;
}
