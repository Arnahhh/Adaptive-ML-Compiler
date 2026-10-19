#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "ml.h"

typedef struct {
    double f[7];
    char label[64];
} Sample;

static int isWordChar(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static int wordEquals(const char* start, size_t len, const char* word) {
    size_t wlen = strlen(word);
    return len == wlen && strncmp(start, word, wlen) == 0;
}

static int isTypeWord(const char* start, size_t len) {
    return wordEquals(start,len,"int") || wordEquals(start,len,"float") ||
           wordEquals(start,len,"double") || wordEquals(start,len,"char") ||
           wordEquals(start,len,"long") || wordEquals(start,len,"short") ||
           wordEquals(start,len,"boolean") || wordEquals(start,len,"String") ||
           wordEquals(start,len,"bool") || wordEquals(start,len,"void");
}

void mlExtractFeatures(const char* source, size_t sourceLength,
                       size_t basicBlocks, MLFeatures* features) {
    if (!features) return;
    memset(features, 0, sizeof(*features));
    if (!source) return;

    features->loc = 0;
    for (size_t i = 0; i < sourceLength; ++i) {
        if (source[i] == '\n') features->loc += 1;
    }
    if (sourceLength > 0 && source[sourceLength - 1] != '\n') features->loc += 1;
    if (features->loc < 1 && sourceLength > 0) features->loc = 1;

    int inString = 0;
    int inChar = 0;
    int escape = 0;
    for (size_t i = 0; i < sourceLength; ) {
        char c = source[i];
        if (escape) { escape = 0; ++i; continue; }
        if ((inString || inChar) && c == '\\') { escape = 1; ++i; continue; }
        if (!inChar && c == '"') { inString = !inString; ++i; continue; }
        if (!inString && c == '\'') { inChar = !inChar; ++i; continue; }
        if (inString || inChar) { ++i; continue; }

        if (isWordChar(c) && !isdigit((unsigned char)c)) {
            size_t j = i + 1;
            while (j < sourceLength && isWordChar(source[j])) ++j;
            size_t len = j - i;
            if (wordEquals(source+i,len,"for") || wordEquals(source+i,len,"while")) features->loops += 1;
            if (wordEquals(source+i,len,"if") || wordEquals(source+i,len,"else") ||
                wordEquals(source+i,len,"switch") || wordEquals(source+i,len,"case") ||
                wordEquals(source+i,len,"catch")) features->branches += 1;
            if (isTypeWord(source+i,len)) {
                size_t k = j;
                while (k < sourceLength && isspace((unsigned char)source[k])) ++k;
                if (k < sourceLength && isWordChar(source[k])) {
                    size_t m = k + 1;
                    while (m < sourceLength && isWordChar(source[m])) ++m;
                    if (!wordEquals(source+k,m-k,"main")) features->variables += 1;
                }
            }
            i = j;
            continue;
        }

        if (strchr("+-*/%", c) != NULL) {
            if (c == '+' || c == '-') {
                if (i + 1 < sourceLength && source[i+1] == c) {
                    i += 2;
                    continue;
                }
            }
            features->arithmetic += 1;
        }
        ++i;
    }

    features->blocks = basicBlocks > 0 ? (double)basicBlocks : 1.0;
    features->complexity = 1.0 + features->loops + features->branches;
}

static int loadSamples(const char* path, Sample** out, size_t* count) {
    FILE* fp = fopen(path, "r");
    if (!fp) return 0;
    size_t cap = 32, n = 0;
    Sample* samples = malloc(cap * sizeof(Sample));
    if (!samples) { fclose(fp); return 0; }
    char line[512];
    if (!fgets(line, sizeof(line), fp)) { free(samples); fclose(fp); return 0; }
    while (fgets(line, sizeof(line), fp)) {
        Sample s;
        char label[64];
        if (sscanf(line, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%63[^\n]",
                   &s.f[0], &s.f[1], &s.f[2], &s.f[3], &s.f[4], &s.f[5], &s.f[6], label) != 8) continue;
        snprintf(s.label, sizeof(s.label), "%s", label);
        if (n == cap) {
            cap *= 2;
            Sample* temp = realloc(samples, cap * sizeof(Sample));
            if (!temp) { free(samples); fclose(fp); return 0; }
            samples = temp;
        }
        samples[n++] = s;
    }
    fclose(fp);
    *out = samples;
    *count = n;
    return n > 0;
}

static double featureScale(const MLFeatures* f, int i) {
    const double maxv[7] = {100.0, 30.0, 5.0, 10.0, 20.0, 30.0, 15.0};
    double v[7] = {f->loc,f->variables,f->loops,f->branches,f->arithmetic,f->blocks,f->complexity};
    return v[i] / maxv[i];
}

int mlPredict(const MLFeatures* features, const char* modelPath,
              int k, MLPrediction* prediction) {
    if (!features || !modelPath || !prediction || k < 1) return 0;
    Sample* samples = NULL;
    size_t count = 0;
    if (!loadSamples(modelPath, &samples, &count)) return 0;

    if ((size_t)k > count) k = (int)count;
    double* distances = malloc(count * sizeof(double));
    int* indices = malloc(count * sizeof(int));
    if (!distances || !indices) { free(distances); free(indices); free(samples); return 0; }

    double target[7];
    for (int j = 0; j < 7; ++j) target[j] = featureScale(features, j);

    for (size_t i = 0; i < count; ++i) {
        double d = 0.0;
        for (int j = 0; j < 7; ++j) {
            const double maxv[7] = {100.0,30.0,5.0,10.0,20.0,30.0,15.0};
            double sample = samples[i].f[j] / maxv[j];
            double diff = target[j] - sample;
            d += diff * diff;
        }
        distances[i] = sqrt(d);
        indices[i] = (int)i;
    }

    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            if (distances[j] < distances[i]) {
                double td = distances[i]; distances[i] = distances[j]; distances[j] = td;
                int ti = indices[i]; indices[i] = indices[j]; indices[j] = ti;
            }
        }
    }

    typedef struct { char label[64]; int votes; double weight; } Vote;
    Vote votes[32]; int voteCount = 0;
    double totalWeight = 0.0;
    for (int i = 0; i < k; ++i) {
        int idx = indices[i];
        double weight = 1.0 / (distances[i] + 0.05);
        totalWeight += weight;
        int found = -1;
        for (int j = 0; j < voteCount; ++j) {
            if (strcmp(votes[j].label, samples[idx].label) == 0) { found = j; break; }
        }
        if (found < 0) {
            snprintf(votes[voteCount].label, sizeof(votes[voteCount].label), "%s", samples[idx].label);
            votes[voteCount].votes = 1;
            votes[voteCount].weight = weight;
            voteCount++;
        } else {
            votes[found].votes++;
            votes[found].weight += weight;
        }
    }

    int best = 0;
    for (int i = 1; i < voteCount; ++i) {
        if (votes[i].weight > votes[best].weight) best = i;
    }

    strncpy(prediction->label, votes[best].label, sizeof(prediction->label) - 1);
    prediction->label[sizeof(prediction->label) - 1] = '\0';
    prediction->confidence = totalWeight > 0 ? (votes[best].weight / totalWeight) * 100.0 : 0.0;
    prediction->distance = distances[0];
    prediction->neighbors = k;

    free(distances);
    free(indices);
    free(samples);
    return 1;
}

void mlPrintFeatures(const MLFeatures* f) {
    printf("  Lines / LOC        : %.0f\n", f->loc);
    printf("  Variables          : %.0f\n", f->variables);
    printf("  Loops              : %.0f\n", f->loops);
    printf("  Branches           : %.0f\n", f->branches);
    printf("  Arithmetic Ops     : %.0f\n", f->arithmetic);
    printf("  Basic Blocks       : %.0f\n", f->blocks);
    printf("  Complexity         : %.0f\n", f->complexity);
}
