#include <stdlib.h>
#include <string.h>
#include "symbol.h"

static char* dupString(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* out = (char*)malloc(n);
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

void initSymbolTable(SymbolTable* table) {
    if (!table) return;
    table->head = NULL;
    table->count = 0;
}

void freeSymbolTable(SymbolTable* table) {
    if (!table) return;
    Symbol* cur = table->head;
    while (cur) {
        Symbol* next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
    table->head = NULL;
    table->count = 0;
}

int symbolDeclare(SymbolTable* table, const char* name, SymbolType type, int line) {
    if (!table || !name) return 0;
    if (symbolLookup(table, name)) return 0;

    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    if (!symbol) return 0;

    symbol->name = dupString(name);
    if (!symbol->name) {
        free(symbol);
        return 0;
    }

    symbol->type = type;
    symbol->declaredLine = line;
    symbol->next = table->head;
    table->head = symbol;
    table->count++;
    return 1;
}

const Symbol* symbolLookup(const SymbolTable* table, const char* name) {
    if (!table || !name) return NULL;
    for (Symbol* cur = table->head; cur; cur = cur->next) {
        if (strcmp(cur->name, name) == 0) return cur;
    }
    return NULL;
}

const char* symbolTypeToString(SymbolType type) {
    switch (type) {
        case SYMBOL_TYPE_INT: return "int";
        case SYMBOL_TYPE_FLOAT: return "float";
        case SYMBOL_TYPE_STRING: return "string";
        case SYMBOL_TYPE_CHAR: return "char";
        default: return "unknown";
    }
}
