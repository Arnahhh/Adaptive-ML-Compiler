#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"

static int addBlock(ControlFlowGraph* cfg,size_t start,size_t end) {
    if (cfg->blockCount==cfg->blockCapacity) {
        size_t cap=cfg->blockCapacity?cfg->blockCapacity*2:8;
        BasicBlock* p=(BasicBlock*)realloc(cfg->blocks,cap*sizeof(BasicBlock));
        if (!p) return 0;
        cfg->blocks=p; cfg->blockCapacity=cap;
    }
    cfg->blocks[cfg->blockCount].start=start;
    cfg->blocks[cfg->blockCount].end=end;
    cfg->blockCount++;
    return 1;
}

static int addEdge(ControlFlowGraph* cfg,size_t from,size_t to) {
    if (cfg->edgeCount==cfg->edgeCapacity) {
        size_t cap=cfg->edgeCapacity?cfg->edgeCapacity*2:16;
        CFGEdge* p=(CFGEdge*)realloc(cfg->edges,cap*sizeof(CFGEdge));
        if (!p) return 0;
        cfg->edges=p; cfg->edgeCapacity=cap;
    }
    cfg->edges[cfg->edgeCount].from=from;
    cfg->edges[cfg->edgeCount].to=to;
    cfg->edgeCount++;
    return 1;
}

void cfgInit(ControlFlowGraph* cfg) {
    if (!cfg) return;
    cfg->blocks=NULL; cfg->blockCount=cfg->blockCapacity=0;
    cfg->edges=NULL; cfg->edgeCount=cfg->edgeCapacity=0;
}

void cfgFree(ControlFlowGraph* cfg) {
    if (!cfg) return;
    free(cfg->blocks); free(cfg->edges);
    cfgInit(cfg);
}

static int isTerminator(CommonIROp op) {
    return op==CIR_JUMP || op==CIR_BRANCH || op==CIR_RETURN;
}

static int findLabel(const CommonIR* ir,const char* label,size_t* index) {
    if (!ir || !label) return 0;
    for (size_t i=0;i<ir->count;i++) {
        if (ir->instructions[i].op==CIR_LABEL &&
            ir->instructions[i].result &&
            strcmp(ir->instructions[i].result,label)==0) {
            *index=i; return 1;
        }
    }
    return 0;
}

int cfgBuild(const CommonIR* ir, ControlFlowGraph* cfg) {
    if (!ir || !cfg) return 0;
    cfgInit(cfg);
    if (!ir->count) return 1;

    unsigned char* leader=(unsigned char*)calloc(ir->count,1);
    if (!leader) return 0;
    leader[0]=1;

    for (size_t i=0;i<ir->count;i++) {
        const CommonIRInstruction* x=&ir->instructions[i];

        if (x->op==CIR_LABEL) leader[i]=1;

        if (isTerminator(x->op) && i+1<ir->count)
            leader[i+1]=1;

        if ((x->op==CIR_JUMP || x->op==CIR_BRANCH) && x->result) {
            size_t target;
            if (findLabel(ir,x->result,&target)) leader[target]=1;
        }
    }

    size_t start=0;
    for (size_t i=1;i<ir->count;i++) {
        if (leader[i]) {
            if (!addBlock(cfg,start,i)) { free(leader); cfgFree(cfg); return 0; }
            start=i;
        }
    }
    if (!addBlock(cfg,start,ir->count)) { free(leader); cfgFree(cfg); return 0; }
    free(leader);

    /* Determine successors from the final instruction of each block. */
    for (size_t b=0;b<cfg->blockCount;b++) {
        size_t last=cfg->blocks[b].end-1;
        const CommonIRInstruction* x=&ir->instructions[last];

        if (x->op==CIR_JUMP) {
            size_t target;
            if (!findLabel(ir,x->result,&target)) continue;
            for (size_t t=0;t<cfg->blockCount;t++)
                if (cfg->blocks[t].start<=target && target<cfg->blocks[t].end)
                    if (!addEdge(cfg,b,t)) { cfgFree(cfg); return 0; }
        } else if (x->op==CIR_BRANCH) {
            size_t target;
            if (findLabel(ir,x->result,&target)) {
                for (size_t t=0;t<cfg->blockCount;t++)
                    if (cfg->blocks[t].start<=target && target<cfg->blocks[t].end)
                        if (!addEdge(cfg,b,t)) { cfgFree(cfg); return 0; }
            }
            if (b+1<cfg->blockCount && !addEdge(cfg,b,b+1)) {
                cfgFree(cfg); return 0;
            }
        } else if (x->op==CIR_RETURN) {
            /* no successor */
        } else if (b+1<cfg->blockCount) {
            if (!addEdge(cfg,b,b+1)) { cfgFree(cfg); return 0; }
        }
    }
    return 1;
}

void cfgPrint(const CommonIR* ir,const ControlFlowGraph* cfg) {
    if (!ir || !cfg) return;
    printf("Control Flow Graph: %zu block(s), %zu edge(s)\n",
           cfg->blockCount,cfg->edgeCount);
    for (size_t b=0;b<cfg->blockCount;b++) {
        printf("  B%zu: instructions %zu..%zu\n",
               b,cfg->blocks[b].start,cfg->blocks[b].end ? cfg->blocks[b].end-1 : 0);
    }
    for (size_t e=0;e<cfg->edgeCount;e++)
        printf("  EDGE: B%zu -> B%zu\n",cfg->edges[e].from,cfg->edges[e].to);
}
