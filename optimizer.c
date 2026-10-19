#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optimizer.h"


#define MAX_CONSTANTS 100


/*
 * Structure used to store known constants.
 */
typedef struct {

    char* name;
    int value;

} Constant;


/*
 * Constant table.
 */
Constant constants[MAX_CONSTANTS];

int constantCount = 0;


/*
 * =========================================================
 * BASIC HELPER FUNCTIONS
 * =========================================================
 */


/*
 * Check whether a node contains a number.
 */
int isNumberNode(ASTNode* node) {

    return node != NULL &&
           node->type == AST_NUMBER;
}


/*
 * =========================================================
 * CONSTANT TABLE FUNCTIONS
 * =========================================================
 */


/*
 * Find the value of a known constant.
 *
 * Returns:
 *
 * 1 -> constant found
 * 0 -> constant not found
 */
int getConstantValue(
    const char* name,
    int* value
) {

    for (int i = 0; i < constantCount; i++) {

        if (strcmp(
                constants[i].name,
                name
            ) == 0) {

            *value =
                constants[i].value;

            return 1;
        }
    }

    return 0;
}


/*
 * Remove a variable from the constant table.
 */
void removeConstant(
    const char* name
) {

    for (int i = 0; i < constantCount; i++) {

        if (strcmp(
                constants[i].name,
                name
            ) == 0) {

            /*
             * Free the variable name.
             */
            free(constants[i].name);


            /*
             * Move remaining entries
             * one position backward.
             */
            for (
                int j = i;
                j < constantCount - 1;
                j++
            ) {

                constants[j] =
                    constants[j + 1];
            }


            constantCount--;

            return;
        }
    }
}


/*
 * Add a constant to the table.
 *
 * If the variable already exists,
 * update its value.
 */
void addConstant(
    const char* name,
    int value
) {

    /*
     * Check whether the variable
     * already exists.
     */
    for (int i = 0; i < constantCount; i++) {

        if (strcmp(
                constants[i].name,
                name
            ) == 0) {

            constants[i].value =
                value;

            return;
        }
    }


    /*
     * Check whether the table is full.
     */
    if (constantCount >= MAX_CONSTANTS) {

        printf(
            "Warning: Constant table is full.\n"
        );

        return;
    }


    /*
     * Allocate memory for the name.
     */
    constants[constantCount].name =
        malloc(strlen(name) + 1);


    if (constants[constantCount].name == NULL) {

        printf(
            "Memory allocation failed.\n"
        );

        return;
    }


    strcpy(
        constants[constantCount].name,
        name
    );


    /*
     * Store the value.
     */
    constants[constantCount].value =
        value;


    constantCount++;
}


/*
 * Reset the constant table.
 */
void resetConstantTable(void) {

    for (int i = 0; i < constantCount; i++) {

        free(constants[i].name);

        constants[i].name = NULL;
    }


    constantCount = 0;
}


/*
 * Free the constant table.
 */
void freeConstantTable(void) {

    resetConstantTable();
}


/*
 * =========================================================
 * CONSTANT FOLDING
 * =========================================================
 */


/*
 * Perform an arithmetic operation.
 *
 * Returns:
 *
 * 1 -> calculation successful
 * 0 -> calculation not possible
 */
int calculate(
    int left,
    int right,
    char operator,
    int* result
) {

    switch (operator) {

        case '+':

            *result =
                left + right;

            return 1;


        case '-':

            *result =
                left - right;

            return 1;


        case '*':

            *result =
                left * right;

            return 1;


        case '/':

            /*
             * Do not fold division by zero.
             */
            if (right == 0) {

                printf(
                    "Warning: Division by zero. "
                    "Expression was not folded.\n"
                );

                return 0;
            }


            *result =
                left / right;

            return 1;


        default:

            return 0;
    }
}


/*
 * Constant Folding.
 *
 * Example:
 *
 * 10 + 20
 *
 * becomes:
 *
 * 30
 */
void constantFold(
    ASTNode* node
) {

    if (node == NULL) {
        return;
    }


    /*
     * First process child nodes.
     */
    constantFold(node->left);

    constantFold(node->right);

    constantFold(node->next);


    /*
     * Check for binary operation.
     */
    if (node->type ==
        AST_BINARY_OPERATION) {

        /*
         * Both children must be numbers.
         */
        if (isNumberNode(node->left) &&
            isNumberNode(node->right)) {

            int left =
                atoi(
                    node->left->value
                );

            int right =
                atoi(
                    node->right->value
                );

            char operator =
                node->value[0];


            int result;


            /*
             * Calculate the result.
             */
            if (!calculate(
                    left,
                    right,
                    operator,
                    &result
                )) {

                return;
            }


            /*
             * Convert result to string.
             */
            char resultString[50];

            sprintf(
                resultString,
                "%d",
                result
            );


            /*
             * Delete old left child.
             */
            free(node->left->value);

            free(node->left);


            /*
             * Delete old right child.
             */
            free(node->right->value);

            free(node->right);


            node->left = NULL;
            node->right = NULL;


            /*
             * Change operation into
             * a NUMBER node.
             */
            node->type =
                AST_NUMBER;


            /*
             * Replace operator with
             * calculated result.
             */
            free(node->value);


            node->value =
                malloc(
                    strlen(resultString) + 1
                );


            if (node->value == NULL) {

                printf(
                    "Memory allocation failed.\n"
                );

                return;
            }


            strcpy(
                node->value,
                resultString
            );
        }
    }
}


/*
 * =========================================================
 * CONSTANT PROPAGATION
 * =========================================================
 *
 * Example:
 *
 * int x = 10;
 * int y = x + 5;
 *
 * becomes:
 *
 * int x = 10;
 * int y = 10 + 5;
 */
void constantPropagate(
    ASTNode* node
) {

    if (node == NULL) {
        return;
    }


    /*
     * Process the complete program.
     */
    if (node->type ==
        AST_PROGRAM) {

        /*
         * Clear constants from
         * previous compilation.
         */
        resetConstantTable();


        ASTNode* statement =
            node->left;


        while (statement != NULL) {

            constantPropagate(
                statement
            );

            statement =
                statement->next;
        }

        return;
    }


    /*
     * Handle declarations.
     */
    if (node->type ==
        AST_DECLARATION) {

        /*
         * First process the expression.
         */
        constantPropagate(
            node->left
        );


        /*
         * If the expression is constant,
         * store it.
         */
        if (node->left != NULL &&
            node->left->type ==
                AST_NUMBER) {

            int value =
                atoi(
                    node->left->value
                );


            addConstant(
                node->value,
                value
            );
        }

        else {

            /*
             * This declaration does not
             * contain a known constant.
             */
            removeConstant(
                node->value
            );
        }

        return;
    }


    /*
     * Handle assignments.
     */
    if (node->type ==
        AST_ASSIGNMENT) {

        /*
         * First propagate constants
         * inside the expression.
         */
        constantPropagate(
            node->left
        );


        /*
         * If the assignment produces
         * a constant, remember it.
         */
        if (node->left != NULL &&
            node->left->type ==
                AST_NUMBER) {

            int value =
                atoi(
                    node->left->value
                );


            addConstant(
                node->value,
                value
            );
        }

        else {

            /*
             * The old constant value is
             * no longer valid.
             */
            removeConstant(
                node->value
            );
        }

        return;
    }


    /*
     * Handle binary operations.
     */
    if (node->type ==
        AST_BINARY_OPERATION) {

        constantPropagate(
            node->left
        );

        constantPropagate(
            node->right
        );

        return;
    }


    /*
     * Handle identifiers.
     */
    if (node->type ==
        AST_IDENTIFIER) {

        int value;


        /*
         * Check whether the identifier
         * has a known constant value.
         */
        if (getConstantValue(
                node->value,
                &value
            )) {

            char numberString[50];


            sprintf(
                numberString,
                "%d",
                value
            );


            /*
             * Replace identifier value.
             */
            free(node->value);


            node->value =
                malloc(
                    strlen(numberString) + 1
                );


            if (node->value == NULL) {

                printf(
                    "Memory allocation failed.\n"
                );

                return;
            }


            strcpy(
                node->value,
                numberString
            );


            /*
             * Change IDENTIFIER
             * into NUMBER.
             */
            node->type =
                AST_NUMBER;
        }

        return;
    }
}


/*
 * =========================================================
 * COMMON SUBEXPRESSION ELIMINATION
 * =========================================================
 */


/*
 * Check whether two AST expressions
 * are exactly the same.
 *
 * Example:
 *
 * a + b
 *
 * and
 *
 * a + b
 *
 * are equal.
 */
int expressionsEqual(
    ASTNode* first,
    ASTNode* second
) {

    /*
     * Both NULL means equal.
     */
    if (first == NULL &&
        second == NULL) {

        return 1;
    }


    /*
     * Only one NULL means different.
     */
    if (first == NULL ||
        second == NULL) {

        return 0;
    }


    /*
     * Node types must match.
     */
    if (first->type !=
        second->type) {

        return 0;
    }


    /*
     * Compare values.
     */
    if (first->value == NULL &&
        second->value != NULL) {

        return 0;
    }


    if (first->value != NULL &&
        second->value == NULL) {

        return 0;
    }


    if (first->value != NULL &&
        second->value != NULL) {

        if (strcmp(
                first->value,
                second->value
            ) != 0) {

            return 0;
        }
    }


    /*
     * Compare left subtree.
     */
    if (!expressionsEqual(
            first->left,
            second->left
        )) {

        return 0;
    }


    /*
     * Compare right subtree.
     */
    if (!expressionsEqual(
            first->right,
            second->right
        )) {

        return 0;
    }


    return 1;
}


/*
 * Perform Common Subexpression Elimination.
 *
 * Example:
 *
 * int x = a + b;
 * int y = a + b;
 *
 * becomes:
 *
 * int x = a + b;
 * int y = x;
 */
void commonSubexpressionElimination(
    ASTNode* node
) {

    if (node == NULL) {
        return;
    }


    /*
     * CSE operates on the program
     * statement list.
     */
    if (node->type !=
        AST_PROGRAM) {

        return;
    }


    ASTNode* current =
        node->left;


    /*
     * Examine each statement.
     */
    while (current != NULL) {

        /*
         * We only handle declarations
         * containing binary expressions.
         */
        if (current->type ==
                AST_DECLARATION &&
            current->left != NULL &&
            current->left->type ==
                AST_BINARY_OPERATION) {

            ASTNode* later =
                current->next;


            /*
             * Compare against later
             * declarations.
             */
            while (later != NULL) {

                if (later->type ==
                        AST_DECLARATION &&
                    later->left != NULL &&
                    later->left->type ==
                        AST_BINARY_OPERATION) {

                    /*
                     * Check whether the
                     * expressions are identical.
                     */
                    if (expressionsEqual(
                            current->left,
                            later->left
                        )) {

                        printf(
                            "\nCSE: Replacing repeated "
                            "expression '%s' with '%s'\n",
                            later->left->value,
                            current->value
                        );


                        /*
                         * Create a new identifier
                         * containing the name of
                         * the first variable.
                         *
                         * Example:
                         *
                         * y = a + b
                         *
                         * becomes:
                         *
                         * y = x
                         */
                        ASTNode* replacement =
                            createNode(
                                AST_IDENTIFIER,
                                current->value
                            );


                        /*
                         * Delete the repeated
                         * expression.
                         */
                        freeAST(
                            later->left
                        );


                        /*
                         * Replace it with
                         * the identifier.
                         */
                        later->left =
                            replacement;
                    }
                }


                later =
                    later->next;
            }
        }


        current =
            current->next;
    }
}


/*
 * =========================================================
 * DEAD CODE ELIMINATION
 * =========================================================
 */


/*
 * Check whether an identifier is used
 * inside an AST subtree.
 */
int isVariableUsed(
    ASTNode* node,
    const char* variable
) {

    if (node == NULL) {
        return 0;
    }


    /*
     * Check current node.
     */
    if (node->type ==
            AST_IDENTIFIER &&
        node->value != NULL &&
        strcmp(
            node->value,
            variable
        ) == 0) {

        return 1;
    }


    /*
     * Search left subtree.
     */
    if (isVariableUsed(
            node->left,
            variable
        )) {

        return 1;
    }


    /*
     * Search right subtree.
     */
    if (isVariableUsed(
            node->right,
            variable
        )) {

        return 1;
    }


    return 0;
}


/*
 * Dead Code Elimination.
 *
 * Removes unused declarations.
 *
 * The final declaration is preserved
 * in this simplified compiler because
 * we currently do not have an explicit
 * observable program result.
 */
void deadCodeElimination(
    ASTNode* node
) {

    if (node == NULL) {
        return;
    }


    /*
     * DCE operates on the program.
     */
    if (node->type !=
        AST_PROGRAM) {

        return;
    }


    ASTNode* previous =
        NULL;

    ASTNode* current =
        node->left;


    while (current != NULL) {

        /*
         * Only consider declarations
         * that have another statement
         * after them.
         */
        if (current->type ==
                AST_DECLARATION &&
            current->value != NULL &&
            current->next != NULL) {

            int used = 0;


            /*
             * Search later statements.
             */
            ASTNode* later =
                current->next;


            while (later != NULL) {

                if (isVariableUsed(
                        later->left,
                        current->value
                    )) {

                    used = 1;

                    break;
                }


                later =
                    later->next;
            }


            /*
             * Remove declaration if
             * it is never used.
             */
            if (!used) {

                ASTNode* deadNode =
                    current;


                /*
                 * Move to next statement.
                 */
                current =
                    current->next;


                /*
                 * Reconnect statement list.
                 */
                if (previous == NULL) {

                    node->left =
                        current;
                }

                else {

                    previous->next =
                        current;
                }


                /*
                 * Disconnect the dead node
                 * before freeing it.
                 *
                 * Otherwise freeAST()
                 * would also free later
                 * statements.
                 */
                deadNode->next =
                    NULL;


                /*
                 * Free the dead declaration.
                 */
                freeAST(
                    deadNode
                );


                /*
                 * Do not move previous.
                 */
                continue;
            }
        }


        previous =
            current;

        current =
            current->next;
    }
}