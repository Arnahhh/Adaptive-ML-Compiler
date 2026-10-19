#ifndef REPAIR_H
#define REPAIR_H

#include "error.h"


/*
 * Repair a program based on
 * the detected error and line.
 */
char* repairProgram(
    const char* source,
    ErrorType errorType,
    int errorLine
);

#endif