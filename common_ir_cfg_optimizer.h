#ifndef COMMON_IR_CFG_OPTIMIZER_H
#define COMMON_IR_CFG_OPTIMIZER_H

#include "common_ir.h"

/* CFG-aware cleanup performed after local Common IR optimization. */
int commonIRCFGOptimize(CommonIR* ir);

#endif
