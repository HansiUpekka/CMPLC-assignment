#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 100
#define MAX_OUTPUTS 100
#define MAX_CATEGORY_ITEMS 128
#define MAX_ITEM_LEN 32

/*
CFG (reference):
program      -> statement* EOF
statement    -> declaration | print_stmt

declaration  -> "int" IDENT "=" expression ";"
print_stmt   -> "print" "(" expression ")" ";"

expression   -> term ("+" term)*
term         -> NUMBER | IDENT
*/

typedef enum {
    TOKEN_INT,
    TOKEN_PRINT,
    TOKEN_ID,
    TOKEN_NUM,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_SEMI,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[32];
} Token;

typedef struct {
    char name[32];
    int value;
} Symbol;

static FILE *src;
static Token currentToken;
static Symbol symtable[MAX_SYMBOLS];
static int symcount = 0;
static int outputs[MAX_OUTPUTS];
static int output_count = 0;

static char keywords[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
static int keyword_count = 0;
static char identifiers[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
static int identifier_count = 0;
static char operators[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
static int operator_count = 0;
static char numbers[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
static int number_count = 0;

static Token getNextToken(void);
static void match(TokenType type);
static void program(void);
static void stmt_list(void);
static void stmt(void);
static void decl_stmt(void);
static void print_stmt(void);
static int expr(void);
static int term(void);

static int find_symbol(const char *name);
static void add_symbol(const char *name, int value);
static void syntax_error(const char *msg);
static void semantic_error(const char *msg);

static void add_unique(char list[][MAX_ITEM_LEN], int *count, const char *value) {
    for (int i = 0; i < *count; i++) {
        if (strcmp(list[i], value) == 0) {
            return;
        }
    }
    if (*count >= MAX_CATEGORY_ITEMS) {
        return;
    }
    strncpy(list[*count], value, MAX_ITEM_LEN - 1);
    list[*count][MAX_ITEM_LEN - 1] = '\0';
    (*count)++;
}

static void record_token(Token token) {
    switch (token.type) {
        case TOKEN_INT:
        case TOKEN_PRINT:
            add_unique(keywords, &keyword_count, token.lexeme);
            break;
        case TOKEN_ID:
            add_unique(identifiers, &identifier_count, token.lexeme);
            break;
        case TOKEN_NUM:
            add_unique(numbers, &number_count, token.lexeme);
            break;
        case TOKEN_ASSIGN:
            add_unique(operators, &operator_count, "=");
            break;
        case TOKEN_PLUS:
            add_unique(operators, &operator_count, "+");
            break;
        case TOKEN_SEMI:
            add_unique(operators, &operator_count, ";");
            break;
        case TOKEN_LPAREN:
            add_unique(operators, &operator_count, "(");
            break;
        case TOKEN_RPAREN:
            add_unique(operators, &operator_count, ")");
            break;
        default:
            break;
    }
}

static void print_token(Token token) {
    switch (token.type) {
        case TOKEN_INT:
            printf("TOKEN_INT      (%s)\n", token.lexeme);
            break;
        case TOKEN_PRINT:
            printf("TOKEN_PRINT    (%s)\n", token.lexeme);
            break;
        case TOKEN_ID:
            printf("TOKEN_ID       (%s)\n", token.lexeme);
            break;
        case TOKEN_NUM:
            printf("TOKEN_NUM      (%s)\n", token.lexeme);
            break;
        case TOKEN_ASSIGN:
            printf("TOKEN_ASSIGN   (=)\n");
            break;
        case TOKEN_PLUS:
            printf("TOKEN_PLUS     (+)\n");
            break;
        case TOKEN_SEMI:
            printf("TOKEN_SEMI     (;)\n");
            break;
        case TOKEN_LPAREN:
            printf("TOKEN_LPAREN   (()\n");
            break;
        case TOKEN_RPAREN:
            printf("TOKEN_RPAREN   ())\n");
            break;
        case TOKEN_EOF:
            printf("TOKEN_EOF\n");
            break;
        default:
            printf("TOKEN_INVALID\n");
    }
}

static void match(TokenType type) {
    if (currentToken.type == type) {
        record_token(currentToken);
        print_token(currentToken);
        currentToken = getNextToken();
    } else {
        syntax_error("Unexpected token");
    }
}

static Token getNextToken(void) {
    Token token;
    int c;

    while ((c = fgetc(src)) != EOF && isspace((unsigned char)c)) {
    }

    if (c == EOF) {
        token.type = TOKEN_EOF;
        return token;
    }

    if (isalpha((unsigned char)c)) {
        int i = 0;
        token.lexeme[i++] = (char)c;
        while ((c = fgetc(src)) != EOF && isalnum((unsigned char)c)) {
            if (i < (int)sizeof(token.lexeme) - 1) {
                token.lexeme[i++] = (char)c;
            }
        }
        token.lexeme[i] = '\0';
        if (c != EOF) {
            ungetc(c, src);
        }

        if (strcmp(token.lexeme, "int") == 0) {
            token.type = TOKEN_INT;
        } else if (strcmp(token.lexeme, "print") == 0) {
            token.type = TOKEN_PRINT;
        } else {
            token.type = TOKEN_ID;
        }
        return token;
    }

    if (isdigit((unsigned char)c)) {
        int i = 0;
        token.lexeme[i++] = (char)c;
        while ((c = fgetc(src)) != EOF && isdigit((unsigned char)c)) {
            if (i < (int)sizeof(token.lexeme) - 1) {
                token.lexeme[i++] = (char)c;
            }
        }
        token.lexeme[i] = '\0';
        if (c != EOF) {
            ungetc(c, src);
        }
        token.type = TOKEN_NUM;
        return token;
    }

    switch (c) {
        case '=': token.type = TOKEN_ASSIGN; break;
        case '+': token.type = TOKEN_PLUS; break;
        case ';': token.type = TOKEN_SEMI; break;
        case '(': token.type = TOKEN_LPAREN; break;
        case ')': token.type = TOKEN_RPAREN; break;
        default: token.type = TOKEN_INVALID; break;
    }
    token.lexeme[0] = (char)c;
    token.lexeme[1] = '\0';
    return token;
}

static void program(void) {
    stmt_list();
    if (currentToken.type != TOKEN_EOF) {
        syntax_error("Extra input after program end");
    }
}

static void stmt_list(void) {
    while (currentToken.type == TOKEN_INT || currentToken.type == TOKEN_PRINT) {
        stmt();
    }
}

static void stmt(void) {
    if (currentToken.type == TOKEN_INT) {
        decl_stmt();
    } else if (currentToken.type == TOKEN_PRINT) {
        print_stmt();
    } else {
        syntax_error("Invalid statement");
    }
}

static void decl_stmt(void) {
    char varname[32];
    int value;

    match(TOKEN_INT);

    if (currentToken.type != TOKEN_ID) {
        syntax_error("Expected identifier");
    }

    strncpy(varname, currentToken.lexeme, sizeof(varname) - 1);
    varname[sizeof(varname) - 1] = '\0';
    match(TOKEN_ID);

    match(TOKEN_ASSIGN);

    value = expr();

    match(TOKEN_SEMI);

    add_symbol(varname, value);
}

static void print_stmt(void) {
    match(TOKEN_PRINT);
    match(TOKEN_LPAREN);

    if (currentToken.type != TOKEN_ID && currentToken.type != TOKEN_NUM) {
        syntax_error("Expected identifier or number in print");
    }

    if (output_count >= MAX_OUTPUTS) {
        semantic_error("Too many print statements");
    }

    outputs[output_count++] = expr();

    match(TOKEN_RPAREN);
    match(TOKEN_SEMI);
}

static int expr(void) {
    int value = term();

    while (currentToken.type == TOKEN_PLUS) {
        match(TOKEN_PLUS);
        value += term();
    }

    return value;
}

static int term(void) {
    int value;

    if (currentToken.type == TOKEN_NUM) {
        value = atoi(currentToken.lexeme);
        match(TOKEN_NUM);
        return value;
    }

    if (currentToken.type == TOKEN_ID) {
        int index = find_symbol(currentToken.lexeme);
        if (index == -1) {
            semantic_error("Variable not declared");
        }
        value = symtable[index].value;
        match(TOKEN_ID);
        return value;
    }

    syntax_error("Invalid term");
    return 0;
}

static int find_symbol(const char *name) {
    for (int i = 0; i < symcount; i++) {
        if (strcmp(symtable[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void add_symbol(const char *name, int value) {
    if (find_symbol(name) != -1) {
        semantic_error("Variable redeclared");
    }
    if (symcount >= MAX_SYMBOLS) {
        semantic_error("Symbol table full");
    }
    strncpy(symtable[symcount].name, name, sizeof(symtable[symcount].name) - 1);
    symtable[symcount].name[sizeof(symtable[symcount].name) - 1] = '\0';
    symtable[symcount].value = value;
    symcount++;
}

static void syntax_error(const char *msg) {
    printf("Syntax Error: %s\n", msg);
    exit(1);
}

static void semantic_error(const char *msg) {
    printf("Semantic Error: %s\n", msg);
    exit(1);
}

static void print_category(const char *label, char list[][MAX_ITEM_LEN], int count) {
    printf("%s: ", label);
    if (count == 0) {
        printf("none\n");
        return;
    }
    for (int i = 0; i < count; i++) {
        printf("%s", list[i]);
        if (i + 1 < count) {
            printf(", ");
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./parser_original <inputfile>\n");
        return 1;
    }

    src = fopen(argv[1], "r");
    if (!src) {
        printf("Cannot open file\n");
        return 1;
    }

    printf("TOKENS:\n");
    printf("--------\n");

    currentToken = getNextToken();
    program();

    printf("\nCATEGORIES:\n");
    print_category("Keywords", keywords, keyword_count);
    print_category("Identifiers", identifiers, identifier_count);
    print_category("Operators", operators, operator_count);
    print_category("Numbers", numbers, number_count);

    printf("\nOUTPUT:\n");
    for (int i = 0; i < output_count; i++) {
        printf("%d\n", outputs[i]);
    }

    fclose(src);
    return 0;
}
