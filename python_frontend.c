#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "python_frontend.h"

typedef struct { const char* s; size_t p; } ExprParser;

static char* dupstr(const char* s) {
    size_t n=strlen(s)+1; char* p=(char*)malloc(n);
    if(p) memcpy(p,s,n);
    return p;
}
static void skip(ExprParser* p){ while(isspace((unsigned char)p->s[p->p])) p->p++; }
static ASTNode* exprOr(ExprParser* p);

static ASTNode* factor(ExprParser* p) {
    skip(p);
    if(p->s[p->p]=='(') {
        p->p++;
        ASTNode*n=exprOr(p);
        skip(p);
        if(p->s[p->p]==')') p->p++;
        return n;
    }
    if(p->s[p->p]=='-' || isdigit((unsigned char)p->s[p->p])) {
        size_t st=p->p;
        if(p->s[p->p]=='-') p->p++;
        while(isdigit((unsigned char)p->s[p->p]) || p->s[p->p]=='.') p->p++;
        char b[128]; size_t n=p->p-st; if(n>=sizeof(b)) n=sizeof(b)-1;
        memcpy(b,p->s+st,n); b[n]='\0';
        return createNode(strchr(b,'.')?AST_FLOAT:AST_NUMBER,b);
    }
    if(isalpha((unsigned char)p->s[p->p]) || p->s[p->p]=='_') {
        size_t st=p->p++;
        while(isalnum((unsigned char)p->s[p->p]) || p->s[p->p]=='_') p->p++;
        char b[128]; size_t n=p->p-st; if(n>=sizeof(b)) n=sizeof(b)-1;
        memcpy(b,p->s+st,n); b[n]='\0';
        if(!strcmp(b,"True")) return createNode(AST_NUMBER,"1");
        if(!strcmp(b,"False")) return createNode(AST_NUMBER,"0");
        return createNode(AST_IDENTIFIER,b);
    }
    return NULL;
}

static ASTNode* exprMul(ExprParser*p) {
    ASTNode*l=factor(p);
    for(;;) {
        skip(p); char c=p->s[p->p];
        if(c!='*'&&c!='/'&&c!='%') break;
        p->p++; char op[2]={c,'\0'};
        ASTNode*r=factor(p); if(!r){freeAST(l);return NULL;}
        ASTNode*n=createNode(AST_BINARY_OPERATION,op);n->left=l;n->right=r;l=n;
    }
    return l;
}
static ASTNode* exprAdd(ExprParser*p) {
    ASTNode*l=exprMul(p);
    for(;;) {
        skip(p); char c=p->s[p->p];
        if(c!='+'&&c!='-') break;
        p->p++; char op[2]={c,'\0'};
        ASTNode*r=exprMul(p); if(!r){freeAST(l);return NULL;}
        ASTNode*n=createNode(AST_BINARY_OPERATION,op);n->left=l;n->right=r;l=n;
    }
    return l;
}
static ASTNode* exprCmp(ExprParser*p) {
    ASTNode*l=exprAdd(p); skip(p);
    const char*ops[]={"==","!=","<=",">=","<",">"};
    for(int i=0;i<6;i++) {
        size_t n=strlen(ops[i]);
        if(!strncmp(p->s+p->p,ops[i],n)) {
            p->p+=n; ASTNode*r=exprAdd(p);
            if(!r){freeAST(l);return NULL;}
            ASTNode*x=createNode(AST_BINARY_OPERATION,ops[i]);
            x->left=l;x->right=r;return x;
        }
    }
    return l;
}
static ASTNode* exprAnd(ExprParser*p) {
    ASTNode*l=exprCmp(p);
    for(;;) {
        skip(p);
        if(strncmp(p->s+p->p,"and",3)!=0 ||
           (isalnum((unsigned char)p->s[p->p+3])||p->s[p->p+3]=='_')) break;
        p->p+=3; ASTNode*r=exprCmp(p);
        if(!r){freeAST(l);return NULL;}
        ASTNode*x=createNode(AST_BINARY_OPERATION,"&&");x->left=l;x->right=r;l=x;
    }
    return l;
}
static ASTNode* exprOr(ExprParser*p) {
    ASTNode*l=exprAnd(p);
    for(;;) {
        skip(p);
        if(strncmp(p->s+p->p,"or",2)!=0 ||
           (isalnum((unsigned char)p->s[p->p+2])||p->s[p->p+2]=='_')) break;
        p->p+=2; ASTNode*r=exprAnd(p);
        if(!r){freeAST(l);return NULL;}
        ASTNode*x=createNode(AST_BINARY_OPERATION,"||");x->left=l;x->right=r;l=x;
    }
    return l;
}
static ASTNode* parseExprText(const char* s) { ExprParser p={s,0}; return exprOr(&p); }

typedef struct {
    const char* text;
    char** lines;
    int* indents;
    int count;
    int index;
    char known[128][64];
    int knownCount;
} PyParser;

static char* trim(char* s) {
    while(isspace((unsigned char)*s)) s++;
    char* e=s+strlen(s);
    while(e>s && isspace((unsigned char)e[-1])) *--e='\0';
    return s;
}
static void append(ASTNode* parent,ASTNode* child) {
    if(!child)return;
    if(!parent->left){parent->left=child;return;}
    ASTNode* p=parent->left; while(p->next)p=p->next; p->next=child;
}
static char* afterWord(char* s,const char* word) {
    size_t n=strlen(word);
    if(strncmp(s,word,n)!=0 || (s[n] && !isspace((unsigned char)s[n]) && s[n]!=':')) return NULL;
    return trim(s+n);
}
static int knownName(PyParser* p,const char* name) {
    for(int i=0;i<p->knownCount;i++) if(!strcmp(p->known[i],name)) return 1;
    return 0;
}
static void rememberName(PyParser* p,const char* name) {
    if(p->knownCount<128 && !knownName(p,name)) {
        strncpy(p->known[p->knownCount],name,63);
        p->known[p->knownCount][63]='\0';
        p->knownCount++;
    }
}

static ASTNode* parseList(PyParser* p,int indent) {
    ASTNode* list=createNode(AST_BLOCK,NULL);
    while(p->index<p->count) {
        if(p->indents[p->index]<indent) break;
        if(p->indents[p->index]>indent) { p->index++; continue; }
        char* line=trim(p->lines[p->index++]);
        if(!*line || line[0]=='#') continue;

        char* q;
        if((q=afterWord(line,"if"))) {
            char* colon=strrchr(q,':'); if(!colon){freeAST(list);return NULL;}
            *colon='\0';
            ASTNode* n=createNode(AST_IF,"if"); n->condition=parseExprText(q);
            int nextIndent=(p->index<p->count)?p->indents[p->index]:indent;
            n->thenBranch=parseList(p,nextIndent);
            if(p->index<p->count && p->indents[p->index]==indent) {
                char* el=trim(p->lines[p->index]);
                if(!strcmp(el,"else:")) {
                    p->index++;
                    nextIndent=(p->index<p->count)?p->indents[p->index]:indent;
                    n->elseBranch=parseList(p,nextIndent);
                }
            }
            append(list,n); continue;
        }
        if((q=afterWord(line,"while"))) {
            char* colon=strrchr(q,':'); if(!colon){freeAST(list);return NULL;}
            *colon='\0';
            ASTNode* n=createNode(AST_WHILE,"while"); n->condition=parseExprText(q);
            int nextIndent=(p->index<p->count)?p->indents[p->index]:indent;
            n->body=parseList(p,nextIndent); append(list,n); continue;
        }
        if((q=afterWord(line,"return"))) {
            ASTNode* n=createNode(AST_RETURN,"return");
            if(*q) n->left=parseExprText(q);
            append(list,n); continue;
        }

        char* eq=strchr(line,'=');
        if(eq && (eq==line || eq[1]!='=')) {
            *eq='\0'; char* name=trim(line); char* rhs=trim(eq+1);
            ASTNode* n;
            if (knownName(p,name)) {
                n=createNode(AST_ASSIGNMENT,name);
                n->left=parseExprText(rhs);
            } else {
                n=createNode(AST_DECLARATION,name);
                n->declaredType=dupstr("inferred");
                n->initializer=parseExprText(rhs);
                n->left=n->initializer;
                rememberName(p,name);
            }
            append(list,n); continue;
        }
        freeAST(list); return NULL;
    }
    return list;
}

int pythonFrontendParse(const char* source, ASTNode** root) {
    if(!source||!root)return 0;
    *root=NULL;
    PyParser p={0};
    char* copy=dupstr(source);
    if(!copy)return 0;

    int cap=32; p.lines=(char**)calloc(cap,sizeof(char*)); p.indents=(int*)calloc(cap,sizeof(int));
    if(!p.lines||!p.indents){free(copy);return 0;}

    char* line=strtok(copy,"\n");
    while(line) {
        if(p.count==cap){cap*=2;p.lines=(char**)realloc(p.lines,cap*sizeof(char*));p.indents=(int*)realloc(p.indents,cap*sizeof(int));}
        int i=0; while(line[i]==' ')i++;
        p.lines[p.count]=line; p.indents[p.count]=i; p.count++;
        line=strtok(NULL,"\n");
    }
    p.index=0;
    ASTNode* block=parseList(&p,p.count? p.indents[0]:0);
    if(!block){ free(p.lines);free(p.indents);free(copy);return 0; }

    ASTNode* program=createNode(AST_PROGRAM,NULL);
    program->left=block->left; block->left=NULL; freeAST(block);
    *root=program;
    free(p.lines);free(p.indents);free(copy);
    return 1;
}
