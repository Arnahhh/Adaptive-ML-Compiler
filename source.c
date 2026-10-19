#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "source.h"

static char* duplicateString(const char* text) {
    size_t length;
    char* copy;

    if (text == NULL) return NULL;

    length = strlen(text);
    copy = malloc(length + 1);
    if (copy == NULL) return NULL;

    memcpy(copy, text, length + 1);
    return copy;
}

int loadSourceFile(const char* filename, SourceFile* source) {
    FILE* file;
    long fileSize;
    size_t bytesRead;

    if (filename == NULL || source == NULL) return 0;

    memset(source, 0, sizeof(SourceFile));
    source->language = detectLanguage(filename);

    file = fopen(filename, "rb");
    if (file == NULL) return 0;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }

    fileSize = ftell(file);
    if (fileSize < 0) {
        fclose(file);
        return 0;
    }

    rewind(file);

    source->code = malloc((size_t)fileSize + 1);
    if (source->code == NULL) {
        fclose(file);
        return 0;
    }

    bytesRead = fread(source->code, 1, (size_t)fileSize, file);
    fclose(file);

    if (bytesRead != (size_t)fileSize) {
        free(source->code);
        source->code = NULL;
        return 0;
    }

    source->code[bytesRead] = '\0';
    source->length = bytesRead;
    source->filename = duplicateString(filename);

    if (source->filename == NULL) {
        free(source->code);
        source->code = NULL;
        source->length = 0;
        return 0;
    }

    return 1;
}

void freeSourceFile(SourceFile* source) {
    if (source == NULL) return;

    free(source->filename);
    free(source->code);

    source->filename = NULL;
    source->code = NULL;
    source->length = 0;
    source->language = LANGUAGE_UNKNOWN;
}
