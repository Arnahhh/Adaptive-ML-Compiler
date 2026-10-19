#ifndef ML_H
#define ML_H

#include <stddef.h>

typedef struct {
    double loc;
    double variables;
    double loops;
    double branches;
    double arithmetic;
    double blocks;
    double complexity;
} MLFeatures;

typedef struct {
    char label[64];
    double confidence;
    double distance;
    int neighbors;
} MLPrediction;

void mlExtractFeatures(const char* source, size_t sourceLength,
                       size_t basicBlocks, MLFeatures* features);
int mlPredict(const MLFeatures* features, const char* modelPath,
              int k, MLPrediction* prediction);
void mlPrintFeatures(const MLFeatures* features);

#endif
