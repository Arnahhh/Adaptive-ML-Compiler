#ifndef IR_H
#define IR_H

#include "ast.h"


/*
 * Maximum number of IR instructions.
 */
#define MAX_IR_INSTRUCTIONS 500


/*
 * Types of IR instructions.
 */
typedef enum {

    IR_ASSIGN,
    IR_BINARY,
    IR_DECLARATION

} IRInstructionType;


/*
 * Represents one IR instruction.
 *
 * Examples:
 *
 * x = 10
 * t1 = a + b
 * z = t1
 */
typedef struct {

    IRInstructionType type;

    char* result;

    char* operand1;

    char* operator;

    char* operand2;

} IRInstruction;


/*
 * Represents the complete IR program.
 */
typedef struct {

    IRInstruction instructions[
        MAX_IR_INSTRUCTIONS
    ];

    int count;

    int temporaryCount;

} IRProgram;


/*
 * Initialize the IR program.
 */
void initIR(
    IRProgram* ir
);


/*
 * Generate IR from AST.
 */
void generateIR(
    ASTNode* root,
    IRProgram* ir
);


/*
 * Print the generated IR.
 */
void printIR(
    IRProgram* ir
);


/*
 * Free IR memory.
 */
void freeIR(
    IRProgram* ir
);

#endif