#ifndef SYMBOL_H
#define SYMBOL_H

typedef enum {
    SYMBOL_TYPE_INT,
    SYMBOL_TYPE_FLOAT,
    SYMBOL_TYPE_STRING,
    SYMBOL_TYPE_CHAR,
    SYMBOL_TYPE_UNKNOWN
} SymbolType;

typedef struct Symbol {
    char* name;
    SymbolType type;
    int declaredLine;
    struct Symbol* next;
} Symbol;

typedef struct {
    Symbol* head;
    int count;
} SymbolTable;

void initSymbolTable(SymbolTable* table);
void freeSymbolTable(SymbolTable* table);
int symbolDeclare(SymbolTable* table, const char* name, SymbolType type, int line);
const Symbol* symbolLookup(const SymbolTable* table, const char* name);
const char* symbolTypeToString(SymbolType type);

#endif
