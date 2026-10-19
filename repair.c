#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repair.h"


/*
 * Repair a missing semicolon
 * on a specific line.
 */
static char* repairMissingSemicolon(
    const char* source,
    int errorLine
) {

    int length = strlen(source);

    /*
     * Allocate extra space for ';'
     * and '\0'.
     */
    char* repaired =
        malloc(length + 2);

    if (repaired == NULL) {

        printf(
            "Memory allocation failed.\n"
        );

        exit(1);
    }


    int sourceIndex = 0;

    int repairedIndex = 0;

    int currentLine = 1;


    while (source[sourceIndex] != '\0') {

        char character =
            source[sourceIndex];


        repaired[repairedIndex++] =
            character;


        /*
         * If we reach the newline
         * of the error line, insert ';'
         * before the newline.
         */
        if (
            character == '\n' &&
            currentLine == errorLine
        ) {

            /*
             * Check whether the line
             * already ends with ';'.
             */
            int previous =
                repairedIndex - 2;


            if (
                previous < 0 ||
                repaired[previous] != ';'
            ) {

                repaired[
                    repairedIndex - 1
                ] = ';';


                repaired[
                    repairedIndex++
                ] = '\n';
            }


            currentLine++;
        }


        else if (
            character == '\n'
        ) {

            currentLine++;
        }


        sourceIndex++;
    }


    /*
     * If the error is on the final
     * line and there was no newline,
     * append ';'.
     */
    if (
        currentLine == errorLine
    ) {

        if (
            repairedIndex == 0 ||
            repaired[
                repairedIndex - 1
            ] != ';'
        ) {

            repaired[
                repairedIndex++
            ] = ';';
        }
    }


    repaired[repairedIndex] =
        '\0';


    return repaired;
}


/*
 * Repair a missing right parenthesis.
 */
static char* repairMissingRightParenthesis(
    const char* source,
    int errorLine
) {

    int length =
        strlen(source);


    char* repaired =
        malloc(length + 2);


    if (repaired == NULL) {

        printf(
            "Memory allocation failed.\n"
        );

        exit(1);
    }


    int sourceIndex = 0;

    int repairedIndex = 0;

    int currentLine = 1;


    while (source[sourceIndex] != '\0') {

        char character =
            source[sourceIndex];


        repaired[repairedIndex++] =
            character;


        /*
         * Insert ')' before the
         * semicolon on the error line.
         */
        if (
            character == ';' &&
            currentLine == errorLine
        ) {

            /*
             * Insert ')' before ';'.
             */
            repaired[
                repairedIndex - 1
            ] = ')';


            repaired[
                repairedIndex++
            ] = ';';
        }


        if (
            character == '\n'
        ) {

            currentLine++;
        }


        sourceIndex++;
    }


    /*
     * If no semicolon was found on
     * the error line, append ')'.
     */
    if (
        currentLine == errorLine
    ) {

        repaired[
            repairedIndex++
        ] = ')';
    }


    repaired[
        repairedIndex
    ] = '\0';


    return repaired;
}


/*
 * Main repair function.
 */
char* repairProgram(
    const char* source,
    ErrorType errorType,
    int errorLine
) {

    switch (errorType) {


        /*
         * Missing semicolon.
         */
        case ERROR_MISSING_SEMICOLON:

            return repairMissingSemicolon(
                source,
                errorLine
            );


        /*
         * Missing right parenthesis.
         */
        case ERROR_MISSING_RPAREN:

            return repairMissingRightParenthesis(
                source,
                errorLine
            );


        /*
         * Other errors currently
         * do not have automatic repair.
         */
        default:

            return NULL;
    }
}