#include <stdio.h>
#include <string.h>
#include "adaptive.h"
#include "common_ir_optimizer.h"
#include "common_ir_cfg_optimizer.h"

static void setStrategy(AdaptiveResult* result, const char* name) {
    if (!result) return;
    snprintf(result->strategy, sizeof(result->strategy), "%s", name);
}

void adaptivePrintStrategy(const MLPrediction* prediction) {
    if (!prediction) return;
    printf("\n========== ADAPTIVE DECISION ==========\n");
    printf("  Program analyzed by ML model.\n");
    printf("  Model recommendation : %s\n", prediction->label);
    printf("  Confidence            : %.1f%%\n", prediction->confidence);
    printf("  Decision               : Automatically selecting matching transformations.\n");
}

int adaptiveOptimize(CommonIR* ir, const MLPrediction* prediction, AdaptiveResult* result) {
    if (!ir || !prediction || !result) return 0;
    memset(result, 0, sizeof(*result));

    int changes = 0;
    if (strcmp(prediction->label, "CONSTANT_FOLDING") == 0) {
        setStrategy(result, "Constant Folding + Constant Propagation");
        changes += commonIRConstantPropagation(ir);
        changes += commonIRConstantFold(ir);
        changes += commonIRConstantPropagation(ir);
        changes += commonIRConstantFold(ir);
    } else if (strcmp(prediction->label, "COPY_PROPAGATION") == 0) {
        setStrategy(result, "Copy Propagation + Safe Cleanup");
        changes += commonIRConstantPropagation(ir);
        changes += commonIRDeadCodeElimination(ir);
        changes += commonIROptimize(ir);
    } else if (strcmp(prediction->label, "CFG_OPTIMIZATION") == 0) {
        setStrategy(result, "CFG Optimization + Data-flow Friendly Cleanup");
        changes += commonIRConstantPropagation(ir);
        changes += commonIRDeadCodeElimination(ir);
        changes += commonIRCFGOptimize(ir);
    } else if (strcmp(prediction->label, "LOOP_OPTIMIZATION") == 0) {
        setStrategy(result, "Loop-aware IR + CFG Optimization");
        changes += commonIRConstantPropagation(ir);
        changes += commonIRConstantFold(ir);
        changes += commonIRCFGOptimize(ir);
    } else {
        setStrategy(result, "Balanced Safe Optimization");
        changes += commonIROptimize(ir);
        changes += commonIRCFGOptimize(ir);
    }

    result->transformations = changes;
    return changes;
}
