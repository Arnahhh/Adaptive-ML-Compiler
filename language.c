#include <string.h>
#include <ctype.h>
#include "language.h"

static int hasExtension(const char* filename, const char* extension) {
    size_t filenameLength;
    size_t extensionLength;

    if (filename == NULL || extension == NULL) return 0;

    filenameLength = strlen(filename);
    extensionLength = strlen(extension);

    if (filenameLength < extensionLength) return 0;

    for (size_t i = 0; i < extensionLength; i++) {
        unsigned char a = (unsigned char)filename[filenameLength - extensionLength + i];
        unsigned char b = (unsigned char)extension[i];
        if (tolower(a) != tolower(b)) return 0;
    }

    return 1;
}

Language detectLanguage(const char* filename) {
    if (hasExtension(filename, ".c")) return LANGUAGE_C;
    if (hasExtension(filename, ".cpp") ||
        hasExtension(filename, ".cc") ||
        hasExtension(filename, ".cxx") ||
        hasExtension(filename, ".hpp") ||
        hasExtension(filename, ".hxx")) return LANGUAGE_CPP;
    if (hasExtension(filename, ".py")) return LANGUAGE_PYTHON;
    if (hasExtension(filename, ".java")) return LANGUAGE_JAVA;

    return LANGUAGE_UNKNOWN;
}

const char* languageToString(Language language) {
    switch (language) {
        case LANGUAGE_C: return "C";
        case LANGUAGE_CPP: return "C++";
        case LANGUAGE_PYTHON: return "Python";
        case LANGUAGE_JAVA: return "Java";
        default: return "Unknown";
    }
}

int isSupportedByCurrentFrontend(Language language) {
    return language == LANGUAGE_C || language == LANGUAGE_CPP;
}
