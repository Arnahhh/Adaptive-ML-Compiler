#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_ir_codegen.h"

typedef struct { char* name; int offset; } Slot;

static char* dupstr(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s)+1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p,s,n);
    return p;
}
static int isNumber(const char* s) {
    if (!s || !*s) return 0;
    if (*s=='-' || *s=='+') s++;
    if (!*s) return 0;
    while (*s) { if (*s<'0'||*s>'9') return 0; s++; }
    return 1;
}
static int addSlot(Slot** a,size_t* n,size_t* cap,const char* name) {
    if (!name || !*name) return 1;
    for (size_t i=0;i<*n;i++) if (!strcmp((*a)[i].name,name)) return 1;
    if (*n==*cap) {
        size_t nc=*cap?*cap*2:32;
        Slot* p=(Slot*)realloc(*a,nc*sizeof(Slot));
        if (!p) return 0;
        *a=p; *cap=nc;
    }
    (*a)[*n].name=dupstr(name);
    if (!(*a)[*n].name) return 0;
    (*a)[*n].offset=-8*(int)(*n+1);
    (*n)++;
    return 1;
}
static int findSlot(Slot* a,size_t n,const char* name) {
    for (size_t i=0;i<n;i++) if (!strcmp(a[i].name,name)) return a[i].offset;
    return 0;
}
static void load(FILE* f,Slot* a,size_t n,const char* x,const char* reg) {
    if (!x) fprintf(f,"    movl $0, %s\n",reg);
    else if (isNumber(x)) fprintf(f,"    movl $%s, %s\n",x,reg);
    else {
        int o=findSlot(a,n,x);
        fprintf(f,"    movl %d(%%rbp), %s\n",o,reg);
    }
}
static void store(FILE* f,Slot* a,size_t n,const char* x,const char* reg) {
    int o=findSlot(a,n,x);
    if (o) fprintf(f,"    movl %s, %d(%%rbp)\n",reg,o);
}
static void binary(FILE* f,Slot* a,size_t n,const CommonIRInstruction* x) {
    load(f,a,n,x->arg1,"%eax");
    if (!strcmp(x->extra,"+")) {
        if (isNumber(x->arg2)) fprintf(f,"    addl $%s, %%eax\n",x->arg2);
        else fprintf(f,"    addl %d(%%rbp), %%eax\n",findSlot(a,n,x->arg2));
    } else if (!strcmp(x->extra,"-")) {
        if (isNumber(x->arg2)) fprintf(f,"    subl $%s, %%eax\n",x->arg2);
        else fprintf(f,"    subl %d(%%rbp), %%eax\n",findSlot(a,n,x->arg2));
    } else if (!strcmp(x->extra,"*")) {
        if (isNumber(x->arg2)) fprintf(f,"    imull $%s, %%eax\n",x->arg2);
        else fprintf(f,"    imull %d(%%rbp), %%eax\n",findSlot(a,n,x->arg2));
    } else if (!strcmp(x->extra,"/") || !strcmp(x->extra,"%")) {
        load(f,a,n,x->arg2,"%ecx");
        fprintf(f,"    cltd\n    idivl %%ecx\n");
        if (!strcmp(x->extra,"%")) fprintf(f,"    movl %%edx, %%eax\n");
    } else {
        load(f,a,n,x->arg2,"%ecx");
        const char* cc=NULL;
        if(!strcmp(x->extra,"<")) cc="l";
        else if(!strcmp(x->extra,">")) cc="g";
        else if(!strcmp(x->extra,"<=")) cc="le";
        else if(!strcmp(x->extra,">=")) cc="ge";
        else if(!strcmp(x->extra,"==")) cc="e";
        else if(!strcmp(x->extra,"!=")) cc="ne";
        if (cc) {
            fprintf(f,"    cmpl %%ecx, %%eax\n    set%s %%al\n    movzbl %%al, %%eax\n",cc);
        } else if (!strcmp(x->extra,"&&")) {
            fprintf(f,"    testl %%eax, %%eax\n    setne %%al\n    movzbl %%al, %%eax\n");
            fprintf(f,"    testl %%ecx, %%ecx\n    setne %%cl\n    andb %%cl, %%al\n    movzbl %%al, %%eax\n");
        } else if (!strcmp(x->extra,"||")) {
            fprintf(f,"    testl %%eax, %%eax\n    setne %%al\n    movzbl %%al, %%eax\n");
            fprintf(f,"    testl %%ecx, %%ecx\n    setne %%cl\n    orb %%cl, %%al\n    movzbl %%al, %%eax\n");
        }
    }
    store(f,a,n,x->result,"%eax");
}
int generateCommonIRCode(const CommonIR* ir,const char* filename) {
    if (!ir || !filename) return 0;
    FILE* f=fopen(filename,"w");
    if (!f) return 0;

    Slot* slots=NULL; size_t n=0,cap=0;
    for(size_t i=0;i<ir->count;i++) {
        const CommonIRInstruction* x=&ir->instructions[i];
        if(x->result && x->op!=CIR_LABEL && x->op!=CIR_JUMP && x->op!=CIR_BRANCH)
            if(!addSlot(&slots,&n,&cap,x->result)) { fclose(f); return 0; }
    }
    int frame=((int)n*8+15)/16*16;
    if(frame<16) frame=16;

    fprintf(f,".text\n.globl main\nmain:\n");
    fprintf(f,"    pushq %%rbp\n    movq %%rsp, %%rbp\n    subq $%d, %%rsp\n",frame);

    int returned=0;
    for(size_t i=0;i<ir->count;i++) {
        const CommonIRInstruction* x=&ir->instructions[i];
        switch(x->op) {
        case CIR_LABEL:
            /* The backend emits the entry symbol/prologue itself. A frontend
               function label named main must not create a duplicate symbol. */
            if (!x->result || strcmp(x->result, "main") != 0)
                fprintf(f,"\n%s:\n",x->result ? x->result : "L");
            break;
        case CIR_DECLARE: break;
        case CIR_ASSIGN: load(f,slots,n,x->arg1,"%eax"); store(f,slots,n,x->result,"%eax"); break;
        case CIR_BINARY: binary(f,slots,n,x); break;
        case CIR_UNARY:
            load(f,slots,n,x->arg1,"%eax");
            if(x->extra && !strcmp(x->extra,"-")) fprintf(f,"    negl %%eax\n");
            else if(x->extra && !strcmp(x->extra,"!"))
                fprintf(f,"    testl %%eax, %%eax\n    sete %%al\n    movzbl %%al, %%eax\n");
            store(f,slots,n,x->result,"%eax");
            break;
        case CIR_JUMP: fprintf(f,"    jmp %s\n",x->result); break;
        case CIR_BRANCH:
            load(f,slots,n,x->arg1,"%eax");
            fprintf(f,"    cmpl $0, %%eax\n    %s %s\n",
                    (x->extra && !strcmp(x->extra,"false"))?"je":"jne",x->result);
            break;
        case CIR_RETURN:
            load(f,slots,n,x->arg1,"%eax");
            fprintf(f,"    leave\n    ret\n");
            returned=1;
            break;
        case CIR_CALL:
            fprintf(f,"    # CALL %s -- function ABI lowering deferred\n",x->arg1?x->arg1:"");
            break;
        default: break;
        }
    }
    if(!returned) fprintf(f,"    movl $0, %%eax\n    leave\n    ret\n");
    fprintf(f,".section .note.GNU-stack,\"\",@progbits\n");

    fclose(f);
    for(size_t i=0;i<n;i++) free(slots[i].name);
    free(slots);
    return 1;
}
