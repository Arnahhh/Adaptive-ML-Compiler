#ifndef AST_H
#define AST_H

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_BINARY_OPERATION,
    AST_UNARY_OPERATION,
    AST_NUMBER,
    AST_FLOAT,
    AST_STRING,
    AST_CHAR,
    AST_IDENTIFIER,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_FUNCTION
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char* value;

    /* Legacy expression/child links retained for optimizer compatibility. */
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* next;

    /* Explicit V2 semantic/control-flow fields. */
    char* declaredType;
    int line;
    struct ASTNode* initializer;
    struct ASTNode* condition;
    struct ASTNode* thenBranch;
    struct ASTNode* elseBranch;
    struct ASTNode* body;
} ASTNode;

ASTNode* createNode(ASTNodeType type, const char* value);
ASTNode* createTypedNode(ASTNodeType type, const char* value, const char* declaredType, int line);
void printAST(ASTNode* node, int depth);
void freeAST(ASTNode* node);
const char* astNodeTypeToString(ASTNodeType type);

#endif
