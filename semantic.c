#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"
#include "symbol.h"

static SemanticResult* gResult = NULL;

static void addError(const char*msg, ASTNode*n){
    if(!gResult)return;
    gResult->errors++;
    fprintf(stderr,"Semantic Error: %s '%s' at line %d\n",
            msg,(n&&n->value)?n->value:"",n?n->line:0);
}
static SymbolType mapType(const char*t){
    if(!t)return SYMBOL_TYPE_UNKNOWN;
    if(!strcmp(t,"int")||!strcmp(t,"bool")||!strcmp(t,"boolean"))return SYMBOL_TYPE_INT;
    if(!strcmp(t,"float")||!strcmp(t,"double"))return SYMBOL_TYPE_FLOAT;
    if(!strcmp(t,"char"))return SYMBOL_TYPE_CHAR;
    if(!strcmp(t,"String")||!strcmp(t,"string"))return SYMBOL_TYPE_STRING;
    return SYMBOL_TYPE_UNKNOWN;
}
static void collectDeclarations(ASTNode*n);
static void checkExpr(ASTNode*n);

static void collectDeclarations(ASTNode*n){
    for(ASTNode*x=n;x;x=x->next){
        switch(x->type){
        case AST_DECLARATION:
            if(!x->value||!x->declaredType){addError("invalid declaration",x);break;}
            if(symbolLookup(&gResult->symbols,x->value)){
                addError("duplicate declaration",x);
            }else{
                symbolDeclare(&gResult->symbols,x->value,mapType(x->declaredType),x->line);
            }
            if(x->initializer)collectDeclarations(x->initializer);
            break;
        case AST_IF:
            collectDeclarations(x->thenBranch);
            collectDeclarations(x->elseBranch);
            break;
        case AST_WHILE:
            collectDeclarations(x->body);break;
        case AST_FOR:
            collectDeclarations(x->body);break;
        case AST_BLOCK:
            collectDeclarations(x->left);break;
        case AST_FUNCTION:
            collectDeclarations(x->body);break;
        default:
            break;
        }
    }
}
static void checkExpr(ASTNode*n){
    if(!n)return;
    if(n->type==AST_IDENTIFIER){
        if(!symbolLookup(&gResult->symbols,n->value))
            addError("use of undeclared identifier",n);
        return;
    }
    if(n->left)checkExpr(n->left);
    if(n->right)checkExpr(n->right);
}
static void checkStatements(ASTNode*n){
    for(ASTNode*x=n;x;x=x->next){
        switch(x->type){
        case AST_DECLARATION: checkExpr(x->initializer);break;
        case AST_ASSIGNMENT:
            if(!symbolLookup(&gResult->symbols,x->value))
                addError("assignment to undeclared identifier",x);
            checkExpr(x->left);break;
        case AST_IF:
            checkExpr(x->condition);checkStatements(x->thenBranch);checkStatements(x->elseBranch);break;
        case AST_WHILE:
            checkExpr(x->condition);checkStatements(x->body);break;
        case AST_FOR:
            checkExpr(x->condition);checkStatements(x->body);break;
        case AST_RETURN: checkExpr(x->left);break;
        case AST_BLOCK: checkStatements(x->left);break;
        case AST_FUNCTION: checkStatements(x->body);break;
        default: checkExpr(x);break;
        }
    }
}
void semanticResultInit(SemanticResult*r){
    if(!r)return;
    memset(r,0,sizeof(*r));
    initSymbolTable(&r->symbols);
}
void semanticResultFree(SemanticResult*r){
    if(!r)return;
    freeSymbolTable(&r->symbols);
    memset(r,0,sizeof(*r));
}
int semanticAnalyze(ASTNode*root,SemanticResult*r){
    if(!r)return 1;
    /* The caller normally initializes this, but make the function safe either way. */
    r->errors=0;r->warnings=0;
    gResult=r;

    if(root&&root->type==AST_PROGRAM){
        /* First pass makes declarations visible regardless of their nesting,
           matching the compiler's current language subset. */
        collectDeclarations(root->left);
        checkStatements(root->left);
    }else if(root){
        collectDeclarations(root);
        checkStatements(root);
    }
    gResult=NULL;
    return r->errors;
}
