#include <ctype.h>
#include <string.h>

#include "lexer.h"

static int lexerLine = 1;
static int lexerColumn = 1;

void resetLexer(void) {
    lexerLine = 1;
    lexerColumn = 1;
}

static void setValue(Token* token, const char* value) {
    strncpy(token->value, value, MAX_TOKEN_LENGTH - 1);
    token->value[MAX_TOKEN_LENGTH - 1] = '\0';
}

static void addChar(Token* token, int* length, char c) {
    if (*length < MAX_TOKEN_LENGTH - 1) {
        token->value[*length] = c;
        (*length)++;
        token->value[*length] = '\0';
    }
}

static TokenType keywordType(const char* word) {
    struct Keyword { const char* word; TokenType type; };
    static const struct Keyword keywords[] = {
        {"int", TOKEN_INT}, {"float", TOKEN_FLOAT}, {"double", TOKEN_DOUBLE},
        {"char", TOKEN_CHAR}, {"void", TOKEN_VOID}, {"bool", TOKEN_BOOL},
        {"string", TOKEN_STRING_KW}, {"String", TOKEN_STRING_KW},
        {"if", TOKEN_IF}, {"else", TOKEN_ELSE}, {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR}, {"return", TOKEN_RETURN}, {"break", TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE}, {"class", TOKEN_CLASS},
        {"public", TOKEN_PUBLIC}, {"private", TOKEN_PRIVATE},
        {"protected", TOKEN_PROTECTED}, {"static", TOKEN_STATIC},
        {"new", TOKEN_NEW}, {"true", TOKEN_TRUE}, {"false", TOKEN_FALSE}
    };

    size_t count = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(word, keywords[i].word) == 0)
            return keywords[i].type;
    }
    return TOKEN_IDENTIFIER;
}

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_UNKNOWN: return "UNKNOWN";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_CHAR_LITERAL: return "CHAR_LITERAL";
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_DOUBLE: return "DOUBLE";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_VOID: return "VOID";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_STRING_KW: return "STRING";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_CLASS: return "CLASS";
        case TOKEN_PUBLIC: return "PUBLIC";
        case TOKEN_PRIVATE: return "PRIVATE";
        case TOKEN_PROTECTED: return "PROTECTED";
        case TOKEN_STATIC: return "STATIC";
        case TOKEN_NEW: return "NEW";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_MULTIPLY: return "MULTIPLY";
        case TOKEN_DIVIDE: return "DIVIDE";
        case TOKEN_MODULO: return "MODULO";
        case TOKEN_INCREMENT: return "INCREMENT";
        case TOKEN_DECREMENT: return "DECREMENT";
        case TOKEN_LOGICAL_AND: return "LOGICAL_AND";
        case TOKEN_LOGICAL_OR: return "LOGICAL_OR";
        case TOKEN_LOGICAL_NOT: return "LOGICAL_NOT";
        case TOKEN_BITWISE_AND: return "BITWISE_AND";
        case TOKEN_BITWISE_OR: return "BITWISE_OR";
        case TOKEN_LPAREN: return "LEFT_PAREN";
        case TOKEN_RPAREN: return "RIGHT_PAREN";
        case TOKEN_LBRACE: return "LEFT_BRACE";
        case TOKEN_RBRACE: return "RIGHT_BRACE";
        case TOKEN_LBRACKET: return "LEFT_BRACKET";
        case TOKEN_RBRACKET: return "RIGHT_BRACKET";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_COLON: return "COLON";
        default: return "UNKNOWN";
    }
}

static void advanceChar(const char* source, int* position) {
    char c = source[*position];
    if (c == '\n') {
        lexerLine++;
        lexerColumn = 1;
    } else {
        lexerColumn++;
    }
    (*position)++;
}

static int peek(const char* source, int position, char expected) {
    return source[position] == expected;
}

Token getNextToken(const char* source, int* position) {
    Token token;
    token.type = TOKEN_UNKNOWN;
    token.value[0] = '\0';
    token.line = lexerLine;
    token.column = lexerColumn;

    while (source[*position] != '\0') {
        char c = source[*position];

        if (isspace((unsigned char)c)) {
            advanceChar(source, position);
            continue;
        }

        /* C/C++ and Java comments. */
        if (c == '/' && source[*position + 1] == '/') {
            while (source[*position] != '\0' && source[*position] != '\n')
                advanceChar(source, position);
            continue;
        }

        if (c == '/' && source[*position + 1] == '*') {
            advanceChar(source, position);
            advanceChar(source, position);
            while (source[*position] != '\0') {
                if (source[*position] == '*' && source[*position + 1] == '/') {
                    advanceChar(source, position);
                    advanceChar(source, position);
                    break;
                }
                advanceChar(source, position);
            }
            continue;
        }

        break;
    }

    token.line = lexerLine;
    token.column = lexerColumn;

    if (source[*position] == '\0') {
        token.type = TOKEN_EOF;
        setValue(&token, "EOF");
        return token;
    }

    char current = source[*position];

    if (isalpha((unsigned char)current) || current == '_') {
        int length = 0;
        while (isalnum((unsigned char)source[*position]) || source[*position] == '_') {
            addChar(&token, &length, source[*position]);
            advanceChar(source, position);
        }
        token.type = keywordType(token.value);
        return token;
    }

    if (isdigit((unsigned char)current)) {
        int length = 0;
        int isFloat = 0;
        while (isdigit((unsigned char)source[*position])) {
            addChar(&token, &length, source[*position]);
            advanceChar(source, position);
        }
        if (source[*position] == '.' && isdigit((unsigned char)source[*position + 1])) {
            isFloat = 1;
            addChar(&token, &length, source[*position]);
            advanceChar(source, position);
            while (isdigit((unsigned char)source[*position])) {
                addChar(&token, &length, source[*position]);
                advanceChar(source, position);
            }
        }
        token.type = isFloat ? TOKEN_FLOAT_LITERAL : TOKEN_NUMBER;
        return token;
    }

    if (current == '"' || current == '\'') {
        char quote = current;
        int length = 0;
        TokenType type = (quote == '"') ? TOKEN_STRING_LITERAL : TOKEN_CHAR_LITERAL;
        addChar(&token, &length, current);
        advanceChar(source, position);

        while (source[*position] != '\0' && source[*position] != quote) {
            if (source[*position] == '\\' && source[*position + 1] != '\0') {
                addChar(&token, &length, source[*position]);
                advanceChar(source, position);
                addChar(&token, &length, source[*position]);
                advanceChar(source, position);
            } else {
                if (source[*position] == '\n') {
                    token.type = TOKEN_UNKNOWN;
                    return token;
                }
                addChar(&token, &length, source[*position]);
                advanceChar(source, position);
            }
        }

        if (source[*position] == quote) {
            addChar(&token, &length, quote);
            advanceChar(source, position);
            token.type = type;
        } else {
            token.type = TOKEN_UNKNOWN;
        }
        return token;
    }

    /* Two-character operators. */
    struct Pair { char first; char second; TokenType type; const char* text; };
    static const struct Pair pairs[] = {
        {'=', '=', TOKEN_EQUAL, "=="}, {'!', '=', TOKEN_NOT_EQUAL, "!="},
        {'<', '=', TOKEN_LESS_EQUAL, "<="}, {'>', '=', TOKEN_GREATER_EQUAL, ">="},
        {'+', '+', TOKEN_INCREMENT, "++"}, {'-', '-', TOKEN_DECREMENT, "--"},
        {'&', '&', TOKEN_LOGICAL_AND, "&&"}, {'|', '|', TOKEN_LOGICAL_OR, "||"}
    };

    size_t pairCount = sizeof(pairs) / sizeof(pairs[0]);
    for (size_t i = 0; i < pairCount; ++i) {
        if (current == pairs[i].first && peek(source, *position + 1, pairs[i].second)) {
            token.type = pairs[i].type;
            setValue(&token, pairs[i].text);
            advanceChar(source, position);
            advanceChar(source, position);
            return token;
        }
    }

    advanceChar(source, position);
    switch (current) {
        case '=': token.type = TOKEN_ASSIGN; setValue(&token, "="); break;
        case '+': token.type = TOKEN_PLUS; setValue(&token, "+"); break;
        case '-': token.type = TOKEN_MINUS; setValue(&token, "-"); break;
        case '*': token.type = TOKEN_MULTIPLY; setValue(&token, "*"); break;
        case '/': token.type = TOKEN_DIVIDE; setValue(&token, "/"); break;
        case '%': token.type = TOKEN_MODULO; setValue(&token, "%"); break;
        case '!': token.type = TOKEN_LOGICAL_NOT; setValue(&token, "!"); break;
        case '&': token.type = TOKEN_BITWISE_AND; setValue(&token, "&"); break;
        case '|': token.type = TOKEN_BITWISE_OR; setValue(&token, "|"); break;
        case '<': token.type = TOKEN_LESS; setValue(&token, "<"); break;
        case '>': token.type = TOKEN_GREATER; setValue(&token, ">"); break;
        case '(': token.type = TOKEN_LPAREN; setValue(&token, "("); break;
        case ')': token.type = TOKEN_RPAREN; setValue(&token, ")"); break;
        case '{': token.type = TOKEN_LBRACE; setValue(&token, "{"); break;
        case '}': token.type = TOKEN_RBRACE; setValue(&token, "}"); break;
        case '[': token.type = TOKEN_LBRACKET; setValue(&token, "["); break;
        case ']': token.type = TOKEN_RBRACKET; setValue(&token, "]"); break;
        case ';': token.type = TOKEN_SEMICOLON; setValue(&token, ";"); break;
        case ',': token.type = TOKEN_COMMA; setValue(&token, ","); break;
        case '.': token.type = TOKEN_DOT; setValue(&token, "."); break;
        case ':': token.type = TOKEN_COLON; setValue(&token, ":"); break;
        default:
            token.type = TOKEN_UNKNOWN;
            token.value[0] = current;
            token.value[1] = '\0';
            break;
    }
    return token;
}
