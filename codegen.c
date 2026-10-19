#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "ir.h"
#include <string.h>


/*
 * Check whether a string represents
 * an integer constant.
 */
static int isNumber(const char* value) {

    if (value == NULL) {
        return 0;
    }

    int i = 0;

    /*
     * Allow negative numbers.
     */
    if (value[0] == '-') {
        i = 1;
    }

    /*
     * Empty string or only '-'.
     */
    if (value[i] == '\0') {
        return 0;
    }

    while (value[i] != '\0') {

        if (value[i] < '0' ||
            value[i] > '9') {

            return 0;
        }

        i++;
    }

    return 1;
}


/*
 * Load an operand into EAX.
 */
static void loadOperand(
    FILE* file,
    const char* operand
) {

    if (isNumber(operand)) {

        fprintf(
            file,
            "    mov eax, %s\n",
            operand
        );

    } else {

        fprintf(
            file,
            "    mov eax, DWORD PTR [%s]\n",
            operand
        );
    }
}


/*
 * Generate complete executable
 * x86 assembly code.
 */
void generateCode(
    IRProgram* ir,
    const char* filename
) {

    if (ir == NULL ||
        filename == NULL) {

        return;
    }


    /*
     * Open assembly file.
     */
    FILE* file =
        fopen(
            filename,
            "w"
        );


    if (file == NULL) {

        printf(
            "Error: Could not create assembly file.\n"
        );

        return;
    }


    /*
     * =========================================
     * INTEL SYNTAX
     * =========================================
     */

    fprintf(
        file,
        ".intel_syntax noprefix\n"
    );


    /*
     * =========================================
     * DATA SECTION
     * =========================================
     */

    fprintf(
        file,
        "\n.section .data\n"
    );


    /*
     * Format string used by printf.
     *
     * The generated executable will print:
     *
     * z = 50
     */
    fprintf(
        file,
        "output_format: .asciz \"z = %%d\\n\"\n"
    );


    /*
     * Create storage for variables
     * and temporary values.
     *
     * Avoid duplicate declarations.
     */
    for (
        int i = 0;
        i < ir->count;
        i++
    ) {

        IRInstruction* instruction =
            &ir->instructions[i];


        if (
            instruction->result == NULL
        ) {
            continue;
        }


        /*
         * Check whether this result
         * was already declared.
         */
        int alreadyDeclared = 0;


        for (
            int j = 0;
            j < i;
            j++
        ) {

            if (
                ir->instructions[j].result != NULL
                &&
                strcmp(
                    ir->instructions[j].result,
                    instruction->result
                ) == 0
            ) {

                alreadyDeclared = 1;

                break;
            }
        }


        if (!alreadyDeclared) {

            fprintf(
                file,
                "%s: .long 0\n",
                instruction->result
            );
        }
    }


    /*
     * =========================================
     * TEXT SECTION
     * =========================================
     */

    fprintf(
        file,
        "\n.section .text\n"
    );


    /*
     * MinGW32 uses _main.
     */
    fprintf(
        file,
        ".globl _main\n"
    );


    fprintf(
        file,
        ".extern _printf\n"
    );


    fprintf(
        file,
        "_main:\n"
    );


    /*
     * =========================================
     * GENERATE CODE FOR EACH IR INSTRUCTION
     * =========================================
     */

    for (
        int i = 0;
        i < ir->count;
        i++
    ) {

        IRInstruction* instruction =
            &ir->instructions[i];


        /*
         * =====================================
         * DECLARATION
         * =====================================
         */
        if (
            instruction->type ==
            IR_DECLARATION
        ) {

            fprintf(
                file,
                "    # declare %s\n",
                instruction->result
            );

            continue;
        }


        /*
         * =====================================
         * ASSIGNMENT
         * =====================================
         *
         * Example:
         *
         * z = 50
         */
        if (
            instruction->type ==
            IR_ASSIGN
        ) {

            fprintf(
                file,
                "    # %s = %s\n",
                instruction->result,
                instruction->operand1
            );


            /*
             * Load the source.
             */
            if (
                isNumber(
                    instruction->operand1
                )
            ) {

                fprintf(
                    file,
                    "    mov eax, %s\n",
                    instruction->operand1
                );

            } else {

                fprintf(
                    file,
                    "    mov eax, DWORD PTR [%s]\n",
                    instruction->operand1
                );
            }


            /*
             * Store result.
             */
            fprintf(
                file,
                "    mov DWORD PTR [%s], eax\n",
                instruction->result
            );

            continue;
        }


        /*
         * =====================================
         * BINARY OPERATION
         * =====================================
         *
         * Example:
         *
         * t1 = b * c
         */
        if (
            instruction->type ==
            IR_BINARY
        ) {

            fprintf(
                file,
                "    # %s = %s %s %s\n",
                instruction->result,
                instruction->operand1,
                instruction->operator,
                instruction->operand2
            );


            /*
             * Load first operand.
             */
            loadOperand(
                file,
                instruction->operand1
            );


            /*
             * Addition.
             */
            if (
                instruction->operator[0] ==
                '+'
            ) {

                if (
                    isNumber(
                        instruction->operand2
                    )
                ) {

                    fprintf(
                        file,
                        "    add eax, %s\n",
                        instruction->operand2
                    );

                } else {

                    fprintf(
                        file,
                        "    add eax, DWORD PTR [%s]\n",
                        instruction->operand2
                    );
                }
            }


            /*
             * Subtraction.
             */
            else if (
                instruction->operator[0] ==
                '-'
            ) {

                if (
                    isNumber(
                        instruction->operand2
                    )
                ) {

                    fprintf(
                        file,
                        "    sub eax, %s\n",
                        instruction->operand2
                    );

                } else {

                    fprintf(
                        file,
                        "    sub eax, DWORD PTR [%s]\n",
                        instruction->operand2
                    );
                }
            }


            /*
             * Multiplication.
             */
            else if (
                instruction->operator[0] ==
                '*'
            ) {

                if (
                    isNumber(
                        instruction->operand2
                    )
                ) {

                    fprintf(
                        file,
                        "    imul eax, %s\n",
                        instruction->operand2
                    );

                } else {

                    fprintf(
                        file,
                        "    imul eax, DWORD PTR [%s]\n",
                        instruction->operand2
                    );
                }
            }


            /*
             * Division.
             */
            else if (
                instruction->operator[0] ==
                '/'
            ) {

                /*
                 * Load divisor into ECX.
                 */
                if (
                    isNumber(
                        instruction->operand2
                    )
                ) {

                    fprintf(
                        file,
                        "    mov ecx, %s\n",
                        instruction->operand2
                    );

                } else {

                    fprintf(
                        file,
                        "    mov ecx, DWORD PTR [%s]\n",
                        instruction->operand2
                    );
                }


                /*
                 * Sign extend EAX.
                 */
                fprintf(
                    file,
                    "    cdq\n"
                );


                /*
                 * Divide EDX:EAX by ECX.
                 */
                fprintf(
                    file,
                    "    idiv ecx\n"
                );
            }


            /*
             * Store result.
             */
            fprintf(
                file,
                "    mov DWORD PTR [%s], eax\n",
                instruction->result
            );
        }
    }


    /*
     * =========================================
     * PRINT FINAL RESULT
     * =========================================
     *
     * Find the last assignment.
     *
     * For our test:
     *
     * z = 50
     *
     * this finds:
     *
     * z
     */
    const char* finalVariable = NULL;


    for (
        int i = ir->count - 1;
        i >= 0;
        i--
    ) {

        if (
            ir->instructions[i].type ==
            IR_ASSIGN
        ) {

            finalVariable =
                ir->instructions[i].result;

            break;
        }
    }


    /*
     * If an assignment exists,
     * print its value.
     */
    if (
        finalVariable != NULL
    ) {

        fprintf(
            file,
            "\n    # Print final result\n"
        );


        /*
         * Push the integer value.
         */
        fprintf(
            file,
            "    push DWORD PTR [%s]\n",
            finalVariable
        );


        /*
         * Push address of format string.
         */
        fprintf(
            file,
            "    push OFFSET FLAT:output_format\n"
        );


        /*
         * Call printf.
         */
        fprintf(
            file,
            "    call _printf\n"
        );


        /*
         * Remove the two arguments
         * from the stack.
         */
        fprintf(
            file,
            "    add esp, 8\n"
        );
    }


    /*
     * =========================================
     * PROGRAM EXIT
     * =========================================
     */

    fprintf(
        file,
        "\n    mov eax, 0\n"
    );


    fprintf(
        file,
        "    ret\n"
    );


    /*
     * Close assembly file.
     */
    fclose(file);


    printf(
        "\nTarget assembly generated: %s\n",
        filename
    );
}