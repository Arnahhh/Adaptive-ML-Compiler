#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataflow.h"

static char* dupstr(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static int isVariable(const char* s) {
    if (!s || !*s) return 0;
    if (!(s[0] == '_' || (s[0] >= 'A' && s[0] <= 'Z') ||
          (s[0] >= 'a' && s[0] <= 'z'))) return 0;
    for (const char* p = s + 1; *p; ++p) {
        if (!(*p == '_' || (*p >= 'A' && *p <= 'Z') ||
              (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')))
            return 0;
    }
    /* Integer literals are not identifiers; common IR labels are handled by op. */
    return 1;
}

static int isDefinition(const CommonIRInstruction* x) {
    if (!x || !x->result || !isVariable(x->result)) return 0;
    return x->op == CIR_DECLARE || x->op == CIR_ASSIGN ||
           x->op == CIR_BINARY || x->op == CIR_UNARY || x->op == CIR_CALL;
}

static int findVariable(const DataFlowAnalysis* df, const char* name) {
    if (!df || !name) return -1;
    for (size_t i = 0; i < df->variableCount; ++i)
        if (strcmp(df->variables[i], name) == 0) return (int)i;
    return -1;
}

static int addVariable(DataFlowAnalysis* df, const char* name) {
    if (!isVariable(name)) return 1;
    if (findVariable(df, name) >= 0) return 1;
    char** p = (char**)realloc(df->variables,
                               (df->variableCount + 1) * sizeof(char*));
    if (!p) return 0;
    df->variables = p;
    df->variables[df->variableCount] = dupstr(name);
    if (!df->variables[df->variableCount]) return 0;
    df->variableCount++;
    return 1;
}

static int rowEqual(const unsigned char* a, const unsigned char* b, size_t n) {
    return memcmp(a, b, n) == 0;
}

static int collectVariablesAndDefinitions(const CommonIR* ir,
                                          DataFlowAnalysis* df,
                                          size_t* defCountOut) {
    size_t defs = 0;
    for (size_t i = 0; i < ir->count; ++i) {
        const CommonIRInstruction* x = &ir->instructions[i];
        if (isDefinition(x)) {
            if (!addVariable(df, x->result)) return 0;
            defs++;
        }
        if ((x->op == CIR_ASSIGN || x->op == CIR_BINARY ||
             x->op == CIR_UNARY || x->op == CIR_BRANCH ||
             x->op == CIR_RETURN || x->op == CIR_CALL)) {
            if (!addVariable(df, x->arg1)) return 0;
            if (!addVariable(df, x->arg2)) return 0;
        }
    }
    *defCountOut = defs;
    return 1;
}

static int definitionIndex(const CommonIR* ir, size_t instructionIndex) {
    int d = 0;
    for (size_t i = 0; i < instructionIndex; ++i)
        if (isDefinition(&ir->instructions[i])) d++;
    return d;
}

static int definitionVariable(const CommonIR* ir, size_t definitionNumber) {
    size_t d = 0;
    for (size_t i = 0; i < ir->count; ++i) {
        if (!isDefinition(&ir->instructions[i])) continue;
        if (d == definitionNumber) return i;
        d++;
    }
    return -1;
}

static void computeReaching(const CommonIR* ir, const ControlFlowGraph* cfg,
                            DataFlowAnalysis* df) {
    size_t B = cfg->blockCount, D = df->definitionCount;
    unsigned char* gen = (unsigned char*)calloc(B * D, 1);
    unsigned char* kill = (unsigned char*)calloc(B * D, 1);
    if (!gen || !kill) { free(gen); free(kill); return; }

    /* GEN contains the last definition of each variable in the block. */
    for (size_t b = 0; b < B; ++b) {
        for (size_t i = cfg->blocks[b].start; i < cfg->blocks[b].end; ++i) {
            const CommonIRInstruction* x = &ir->instructions[i];
            if (!isDefinition(x)) continue;
            size_t d = (size_t)definitionIndex(ir, i);
            int var = findVariable(df, x->result);
            if (var < 0) continue;
            for (size_t other = 0; other < D; ++other) {
                int oi = definitionVariable(ir, other);
                if (oi < 0) continue;
                const CommonIRInstruction* od = &ir->instructions[(size_t)oi];
                if (strcmp(od->result, x->result) == 0) {
                    if (other != d) gen[b * D + other] = 0;
                }
            }
            gen[b * D + d] = 1;
        }
    }

    /* KILL contains definitions of variables redefined anywhere in the block. */
    for (size_t b = 0; b < B; ++b) {
        for (size_t i = cfg->blocks[b].start; i < cfg->blocks[b].end; ++i) {
            const CommonIRInstruction* x = &ir->instructions[i];
            if (!isDefinition(x)) continue;
            for (size_t d = 0; d < D; ++d) {
                int di = definitionVariable(ir, d);
                if (di >= 0 && strcmp(ir->instructions[(size_t)di].result, x->result) == 0)
                    kill[b * D + d] = 1;
            }
        }
        /* A block's own generated definitions must not kill its GEN set. */
        for (size_t d = 0; d < D; ++d)
            if (gen[b * D + d]) kill[b * D + d] = 0;
    }

    size_t iter = 0;
    int changed = 1;
    while (changed && iter < B * D + 16) {
        changed = 0;
        ++iter;
        for (size_t b = 0; b < B; ++b) {
            unsigned char* in = df->reachingIn + b * D;
            unsigned char* out = df->reachingOut + b * D;
            unsigned char* oldIn = (unsigned char*)calloc(D ? D : 1, 1);
            unsigned char* oldOut = (unsigned char*)calloc(D ? D : 1, 1);
            if (!oldIn || !oldOut) { free(oldIn); free(oldOut); break; }
            memcpy(oldIn, in, D);
            memcpy(oldOut, out, D);

            memset(in, 0, D);
            for (size_t e = 0; e < cfg->edgeCount; ++e) {
                if (cfg->edges[e].to != b) continue;
                size_t p = cfg->edges[e].from;
                for (size_t d = 0; d < D; ++d)
                    if (df->reachingOut[p * D + d]) in[d] = 1;
            }
            for (size_t d = 0; d < D; ++d)
                out[d] = gen[b * D + d] || (in[d] && !kill[b * D + d]);

            if (!rowEqual(oldIn, in, D) || !rowEqual(oldOut, out, D)) changed = 1;
            free(oldIn); free(oldOut);
        }
    }
    df->reachingIterations = iter;
    free(gen); free(kill);
}

static void markUseBeforeDef(const CommonIR* ir, const ControlFlowGraph* cfg,
                             DataFlowAnalysis* df) {
    size_t B = cfg->blockCount, V = df->variableCount;
    unsigned char* def = (unsigned char*)calloc(B * V, 1);
    if (!def) return;

    for (size_t b = 0; b < B; ++b) {
        unsigned char* use = df->liveIn + b * V;
        for (size_t i = cfg->blocks[b].start; i < cfg->blocks[b].end; ++i) {
            const CommonIRInstruction* x = &ir->instructions[i];
            const char* uses[2] = { x->arg1, x->arg2 };
            for (int u = 0; u < 2; ++u) {
                int v = findVariable(df, uses[u]);
                if (v >= 0 && !def[b * V + (size_t)v]) use[v] = 1;
            }
            if (isDefinition(x)) {
                int v = findVariable(df, x->result);
                if (v >= 0) def[b * V + (size_t)v] = 1;
            }
        }
    }

    size_t iter = 0;
    int changed = 1;
    while (changed && iter < B * V + 16) {
        changed = 0;
        ++iter;
        for (size_t rev = B; rev > 0; --rev) {
            size_t b = rev - 1;
            unsigned char* in = df->liveIn + b * V;
            unsigned char* out = df->liveOut + b * V;
            unsigned char* oldIn = (unsigned char*)calloc(V ? V : 1, 1);
            unsigned char* oldOut = (unsigned char*)calloc(V ? V : 1, 1);
            unsigned char* newIn = (unsigned char*)calloc(V ? V : 1, 1);
            if (!oldIn || !oldOut || !newIn) {
                free(oldIn); free(oldOut); free(newIn); break;
            }
            memcpy(oldIn, in, V); memcpy(oldOut, out, V);

            memset(out, 0, V);
            for (size_t e = 0; e < cfg->edgeCount; ++e) {
                if (cfg->edges[e].from != b) continue;
                size_t s = cfg->edges[e].to;
                for (size_t v = 0; v < V; ++v)
                    if (df->liveIn[s * V + v]) out[v] = 1;
            }
            for (size_t v = 0; v < V; ++v)
                newIn[v] = in[v];
            for (size_t v = 0; v < V; ++v)
                newIn[v] = (in[v] ? 1 : 0);
            /* Reconstruct IN = USE ∪ (OUT - DEF). USE is stored in the initial
               IN row and must be preserved separately; rebuild it from the block. */
            memset(newIn, 0, V);
            for (size_t i = cfg->blocks[b].start; i < cfg->blocks[b].end; ++i) {
                const CommonIRInstruction* x = &ir->instructions[i];
                const char* uses[2] = { x->arg1, x->arg2 };
                for (int u = 0; u < 2; ++u) {
                    int v = findVariable(df, uses[u]);
                    if (v >= 0) {
                        /* Determine whether this use occurs before any definition. */
                        int defined = 0;
                        for (size_t j = cfg->blocks[b].start; j < i; ++j) {
                            const CommonIRInstruction* prior = &ir->instructions[j];
                            if (isDefinition(prior)) {
                                int pv = findVariable(df, prior->result);
                                if (pv == v) { defined = 1; break; }
                            }
                        }
                        if (!defined) newIn[v] = 1;
                    }
                }
            }
            for (size_t v = 0; v < V; ++v)
                if (!def[b * V + v] && out[v]) newIn[v] = 1;
            memcpy(in, newIn, V);

            if (!rowEqual(oldIn, in, V) || !rowEqual(oldOut, out, V)) changed = 1;
            free(oldIn); free(oldOut); free(newIn);
        }
    }
    df->livenessIterations = iter;
    free(def);
}

void dataFlowInit(DataFlowAnalysis* df) {
    if (!df) return;
    memset(df, 0, sizeof(*df));
}

void dataFlowFree(DataFlowAnalysis* df) {
    if (!df) return;
    for (size_t i = 0; i < df->variableCount; ++i) free(df->variables[i]);
    free(df->variables);
    free(df->reachingIn); free(df->reachingOut);
    free(df->liveIn); free(df->liveOut);
    dataFlowInit(df);
}

int dataFlowAnalyze(const CommonIR* ir, const ControlFlowGraph* cfg,
                    DataFlowAnalysis* df) {
    if (!ir || !cfg || !df) return 0;
    dataFlowFree(df);
    if (!cfg->blockCount) return 1;

    size_t defs = 0;
    if (!collectVariablesAndDefinitions(ir, df, &defs)) {
        dataFlowFree(df); return 0;
    }
    df->blockCount = cfg->blockCount;
    df->definitionCount = defs;

    size_t B = df->blockCount, D = df->definitionCount, V = df->variableCount;
    df->reachingIn = (unsigned char*)calloc(B * (D ? D : 1), 1);
    df->reachingOut = (unsigned char*)calloc(B * (D ? D : 1), 1);
    df->liveIn = (unsigned char*)calloc(B * (V ? V : 1), 1);
    df->liveOut = (unsigned char*)calloc(B * (V ? V : 1), 1);
    if (!df->reachingIn || !df->reachingOut || !df->liveIn || !df->liveOut) {
        dataFlowFree(df); return 0;
    }

    computeReaching(ir, cfg, df);
    markUseBeforeDef(ir, cfg, df);
    return 1;
}

static void printDefs(const CommonIR* ir, const DataFlowAnalysis* df,
                      const unsigned char* row) {
    int first = 1;
    printf("{");
    for (size_t d = 0; d < df->definitionCount; ++d) {
        if (!row[d]) continue;
        int idx = definitionVariable(ir, d);
        if (idx < 0) continue;
        if (!first) printf(", ");
        printf("d%zu:%s", d, ir->instructions[(size_t)idx].result);
        first = 0;
    }
    printf("}");
}

static void printVars(const DataFlowAnalysis* df, const unsigned char* row) {
    int first = 1;
    printf("{");
    for (size_t v = 0; v < df->variableCount; ++v) {
        if (!row[v]) continue;
        if (!first) printf(", ");
        printf("%s", df->variables[v]);
        first = 0;
    }
    printf("}");
}

void dataFlowPrint(const CommonIR* ir, const ControlFlowGraph* cfg,
                   const DataFlowAnalysis* df) {
    if (!ir || !cfg || !df) return;
    printf("Data-flow analysis: %zu variable(s), %zu definition(s)\n",
           df->variableCount, df->definitionCount);
    printf("  Reaching definitions iterations: %zu\n", df->reachingIterations);
    for (size_t b = 0; b < cfg->blockCount; ++b) {
        printf("  B%zu RD-IN = ", b); printDefs(ir, df, df->reachingIn + b * df->definitionCount);
        printf("\n  B%zu RD-OUT = ", b); printDefs(ir, df, df->reachingOut + b * df->definitionCount);
        printf("\n");
    }
    printf("  Liveness iterations: %zu\n", df->livenessIterations);
    for (size_t b = 0; b < cfg->blockCount; ++b) {
        printf("  B%zu LIVE-IN = ", b); printVars(df, df->liveIn + b * df->variableCount);
        printf("\n  B%zu LIVE-OUT = ", b); printVars(df, df->liveOut + b * df->variableCount);
        printf("\n");
    }
}
