#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"

/*
 * Generate a complete GCC-compatible
 * x86-64 assembly program.
 */
void generateCode(
    IRProgram* ir,
    const char* filename
);

#endif