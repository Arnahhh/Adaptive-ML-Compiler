#ifndef ADAPTIVE_H
#define ADAPTIVE_H

#include "common_ir.h"
#include "ml.h"

typedef struct {
    char strategy[64];
    int transformations;
} AdaptiveResult;

int adaptiveOptimize(CommonIR* ir, const MLPrediction* prediction, AdaptiveResult* result);
void adaptivePrintStrategy(const MLPrediction* prediction);

#endif
