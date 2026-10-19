#ifndef COMMON_IR_OPTIMIZER_H
#define COMMON_IR_OPTIMIZER_H

#include "common_ir.h"

/* Returns the number of transformations performed. */
int commonIRConstantFold(CommonIR* ir);
int commonIRConstantPropagation(CommonIR* ir);
int commonIRDeadCodeElimination(CommonIR* ir);

/* Run the safe optimization pipeline on Common IR. */
int commonIROptimize(CommonIR* ir);

#endif
