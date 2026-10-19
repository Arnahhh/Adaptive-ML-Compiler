#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_ir_lower.h"

static char* dupString(const char* s) {
    if (!s) return NULL;
    size_t n=strlen(s)+1;
    char* p=(char*)malloc(n);
    if (!p) return NULL;
    memcpy(p,s,n);
    return p;
}

static char* lowerExpr(const ASTNode* node, CommonIR* ir);
static int lowerList(const ASTNode* node, CommonIR* ir);

static char* lowerExpr(const ASTNode* node, CommonIR* ir) {
    if (!node) return NULL;

    if (node->type==AST_NUMBER || node->type==AST_FLOAT ||
        node->type==AST_STRING || node->type==AST_CHAR ||
        node->type==AST_IDENTIFIER)
        return dupString(node->value ? node->value : "");

    if (node->type==AST_BINARY_OPERATION) {
        char* a=lowerExpr(node->left,ir);
        char* b=lowerExpr(node->right,ir);
        char* t=commonIRNewTemp(ir);
        if (!a || !b || !t ||
            !commonIRAppend(ir,CIR_BINARY,t,a,b,node->value)) {
            free(a); free(b); free(t); return NULL;
        }
        free(a); free(b);
        return t;
    }

    if (node->type==AST_UNARY_OPERATION) {
        char* a=lowerExpr(node->left,ir);
        char* t=commonIRNewTemp(ir);
        if (!a || !t || !commonIRAppend(ir,CIR_UNARY,t,a,NULL,node->value)) {
            free(a); free(t); return NULL;
        }
        free(a);
        return t;
    }

    return NULL;
}

static int lowerBlock(const ASTNode* node, CommonIR* ir) {
    if (!node) return 1;
    if (node->type==AST_BLOCK || node->type==AST_PROGRAM)
        return lowerList(node->left,ir);
    return lowerList(node,ir);
}

static int lowerStatement(const ASTNode* n, CommonIR* ir) {
    if (!n) return 1;

    switch(n->type) {
        case AST_DECLARATION: {
            if (!commonIRAppend(ir,CIR_DECLARE,n->value,NULL,NULL,n->declaredType))
                return 0;
            if (n->initializer) {
                char* v=lowerExpr(n->initializer,ir);
                if (!v) return 0;
                int ok=commonIRAppend(ir,CIR_ASSIGN,n->value,v,NULL,NULL);
                free(v);
                return ok;
            }
            return 1;
        }

        case AST_ASSIGNMENT: {
            char* v=lowerExpr(n->left,ir);
            if (!v) return 0;
            int ok=commonIRAppend(ir,CIR_ASSIGN,n->value,v,NULL,NULL);
            free(v);
            return ok;
        }

        case AST_FUNCTION: {
            if (!commonIRAppend(ir,CIR_LABEL,n->value,NULL,NULL,n->declaredType))
                return 0;
            return lowerBlock(n->body,ir);
        }

        case AST_RETURN: {
            char* v=lowerExpr(n->left,ir);
            int ok=commonIRAppend(ir,CIR_RETURN,NULL,v,NULL,NULL);
            free(v);
            return ok;
        }

        case AST_IF: {
            char* cond=lowerExpr(n->condition,ir);
            char* elseL=commonIRNewLabel(ir);
            char* endL=commonIRNewLabel(ir);
            if (!cond || !elseL || !endL) { free(cond);free(elseL);free(endL);return 0; }

            if (!commonIRAppend(ir,CIR_BRANCH,elseL,cond,NULL,"false")) {
                free(cond);free(elseL);free(endL);return 0;
            }
            free(cond);

            if (!lowerBlock(n->thenBranch,ir)) { free(elseL);free(endL);return 0; }

            if (!commonIRAppend(ir,CIR_JUMP,endL,NULL,NULL,NULL) ||
                !commonIRAppend(ir,CIR_LABEL,elseL,NULL,NULL,NULL)) {
                free(elseL);free(endL);return 0;
            }

            if (n->elseBranch && !lowerBlock(n->elseBranch,ir)) {
                free(elseL);free(endL);return 0;
            }

            if (!commonIRAppend(ir,CIR_LABEL,endL,NULL,NULL,NULL)) {
                free(elseL);free(endL);return 0;
            }

            free(elseL);free(endL);
            return 1;
        }

        case AST_WHILE: {
            char* start=commonIRNewLabel(ir);
            char* end=commonIRNewLabel(ir);
            if (!start || !end) {free(start);free(end);return 0;}

            if (!commonIRAppend(ir,CIR_LABEL,start,NULL,NULL,NULL)) {
                free(start);free(end);return 0;
            }

            char* cond=lowerExpr(n->condition,ir);
            if (!cond) {free(start);free(end);return 0;}

            if (!commonIRAppend(ir,CIR_BRANCH,end,cond,NULL,"false")) {
                free(cond);free(start);free(end);return 0;
            }
            free(cond);

            if (!lowerBlock(n->body,ir) ||
                !commonIRAppend(ir,CIR_JUMP,start,NULL,NULL,NULL) ||
                !commonIRAppend(ir,CIR_LABEL,end,NULL,NULL,NULL)) {
                free(start);free(end);return 0;
            }

            free(start);free(end);
            return 1;
        }

        default:
            fprintf(stderr,"Common IR: unsupported AST node type %d\n",(int)n->type);
            return 0;
    }
}

static int lowerList(const ASTNode* node, CommonIR* ir) {
    for (const ASTNode* cur=node;cur;cur=cur->next)
        if (!lowerStatement(cur,ir)) return 0;
    return 1;
}

int lowerASTToCommonIR(const ASTNode* root, CommonIR* ir) {
    if (!root || !ir) return 0;
    commonIRInit(ir);
    return lowerBlock(root,ir);
}
