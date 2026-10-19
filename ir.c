#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"


/*
 * Duplicate a string safely.
 */
static char* duplicateString(
    const char* value
) {

    if (value == NULL) {
        return NULL;
    }

    char* copy =
        malloc(
            strlen(value) + 1
        );

    if (copy == NULL) {

        printf(
            "Memory allocation failed.\n"
        );

        exit(1);
    }

    strcpy(copy, value);

    return copy;
}


/*
 * Initialize IR.
 */
void initIR(IRProgram* ir) {

    ir->count = 0;

    ir->temporaryCount = 0;
}


/*
 * Create a new temporary variable.
 *
 * Example:
 *
 * t1
 * t2
 * t3
 */
static char* newTemporary(
    IRProgram* ir
) {

    char temp[50];

    ir->temporaryCount++;

    sprintf(
        temp,
        "t%d",
        ir->temporaryCount
    );

    return duplicateString(temp);
}


/*
 * Add an IR instruction.
 */
static void addInstruction(
    IRProgram* ir,
    IRInstructionType type,
    const char* result,
    const char* operand1,
    const char* operator,
    const char* operand2
) {

    if (ir->count >=
        MAX_IR_INSTRUCTIONS) {

        printf(
            "IR instruction limit reached.\n"
        );

        return;
    }


    IRInstruction* instruction =
        &ir->instructions[ir->count];


    instruction->type =
        type;


    instruction->result =
        duplicateString(result);


    instruction->operand1 =
        duplicateString(operand1);


    instruction->operator =
        duplicateString(operator);


    instruction->operand2 =
        duplicateString(operand2);


    ir->count++;
}


/*
 * Generate IR for an expression.
 *
 * Returns the name of the location
 * containing the expression result.
 */
static char* generateExpressionIR(
    ASTNode* node,
    IRProgram* ir
) {

    if (node == NULL) {
        return NULL;
    }


    /*
     * NUMBER
     *
     * Example:
     *
     * 10
     */
    if (node->type ==
        AST_NUMBER) {

        return duplicateString(
            node->value
        );
    }


    /*
     * IDENTIFIER
     *
     * Example:
     *
     * x
     */
    if (node->type ==
        AST_IDENTIFIER) {

        return duplicateString(
            node->value
        );
    }


    /*
     * BINARY OPERATION
     *
     * Example:
     *
     * x + 5
     */
    if (node->type ==
        AST_BINARY_OPERATION) {

        /*
         * Generate IR for left side.
         */
        char* left =
            generateExpressionIR(
                node->left,
                ir
            );


        /*
         * Generate IR for right side.
         */
        char* right =
            generateExpressionIR(
                node->right,
                ir
            );


        /*
         * Create a temporary.
         */
        char* temporary =
            newTemporary(ir);


        /*
         * Generate:
         *
         * t1 = left op right
         */
        addInstruction(
            ir,
            IR_BINARY,
            temporary,
            left,
            node->value,
            right
        );


        free(left);

        free(right);


        return temporary;
    }


    return NULL;
}


/*
 * Generate IR for a statement.
 */
static void generateStatementIR(
    ASTNode* node,
    IRProgram* ir
) {

    if (node == NULL) {
        return;
    }


    /*
     * Declaration.
     *
     * Example:
     *
     * int x = 10;
     */
    if (node->type ==
        AST_DECLARATION) {

        if (node->left != NULL) {

            char* value =
                generateExpressionIR(
                    node->left,
                    ir
                );


            /*
             * Generate:
             *
             * x = value
             */
            addInstruction(
                ir,
                IR_ASSIGN,
                node->value,
                value,
                NULL,
                NULL
            );


            free(value);
        }

        else {

            /*
             * Declaration without
             * initialization.
             */
            addInstruction(
                ir,
                IR_DECLARATION,
                node->value,
                NULL,
                NULL,
                NULL
            );
        }

        return;
    }


    /*
     * Assignment.
     *
     * Example:
     *
     * x = y + 5;
     */
    if (node->type ==
        AST_ASSIGNMENT) {

        char* value =
            generateExpressionIR(
                node->left,
                ir
            );


        addInstruction(
            ir,
            IR_ASSIGN,
            node->value,
            value,
            NULL,
            NULL
        );


        free(value);

        return;
    }
}


/*
 * Generate IR for the entire AST.
 */
void generateIR(
    ASTNode* root,
    IRProgram* ir
) {

    if (root == NULL ||
        ir == NULL) {

        return;
    }


    if (root->type !=
        AST_PROGRAM) {

        return;
    }


    ASTNode* statement =
        root->left;


    while (statement != NULL) {

        generateStatementIR(
            statement,
            ir
        );


        statement =
            statement->next;
    }
}


/*
 * Print IR.
 */
void printIR(
    IRProgram* ir
) {

    if (ir == NULL) {
        return;
    }


    printf(
        "\n========== INTERMEDIATE REPRESENTATION ==========\n"
    );


    for (
        int i = 0;
        i < ir->count;
        i++
    ) {

        IRInstruction* instruction =
            &ir->instructions[i];


        if (instruction->type ==
            IR_BINARY) {

            printf(
                "%s = %s %s %s\n",
                instruction->result,
                instruction->operand1,
                instruction->operator,
                instruction->operand2
            );
        }


        else if (instruction->type ==
                 IR_ASSIGN) {

            printf(
                "%s = %s\n",
                instruction->result,
                instruction->operand1
            );
        }


        else if (instruction->type ==
                 IR_DECLARATION) {

            printf(
                "DECLARE %s\n",
                instruction->result
            );
        }
    }
}


/*
 * Free IR memory.
 */
void freeIR(
    IRProgram* ir
) {

    if (ir == NULL) {
        return;
    }


    for (
        int i = 0;
        i < ir->count;
        i++
    ) {

        free(
            ir->instructions[i].result
        );

        free(
            ir->instructions[i].operand1
        );

        free(
            ir->instructions[i].operator
        );

        free(
            ir->instructions[i].operand2
        );
    }


    ir->count = 0;

    ir->temporaryCount = 0;
}