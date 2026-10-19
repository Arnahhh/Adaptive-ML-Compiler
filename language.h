#ifndef LANGUAGE_H
#define LANGUAGE_H

typedef enum {
    LANGUAGE_C,
    LANGUAGE_CPP,
    LANGUAGE_PYTHON,
    LANGUAGE_JAVA,
    LANGUAGE_UNKNOWN
} Language;

Language detectLanguage(const char* filename);
const char* languageToString(Language language);
int isSupportedByCurrentFrontend(Language language);

#endif
