#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "java_frontend.h"
#include "error.h"

static char* dupstr(const char*s){
    if(!s) return NULL;
    size_t n=strlen(s)+1;
    char*p=malloc(n);
    if(p) memcpy(p,s,n);
    return p;
}

static char* trim(char*s){
    while(isspace((unsigned char)*s))s++;
    char*e=s+strlen(s);
    while(e>s&&isspace((unsigned char)e[-1]))*--e=0;
    return s;
}

static void append(ASTNode*b,ASTNode*n){
    if(!n)return;
    if(!b->left){b->left=n;return;}
    ASTNode*p=b->left;
    while(p->next)p=p->next;
    p->next=n;
}

typedef struct{const char*s;size_t p;int line;} EP;

static void ws(EP*p){
    while(isspace((unsigned char)p->s[p->p]))p->p++;
}

static ASTNode* orExpr(EP*);

static ASTNode* makeJavaNode(ASTNodeType type,const char*value,int line){
    return createTypedNode(type,value,NULL,line);
}

static ASTNode* atom(EP*p){
    ws(p);

    if(p->s[p->p]=='('){
        p->p++;
        ASTNode*n=orExpr(p);
        ws(p);
        if(p->s[p->p]==')')p->p++;
        return n;
    }

    if(isdigit((unsigned char)p->s[p->p])){
        size_t a=p->p;
        while(isdigit((unsigned char)p->s[p->p])||p->s[p->p]=='.')p->p++;
        char b[80];
        size_t n=p->p-a;
        if(n>79)n=79;
        memcpy(b,p->s+a,n);
        b[n]=0;
        return makeJavaNode(strchr(b,'.')?AST_FLOAT:AST_NUMBER,b,p->line);
    }

    if(isalpha((unsigned char)p->s[p->p])||p->s[p->p]=='_'){
        size_t a=p->p++;
        while(isalnum((unsigned char)p->s[p->p])||p->s[p->p]=='_')p->p++;
        char b[128];
        size_t n=p->p-a;
        if(n>127)n=127;
        memcpy(b,p->s+a,n);
        b[n]=0;

        if(!strcmp(b,"true")) return makeJavaNode(AST_NUMBER,"1",p->line);
        if(!strcmp(b,"false")) return makeJavaNode(AST_NUMBER,"0",p->line);
        return makeJavaNode(AST_IDENTIFIER,b,p->line);
    }

    return NULL;
}

static ASTNode* chain(EP*p,ASTNode*(*sub)(EP*),const char*ops){
    ASTNode*l=sub(p);

    for(;;){
        ws(p);
        char c=p->s[p->p];
        if(c=='\0'||!strchr(ops,c))break;

        p->p++;
        char op[2]={c,0};
        ASTNode*r=sub(p);

        if(!r){
            freeAST(l);
            return NULL;
        }

        ASTNode*n=makeJavaNode(AST_BINARY_OPERATION,op,p->line);
        n->left=l;
        n->right=r;
        l=n;
    }

    return l;
}

static ASTNode* mul(EP*p){return chain(p,atom,"*/%");}
static ASTNode* add(EP*p){return chain(p,mul,"+-");}

static ASTNode* cmp(EP*p){
    ASTNode*l=add(p);
    ws(p);

    const char*o[]={"==","!=","<=",">=","<",">"};

    for(int i=0;i<6;i++){
        size_t n=strlen(o[i]);

        if(!strncmp(p->s+p->p,o[i],n)){
            p->p+=n;
            ASTNode*r=add(p);

            if(!r){
                freeAST(l);
                return NULL;
            }

            ASTNode*x=makeJavaNode(AST_BINARY_OPERATION,o[i],p->line);
            x->left=l;
            x->right=r;
            return x;
        }
    }

    return l;
}

static ASTNode* andExpr(EP*p){
    ASTNode*l=cmp(p);

    for(;;){
        ws(p);
        if(strncmp(p->s+p->p,"&&",2))break;

        p->p+=2;
        ASTNode*r=cmp(p);
        if(!r){
            freeAST(l);
            return NULL;
        }

        ASTNode*x=makeJavaNode(AST_BINARY_OPERATION,"&&",p->line);
        x->left=l;
        x->right=r;
        l=x;
    }

    return l;
}

static ASTNode* orExpr(EP*p){
    ASTNode*l=andExpr(p);

    for(;;){
        ws(p);
        if(strncmp(p->s+p->p,"||",2))break;

        p->p+=2;
        ASTNode*r=andExpr(p);
        if(!r){
            freeAST(l);
            return NULL;
        }

        ASTNode*x=makeJavaNode(AST_BINARY_OPERATION,"||",p->line);
        x->left=l;
        x->right=r;
        l=x;
    }

    return l;
}

static ASTNode* expr(const char*s,int line){
    EP p={s,0,line};
    return orExpr(&p);
}

static int isEmpty(char*s){
    s=trim(s);
    return *s==0;
}

static void stripSemi(char*s){
    size_t n=strlen(s);
    if(n&&s[n-1]==';')s[n-1]=0;
}

static char* stripModifiers(char*s){
    const char*m[]={
        "public ","private ","protected ",
        "static ","final ","abstract "
    };

    int again=1;

    while(again){
        again=0;
        s=trim(s);

        for(size_t i=0;i<6;i++){
            if(!strncmp(s,m[i],strlen(m[i]))){
                s=trim(s+strlen(m[i]));
                again=1;
                break;
            }
        }
    }

    return s;
}

typedef struct{char**v;int n,i;} Lines;

static int javaParseFailed = 0;

static int hasTrailingSemicolon(const char*line){
    if(!line) return 0;
    size_t n=strlen(line);
    while(n>0 && isspace((unsigned char)line[n-1])) n--;
    return n>0 && line[n-1]==';';
}

static void reportMissingSemicolon(int lineNo, const char*found){
    javaParseFailed = 1;
    reportError(
        ERROR_MISSING_SEMICOLON,
        lineNo,
        "Missing semicolon at end of Java statement.",
        found ? found : "",
        ";"
    );
}

static ASTNode* block(Lines*L);

static ASTNode* statement(Lines*L){
    int lineNo=L->i+1;
    char*line=trim(L->v[L->i]);

    if(!strncmp(line,"if",2)){
        char*q=strchr(line,'(');
        char*r=strrchr(line,')');

        if(!q||!r){
            javaParseFailed = 1;
            reportError(ERROR_MISSING_RPAREN, lineNo,
                        "Missing closing parenthesis in Java control statement.",
                        line, ")");
            L->i++;
            return NULL;
        }

        int inlineOpen=strchr(r+1,'{')!=NULL;

        *q=0;
        q++;
        *r=0;

        ASTNode*n=makeJavaNode(AST_IF,"if",lineNo);
        n->condition=expr(q,lineNo);
        L->i++;

        while(!inlineOpen&&L->i<L->n&&isEmpty(L->v[L->i]))L->i++;

        if(inlineOpen){
            n->thenBranch=block(L);
        }else if(L->i<L->n&&strchr(L->v[L->i],'{')){
            L->i++;
            n->thenBranch=block(L);
        }else{
            return n;
        }

        while(L->i<L->n&&isEmpty(L->v[L->i]))L->i++;

        if(L->i<L->n){
            char*t=trim(L->v[L->i]);

            if(!strncmp(t,"else",4)||strstr(t,"else")!=NULL){
                int elseInline=strchr(t,'{')!=NULL;
                L->i++;

                while(!elseInline&&L->i<L->n&&isEmpty(L->v[L->i]))L->i++;

                if(elseInline){
                    n->elseBranch=block(L);
                }else if(L->i<L->n&&strchr(L->v[L->i],'{')){
                    L->i++;
                    n->elseBranch=block(L);
                }
            }
        }

        return n;
    }

    if(!strncmp(line,"while",5)){
        char*q=strchr(line,'(');
        char*r=strrchr(line,')');

        if(!q||!r){
            javaParseFailed = 1;
            reportError(ERROR_MISSING_RPAREN, lineNo,
                        "Missing closing parenthesis in Java control statement.",
                        line, ")");
            L->i++;
            return NULL;
        }

        int inlineOpen=strchr(r+1,'{')!=NULL;

        *q=0;
        q++;
        *r=0;

        ASTNode*n=makeJavaNode(AST_WHILE,"while",lineNo);
        n->condition=expr(q,lineNo);
        L->i++;

        while(!inlineOpen&&L->i<L->n&&isEmpty(L->v[L->i]))L->i++;

        if(inlineOpen){
            n->body=block(L);
        }else if(L->i<L->n&&strchr(L->v[L->i],'{')){
            L->i++;
            n->body=block(L);
        }

        return n;
    }

    if(!strncmp(line,"return",6)&&(line[6]==' '||line[6]==';')){
        if(!hasTrailingSemicolon(line)){
            reportMissingSemicolon(lineNo, line);
        }

        char*t=trim(line+6);
        stripSemi(t);

        ASTNode*n=makeJavaNode(AST_RETURN,"return",lineNo);
        if(*t)n->left=expr(t,lineNo);

        L->i++;
        return n;
    }

    line=stripModifiers(line);

    char type[32]={0};
    char name[64]={0};

    if(sscanf(line,"%31s %63[^=; ]",type,name)==2 &&
       (!strcmp(type,"int")||
        !strcmp(type,"double")||
        !strcmp(type,"float")||
        !strcmp(type,"boolean")||
        !strcmp(type,"char")||
        !strcmp(type,"String"))){

        if(!hasTrailingSemicolon(line)){
            reportMissingSemicolon(lineNo, line);
        }

        ASTNode*n=makeJavaNode(AST_DECLARATION,name,lineNo);
        n->declaredType=dupstr(type);

        char*eq=strchr(line,'=');

        if(eq){
            char*t=trim(eq+1);
            stripSemi(t);
            n->initializer=expr(t,lineNo);
            n->left=n->initializer;
        }

        L->i++;
        return n;
    }

    char*eq=strchr(line,'=');

    if(eq){
        if(!hasTrailingSemicolon(line)){
            reportMissingSemicolon(lineNo, line);
        }

        *eq=0;
        char*nm=trim(line);
        char*t=trim(eq+1);
        stripSemi(t);

        ASTNode*n=makeJavaNode(AST_ASSIGNMENT,nm,lineNo);
        n->left=expr(t,lineNo);

        L->i++;
        return n;
    }

    L->i++;
    return NULL;
}

static ASTNode* block(Lines*L){
    ASTNode*b=makeJavaNode(AST_BLOCK,NULL,L->i+1);

    while(L->i<L->n){
        char*t=trim(L->v[L->i]);

        if(!strncmp(t,"}",1)){
            if(strstr(t,"else")!=NULL)break;
            L->i++;
            break;
        }

        if(isEmpty(t)||!strncmp(t,"//",2)){
            L->i++;
            continue;
        }

        ASTNode*n=statement(L);
        if(n)append(b,n);
    }

    return b;
}

int javaFrontendParse(const char*source,ASTNode**root){
    if(!source||!root)return 0;

    *root=NULL;
    javaParseFailed = 0;
    clearError();

    char*copy=dupstr(source);
    if(!copy)return 0;

    int cap=64,n=0;
    char**v=calloc(cap,sizeof(char*));

    char*line=strtok(copy,"\n");

    while(line){
        if(n==cap){
            cap*=2;
            v=realloc(v,cap*sizeof(char*));
        }

        v[n++]=line;
        line=strtok(NULL,"\n");
    }

    int mainLine=-1;

    for(int i=0;i<n;i++){
        if(strstr(v[i],"main(")||strstr(v[i],"main (")){
            mainLine=i;
            break;
        }
    }

    if(mainLine<0){
        free(v);
        free(copy);
        return 0;
    }

    Lines L={v,n,mainLine+1};

    if(!strchr(v[mainLine],'{')){
        free(v);
        free(copy);
        return 0;
    }

    ASTNode*body=block(&L);
    ASTNode*p=makeJavaNode(AST_PROGRAM,NULL,mainLine+1);

    p->left=body?body->left:NULL;

    if(body){
        body->left=NULL;
        freeAST(body);
    }

    *root=p;

    free(v);
    free(copy);

    if(javaParseFailed){
        /* Keep the recovered AST so semantic analysis can report additional
           errors after syntax errors have been collected. */
        return 0;
    }

    return 1;
}
