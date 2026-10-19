#ifndef SOURCE_H
#define SOURCE_H

#include "language.h"

typedef struct {
    char* filename;
    char* code;
    size_t length;
    Language language;
} SourceFile;

int loadSourceFile(const char* filename, SourceFile* source);
void freeSourceFile(SourceFile* source);

#endif
