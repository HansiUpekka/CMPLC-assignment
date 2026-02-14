#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_SYMBOLS 100
#define MAX_OUTPUTS 100
#define MAX_CATEGORY_ITEMS 128
#define MAX_ITEM_LEN 32

/* TOKEN DEFINITIONS */

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

/* SYMBOL TABLE */

typedef struct {
    char name[32];
    int value;
} Symbol;

Symbol symtable[MAX_SYMBOLS];
int symcount = 0;
int outputs[MAX_OUTPUTS];
int output_count = 0;
char keywords[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
int keyword_count = 0;
char identifiers[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
int identifier_count = 0;
char operators[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
int operator_count = 0;
char numbers[MAX_CATEGORY_ITEMS][MAX_ITEM_LEN];
int number_count = 0;

/* GLOBALS */

FILE *src;
Token currentToken;

/* FUNCTION DECLARATIONS */

Token getNextToken();
void match(TokenType type);
void program();
void stmt_list();
void stmt();
void decl_stmt();
void print_stmt();
int expr();
int term();

int find_symbol(char *name);
void add_symbol(char *name, int value);
void syntax_error(char *msg);
void semantic_error(char *msg);
void record_token(Token token);

/* TOKEN PRINTING */

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

void record_token(Token token) {
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

void printToken(Token token) {
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

void match(TokenType type) {
    if (currentToken.type == type) {
        record_token(currentToken);
        printToken(currentToken);   // 👈 PRINT TOKEN HERE
        currentToken = getNextToken();
    } else {
        syntax_error("Unexpected token");
    }
}

/* TOKENIZER */

Token getNextToken() {
    Token token;
    int c;

    while ((c = fgetc(src)) != EOF && isspace(c));

    if (c == EOF) {
        token.type = TOKEN_EOF;
        return token;
    }

    /* Identifiers or keywords */
    /* keywords = int , print */
    /* identifiers = x , y , z */
    if (isalpha(c)) {
        int i = 0;
        token.lexeme[i++] = c;

        while (isalnum(c = fgetc(src))) {
            token.lexeme[i++] = c;
        }
        token.lexeme[i] = '\0';
        ungetc(c, src);

        if (strcmp(token.lexeme, "int") == 0)
            token.type = TOKEN_INT;
        else if (strcmp(token.lexeme, "print") == 0)
            token.type = TOKEN_PRINT;
        else
            token.type = TOKEN_ID;

        return token;
    }

    /* Numbers */
    /* numbers = 5 , 20 */
    if (isdigit(c)) {
        int i = 0;
        token.lexeme[i++] = c;

        while (isdigit(c = fgetc(src))) {
            token.lexeme[i++] = c;
        }
        token.lexeme[i] = '\0';
        ungetc(c, src);

        token.type = TOKEN_NUM;
        return token;
    }

    /* Single character tokens */
    /* operators and symbols = + , ; , = , ( , ) */
    switch (c) {
        case '=': token.type = TOKEN_ASSIGN; break;
        case '+': token.type = TOKEN_PLUS; break;
        case ';': token.type = TOKEN_SEMI; break;
        case '(': token.type = TOKEN_LPAREN; break;
        case ')': token.type = TOKEN_RPAREN; break;
        default: token.type = TOKEN_INVALID; break;
    }

    return token;
}

/* PARSER HELPERS */

/* GRAMMAR FUNCTIONS */

void program() {
    stmt_list();
    if (currentToken.type != TOKEN_EOF)
        syntax_error("Extra input after program end");
}

void stmt_list() {
    while (currentToken.type == TOKEN_INT || currentToken.type == TOKEN_PRINT) {
        stmt();
    }
}

void stmt() {
    if (currentToken.type == TOKEN_INT)
        decl_stmt();
    else if (currentToken.type == TOKEN_PRINT)
        print_stmt();
    else
        syntax_error("Invalid statement");
}

void decl_stmt() {
    char varname[32];
    int value;

    match(TOKEN_INT);

    if (currentToken.type != TOKEN_ID)
        syntax_error("Expected identifier");

    strcpy(varname, currentToken.lexeme);
    match(TOKEN_ID);

    match(TOKEN_ASSIGN);

    value = expr();

    match(TOKEN_SEMI);

    add_symbol(varname, value);
}

void print_stmt() {
    match(TOKEN_PRINT);
    match(TOKEN_LPAREN);

    if (currentToken.type != TOKEN_ID)
        syntax_error("Expected identifier in print");

    int index = find_symbol(currentToken.lexeme);
    if (index == -1)
        semantic_error("Variable not declared");

    if (output_count >= MAX_OUTPUTS)
        semantic_error("Too many print statements");
    outputs[output_count++] = symtable[index].value;

    match(TOKEN_ID);
    match(TOKEN_RPAREN);
    match(TOKEN_SEMI);
}

int expr() {
    int val = term();

    if (currentToken.type == TOKEN_PLUS) {
        match(TOKEN_PLUS);
        val += term();
    }

    return val;
}

int term() {
    int value;

    if (currentToken.type == TOKEN_NUM) {
        value = atoi(currentToken.lexeme);
        match(TOKEN_NUM);
        return value;
    }

    if (currentToken.type == TOKEN_ID) {
        int index = find_symbol(currentToken.lexeme);
        if (index == -1)
            semantic_error("Variable not declared");

        value = symtable[index].value;
        match(TOKEN_ID);
        return value;
    }

    syntax_error("Invalid term");
    return 0;
}

/* SYMBOL TABLE FUNCTIONS */

int find_symbol(char *name) {
    for (int i = 0; i < symcount; i++) {
        if (strcmp(symtable[i].name, name) == 0)
            return i;
    }
    return -1;
}

void add_symbol(char *name, int value) {
    if (find_symbol(name) != -1)
        semantic_error("Variable redeclared");

    strcpy(symtable[symcount].name, name);
    symtable[symcount].value = value;
    symcount++;
}

/* ERROR HANDLING */

void syntax_error(char *msg) {
    printf("Syntax Error: %s\n", msg);
    exit(1);
}

void semantic_error(char *msg) {
    printf("Semantic Error: %s\n", msg);
    exit(1);
}



/* MAIN FUNCTION */

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
        printf("Usage: ./parser <inputfile>\n");
        return 1;
    }

    src = fopen(argv[1], "r");
    if (!src) {
        printf("Cannot open file\n");
        return 1;
    }

    printf("TOKENS:\n");

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
