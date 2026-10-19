#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static char* copyString(const char* s) {
    if (!s) return NULL;
    char* p = (char*)malloc(strlen(s) + 1);
    if (!p) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
    strcpy(p, s);
    return p;
}

ASTNode* createTypedNode(ASTNodeType type, const char* value, const char* declaredType, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    node->type = type;
    node->value = copyString(value);
    node->left = node->right = node->next = NULL;
    node->declaredType = copyString(declaredType);
    node->line = line;
    node->initializer = NULL;
    node->condition = NULL;
    node->thenBranch = NULL;
    node->elseBranch = NULL;
    node->body = NULL;
    return node;
}

ASTNode* createNode(ASTNodeType type, const char* value) {
    return createTypedNode(type, value, NULL, 0);
}

const char* astNodeTypeToString(ASTNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_BLOCK: return "BLOCK";
        case AST_DECLARATION: return "DECLARATION";
        case AST_ASSIGNMENT: return "ASSIGNMENT";
        case AST_BINARY_OPERATION: return "BINARY_OPERATION";
        case AST_UNARY_OPERATION: return "UNARY_OPERATION";
        case AST_NUMBER: return "NUMBER";
        case AST_FLOAT: return "FLOAT";
        case AST_STRING: return "STRING";
        case AST_CHAR: return "CHAR";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_IF: return "IF";
        case AST_WHILE: return "WHILE";
        case AST_FOR: return "FOR";
        case AST_RETURN: return "RETURN";
        case AST_FUNCTION: return "FUNCTION";
        default: return "UNKNOWN";
    }
}

static void printTree(ASTNode* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; ++i) printf("  ");
    printf("%s", astNodeTypeToString(node->type));

    if (node->value) printf(" : %s", node->value);
    if (node->declaredType) printf(" [type=%s]", node->declaredType);
    if (node->line > 0) printf(" [line=%d]", node->line);
    printf("\n");

    switch (node->type) {
        case AST_DECLARATION:
            if (node->initializer) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("INITIALIZER\n");
                printTree(node->initializer, depth+2);
            }
            break;
        case AST_ASSIGNMENT:
            printTree(node->left, depth+1);
            break;
        case AST_BINARY_OPERATION:
        case AST_UNARY_OPERATION:
            printTree(node->left, depth+1);
            printTree(node->right, depth+1);
            break;
        case AST_IF:
            if (node->condition) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("CONDITION\n");
                printTree(node->condition, depth+2);
            }
            if (node->thenBranch) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("THEN\n");
                printTree(node->thenBranch, depth+2);
            }
            if (node->elseBranch) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("ELSE\n");
                printTree(node->elseBranch, depth+2);
            }
            break;
        case AST_WHILE:
            if (node->condition) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("CONDITION\n");
                printTree(node->condition, depth+2);
            }
            if (node->body) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("BODY\n");
                printTree(node->body, depth+2);
            }
            break;
        case AST_BLOCK:
        case AST_PROGRAM:
            printTree(node->left, depth+1);
            break;
        case AST_RETURN:
            printTree(node->left, depth+1);
            break;
        case AST_FUNCTION:
            if (node->body) {
                for (int i=0;i<depth+1;i++) printf("  ");
                printf("BODY\n");
                printTree(node->body, depth+2);
            }
            break;
        default:
            break;
    }

    if (node->type != AST_PROGRAM && node->type != AST_BLOCK)
        printTree(node->next, depth);
}

void printAST(ASTNode* node, int depth) {
    printTree(node, depth);
}

void freeAST(ASTNode* node) {
    if (!node) return;

    /*
     * Free only unique structural links. For V2 control-flow nodes the
     * explicit fields are aliases of the semantic tree and are not freed
     * separately.
     */
    freeAST(node->left);
    freeAST(node->right);
    freeAST(node->next);

    free(node->value);
    free(node->declaredType);
    free(node);
}
