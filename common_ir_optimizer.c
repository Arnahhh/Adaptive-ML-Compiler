#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_ir_optimizer.h"

static char* dupstr(const char* s) { if (!s) return NULL; size_t n=strlen(s)+1; char* p=(char*)malloc(n); if (p) memcpy(p,s,n); return p; }

typedef struct {
    char* name;
    char* value;
} ConstEntry;

static void freeConst(ConstEntry* e) {
    if (!e) return;
    free(e->name); free(e->value);
    e->name=e->value=NULL;
}

static int isInteger(const char* s) {
    if (!s || !*s) return 0;
    const char* p=s;
    if (*p=='-' || *p=='+') p++;
    if (!*p) return 0;
    while (*p) {
        if (*p<'0' || *p>'9') return 0;
        p++;
    }
    return 1;
}

static int calc(long long a,long long b,const char* op,long long* out) {
    if (!op || !out) return 0;
    if (!strcmp(op,"+")) *out=a+b;
    else if (!strcmp(op,"-")) *out=a-b;
    else if (!strcmp(op,"*")) *out=a*b;
    else if (!strcmp(op,"/")) { if (b==0) return 0; *out=a/b; }
    else if (!strcmp(op,"%")) { if (b==0) return 0; *out=a%b; }
    else if (!strcmp(op,"<")) *out=(a<b);
    else if (!strcmp(op,">")) *out=(a>b);
    else if (!strcmp(op,"<=")) *out=(a<=b);
    else if (!strcmp(op,">=")) *out=(a>=b);
    else if (!strcmp(op,"==")) *out=(a==b);
    else if (!strcmp(op,"!=")) *out=(a!=b);
    else if (!strcmp(op,"&&")) *out=(a!=0 && b!=0);
    else if (!strcmp(op,"||")) *out=(a!=0 || b!=0);
    else return 0;
    return 1;
}

static void replaceString(char** dst,const char* src) {
    char* p=src ? dupstr(src) : NULL;
    if (src && !p) return;
    free(*dst); *dst=p;
}

static int findConst(ConstEntry* t,size_t n,const char* name) {
    for (size_t i=0;i<n;i++) if (!strcmp(t[i].name,name)) return (int)i;
    return -1;
}

static void killConst(ConstEntry* t,size_t* n,const char* name) {
    int i=findConst(t,*n,name);
    if (i<0) return;
    freeConst(&t[i]);
    for (size_t j=(size_t)i;j+1<*n;j++) t[j]=t[j+1];
    (*n)--;
}

static void setConst(ConstEntry* t,size_t* n,const char* name,const char* value) {
    if (!name || !value) return;
    int i=findConst(t,*n,name);
    if (i>=0) { replaceString(&t[i].value,value); return; }
    t[*n].name=dupstr(name);
    t[*n].value=dupstr(value);
    if (t[*n].name && t[*n].value) (*n)++;
    else { freeConst(&t[*n]); }
}

int commonIRConstantPropagation(CommonIR* ir) {
    if (!ir) return 0;
    ConstEntry* table=(ConstEntry*)calloc(ir->count+1,sizeof(ConstEntry));
    if (!table) return 0;
    size_t n=0; int changes=0;

    for (size_t i=0;i<ir->count;i++) {
        CommonIRInstruction* x=&ir->instructions[i];

        /* Control-flow boundaries conservatively invalidate values. */
        if (x->op==CIR_LABEL || x->op==CIR_JUMP || x->op==CIR_BRANCH) {
            for (size_t k=0;k<n;k++) freeConst(&table[k]);
            n=0;
            continue;
        }

        if (x->op==CIR_ASSIGN) {
            int k=x->arg1 ? findConst(table,n,x->arg1) : -1;
            if (k>=0) {
                replaceString(&x->arg1,table[k].value);
                changes++;
            }
            if (isInteger(x->arg1)) setConst(table,&n,x->result,x->arg1);
            else killConst(table,&n,x->result);
        } else if (x->op==CIR_BINARY) {
            int k=x->arg1 ? findConst(table,n,x->arg1) : -1;
            if (k>=0) { replaceString(&x->arg1,table[k].value); changes++; }
            k=x->arg2 ? findConst(table,n,x->arg2) : -1;
            if (k>=0) { replaceString(&x->arg2,table[k].value); changes++; }
            killConst(table,&n,x->result);
        } else if (x->op==CIR_UNARY) {
            int k=x->arg1 ? findConst(table,n,x->arg1) : -1;
            if (k>=0) { replaceString(&x->arg1,table[k].value); changes++; }
            killConst(table,&n,x->result);
        } else if (x->op==CIR_RETURN) {
            int k=x->arg1 ? findConst(table,n,x->arg1) : -1;
            if (k>=0) { replaceString(&x->arg1,table[k].value); changes++; }
        }
    }
    for (size_t k=0;k<n;k++) freeConst(&table[k]);
    free(table);
    return changes;
}

int commonIRConstantFold(CommonIR* ir) {
    if (!ir) return 0;
    int changes=0;
    for (size_t i=0;i<ir->count;i++) {
        CommonIRInstruction* x=&ir->instructions[i];
        if (x->op!=CIR_BINARY || !isInteger(x->arg1) || !isInteger(x->arg2)) continue;
        long long a=atoll(x->arg1), b=atoll(x->arg2), r;
        if (!calc(a,b,x->extra,&r)) continue;
        char buf[64]; snprintf(buf,sizeof(buf),"%d",(int)r);
        x->op=CIR_ASSIGN;
        free(x->arg1); x->arg1=dupstr(buf);
        free(x->arg2); x->arg2=NULL;
        free(x->extra); x->extra=NULL;
        changes++;
    }
    return changes;
}

int commonIRDeadCodeElimination(CommonIR* ir) {
    /*
     * Conservative DCE: remove only instructions that define a temporary
     * which is never subsequently used before the next control-flow boundary.
     */
    if (!ir) return 0;
    int changes=0;
    for (size_t i=0;i<ir->count;i++) {
        CommonIRInstruction* x=&ir->instructions[i];
        if (x->op!=CIR_BINARY && x->op!=CIR_UNARY) continue;
        if (!x->result || x->result[0]!='t') continue;

        int used=0;
        for (size_t j=i+1;j<ir->count;j++) {
            CommonIRInstruction* y=&ir->instructions[j];
            if ((y->arg1 && !strcmp(y->arg1,x->result)) ||
                (y->arg2 && !strcmp(y->arg2,x->result))) {
                used=1; break;
            }
            if (y->op==CIR_LABEL || y->op==CIR_JUMP ||
                y->op==CIR_BRANCH || y->op==CIR_RETURN) break;
            if (y->result && !strcmp(y->result,x->result)) break;
        }
        if (!used) {
            free(x->result); free(x->arg1); free(x->arg2); free(x->extra);
            for (size_t k=i;k+1<ir->count;k++) ir->instructions[k]=ir->instructions[k+1];
            ir->count--; i--; changes++;
        }
    }
    return changes;
}


static int isTempName(const char* s) {
    return s && s[0]=='t' && s[1]>='0' && s[1]<='9';
}

static int commonIRCopyCleanup(CommonIR* ir) {
    if (!ir) return 0;
    int changes=0;

    /* Replace `ASSIGN tN <- value` uses with the value when safe within the
       same straight-line region, then remove the temporary assignment. */
    for (size_t i=0;i<ir->count;i++) {
        CommonIRInstruction* def=&ir->instructions[i];
        if (def->op!=CIR_ASSIGN || !isTempName(def->result) || !def->arg1) continue;

        int uses=0;
        size_t useIndex=0;
        for (size_t j=i+1;j<ir->count;j++) {
            CommonIRInstruction* x=&ir->instructions[j];
            if (x->op==CIR_LABEL || x->op==CIR_JUMP || x->op==CIR_BRANCH) break;
            if (x->arg1 && !strcmp(x->arg1,def->result)) { uses++; useIndex=j; }
            if (x->arg2 && !strcmp(x->arg2,def->result)) { uses++; useIndex=j; }
            if (x->result && !strcmp(x->result,def->result)) break;
        }
        if (uses==1) {
            CommonIRInstruction* use=&ir->instructions[useIndex];
            if (use->arg1 && !strcmp(use->arg1,def->result))
                replaceString(&use->arg1,def->arg1);
            if (use->arg2 && !strcmp(use->arg2,def->result))
                replaceString(&use->arg2,def->arg1);

            free(def->result); free(def->arg1); free(def->arg2); free(def->extra);
            for (size_t k=i;k+1<ir->count;k++) ir->instructions[k]=ir->instructions[k+1];
            ir->count--; i--; changes++;
        }
    }
    return changes;
}

int commonIROptimize(CommonIR* ir) {
    if (!ir) return 0;
    int total=0;
    /* Iterate because propagation can expose new folds. */
    for (int pass=0;pass<3;pass++) {
        int n=0;
        n+=commonIRConstantPropagation(ir);
        n+=commonIRConstantFold(ir);
        if (!n) break;
        total+=n;
    }
    total+=commonIRDeadCodeElimination(ir);
    total+=commonIRCopyCleanup(ir);
    return total;
}
