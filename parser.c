#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
CFG (for reference):

program      -> statement* EOF
statement    -> declaration | print_stmt

declaration  -> "int" IDENT "=" expression ";"
print_stmt   -> "print" "(" expression ")" ";"

expression   -> term (("+" | "-") term)*
term         -> factor (("*" | "/") factor)*
factor       -> NUMBER | IDENT | "(" expression ")"
*/

typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_PRINT,
    TOKEN_IDENT,
    TOKEN_NUMBER,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[64];
    long value;
    int line;
    int column;
} Token;

typedef struct {
    const char *src;
    size_t pos;
    int line;
    int column;
} Lexer;

typedef struct {
    char name[64];
    long value;
} Symbol;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;

typedef struct {
    TokenList list;
    size_t index;
    Symbol *symbols;
    size_t symbol_count;
    size_t symbol_capacity;
} Parser;

static void die(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

static void syntax_error(const Token *token, const char *message) {
    fprintf(stderr, "Syntax error at line %d, column %d: %s", token->line, token->column, message);
    if (token->type != TOKEN_EOF) {
        fprintf(stderr, " (found '%s')", token->lexeme);
    }
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void semantic_error(const Token *token, const char *message) {
    fprintf(stderr, "Semantic error at line %d, column %d: %s", token->line, token->column, message);
    if (token->type == TOKEN_IDENT) {
        fprintf(stderr, " ('%s')", token->lexeme);
    }
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void init_lexer(Lexer *lexer, const char *src) {
    lexer->src = src;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static char peek_char(const Lexer *lexer) {
    return lexer->src[lexer->pos];
}

static char advance_char(Lexer *lexer) {
    char c = lexer->src[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace(Lexer *lexer) {
    while (isspace((unsigned char)peek_char(lexer))) {
        advance_char(lexer);
    }
}

static Token make_token(TokenType type, const char *lexeme, long value, int line, int column) {
    Token token;
    token.type = type;
    strncpy(token.lexeme, lexeme, sizeof(token.lexeme) - 1);
    token.lexeme[sizeof(token.lexeme) - 1] = '\0';
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}

static bool is_identifier_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool is_identifier_part(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static Token next_token(Lexer *lexer) {
    skip_whitespace(lexer);

    int line = lexer->line;
    int column = lexer->column;
    char c = peek_char(lexer);

    if (c == '\0') {
        return make_token(TOKEN_EOF, "<eof>", 0, line, column);
    }

    if (is_identifier_start(c)) {
        char buffer[64];
        size_t len = 0;
        while (is_identifier_part(peek_char(lexer)) && len < sizeof(buffer) - 1) {
            buffer[len++] = advance_char(lexer);
        }
        buffer[len] = '\0';

        if (strcmp(buffer, "int") == 0) {
            return make_token(TOKEN_INT, buffer, 0, line, column);
        }
        if (strcmp(buffer, "print") == 0) {
            return make_token(TOKEN_PRINT, buffer, 0, line, column);
        }
        return make_token(TOKEN_IDENT, buffer, 0, line, column);
    }

    if (isdigit((unsigned char)c)) {
        char buffer[64];
        size_t len = 0;
        while (isdigit((unsigned char)peek_char(lexer)) && len < sizeof(buffer) - 1) {
            buffer[len++] = advance_char(lexer);
        }
        buffer[len] = '\0';
        errno = 0;
        long value = strtol(buffer, NULL, 10);
        if (errno != 0) {
            return make_token(TOKEN_INVALID, buffer, 0, line, column);
        }
        return make_token(TOKEN_NUMBER, buffer, value, line, column);
    }

    advance_char(lexer);
    switch (c) {
        case '=': return make_token(TOKEN_ASSIGN, "=", 0, line, column);
        case '+': return make_token(TOKEN_PLUS, "+", 0, line, column);
        case '-': return make_token(TOKEN_MINUS, "-", 0, line, column);
        case '*': return make_token(TOKEN_STAR, "*", 0, line, column);
        case '/': return make_token(TOKEN_SLASH, "/", 0, line, column);
        case ';': return make_token(TOKEN_SEMICOLON, ";", 0, line, column);
        case '(': return make_token(TOKEN_LPAREN, "(", 0, line, column);
        case ')': return make_token(TOKEN_RPAREN, ")", 0, line, column);
        default: {
            char buffer[2] = {c, '\0'};
            return make_token(TOKEN_INVALID, buffer, 0, line, column);
        }
    }
}

static void token_list_init(TokenList *list) {
    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void token_list_push(TokenList *list, Token token) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        Token *new_tokens = realloc(list->tokens, new_capacity * sizeof(Token));
        if (!new_tokens) {
            die("Out of memory");
        }
        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }
    list->tokens[list->count++] = token;
}

static void tokenize_source(TokenList *list, const char *src) {
    Lexer lexer;
    init_lexer(&lexer, src);

    while (true) {
        Token token = next_token(&lexer);
        token_list_push(list, token);
        if (token.type == TOKEN_EOF) {
            break;
        }
        if (token.type == TOKEN_INVALID) {
            syntax_error(&token, "Invalid token");
        }
    }
}

static void parser_init(Parser *parser, TokenList *list) {
    parser->list = *list;
    parser->index = 0;
    parser->symbols = NULL;
    parser->symbol_count = 0;
    parser->symbol_capacity = 0;
}

static Token *current_token(Parser *parser) {
    return &parser->list.tokens[parser->index];
}

static Token *advance_token(Parser *parser) {
    if (parser->index < parser->list.count - 1) {
        parser->index++;
    }
    return current_token(parser);
}

static bool match(Parser *parser, TokenType type) {
    if (current_token(parser)->type == type) {
        advance_token(parser);
        return true;
    }
    return false;
}

static void expect(Parser *parser, TokenType type, const char *message) {
    if (!match(parser, type)) {
        syntax_error(current_token(parser), message);
    }
}

static Symbol *find_symbol(Parser *parser, const char *name) {
    for (size_t i = 0; i < parser->symbol_count; i++) {
        if (strcmp(parser->symbols[i].name, name) == 0) {
            return &parser->symbols[i];
        }
    }
    return NULL;
}

static void set_symbol(Parser *parser, const char *name, long value) {
    Symbol *symbol = find_symbol(parser, name);
    if (symbol) {
        symbol->value = value;
        return;
    }

    if (parser->symbol_count == parser->symbol_capacity) {
        size_t new_capacity = parser->symbol_capacity == 0 ? 16 : parser->symbol_capacity * 2;
        Symbol *new_symbols = realloc(parser->symbols, new_capacity * sizeof(Symbol));
        if (!new_symbols) {
            die("Out of memory");
        }
        parser->symbols = new_symbols;
        parser->symbol_capacity = new_capacity;
    }

    Symbol *new_symbol = &parser->symbols[parser->symbol_count++];
    strncpy(new_symbol->name, name, sizeof(new_symbol->name) - 1);
    new_symbol->name[sizeof(new_symbol->name) - 1] = '\0';
    new_symbol->value = value;
}

static long parse_expression(Parser *parser);

static long parse_factor(Parser *parser) {
    Token *token = current_token(parser);
    if (match(parser, TOKEN_NUMBER)) {
        return token->value;
    }

    if (match(parser, TOKEN_IDENT)) {
        Symbol *symbol = find_symbol(parser, token->lexeme);
        if (!symbol) {
            semantic_error(token, "Use of undeclared variable");
        }
        return symbol->value;
    }

    if (match(parser, TOKEN_LPAREN)) {
        long value = parse_expression(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return value;
    }

    syntax_error(token, "Expected number, identifier, or '('" );
    return 0;
}

static long parse_term(Parser *parser) {
    long value = parse_factor(parser);
    while (true) {
        Token *token = current_token(parser);
        if (match(parser, TOKEN_STAR)) {
            value *= parse_factor(parser);
        } else if (match(parser, TOKEN_SLASH)) {
            long divisor = parse_factor(parser);
            if (divisor == 0) {
                semantic_error(token, "Division by zero");
            }
            value /= divisor;
        } else {
            break;
        }
    }
    return value;
}

static long parse_expression(Parser *parser) {
    long value = parse_term(parser);
    while (true) {
        if (match(parser, TOKEN_PLUS)) {
            value += parse_term(parser);
        } else if (match(parser, TOKEN_MINUS)) {
            value -= parse_term(parser);
        } else {
            break;
        }
    }
    return value;
}

static void parse_declaration(Parser *parser) {
    expect(parser, TOKEN_INT, "Expected 'int' keyword");

    Token *identifier = current_token(parser);
    expect(parser, TOKEN_IDENT, "Expected identifier after 'int'");
    expect(parser, TOKEN_ASSIGN, "Expected '=' after identifier");

    long value = parse_expression(parser);
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after declaration");

    set_symbol(parser, identifier->lexeme, value);
}

static void parse_print(Parser *parser) {
    expect(parser, TOKEN_PRINT, "Expected 'print' keyword");
    expect(parser, TOKEN_LPAREN, "Expected '(' after 'print'");
    long value = parse_expression(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' after expression");
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after print statement");

    printf("%ld\n", value);
}

static void parse_statement(Parser *parser) {
    Token *token = current_token(parser);
    if (token->type == TOKEN_INT) {
        parse_declaration(parser);
        return;
    }
    if (token->type == TOKEN_PRINT) {
        parse_print(parser);
        return;
    }

    syntax_error(token, "Expected 'int' declaration or 'print' statement");
}

static void parse_program(Parser *parser) {
    while (current_token(parser)->type != TOKEN_EOF) {
        parse_statement(parser);
    }
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s'\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        die("Failed to read file size");
    }
    rewind(file);

    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        die("Out of memory");
    }

    size_t read_bytes = fread(buffer, 1, (size_t)size, file);
    buffer[read_bytes] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *source = read_file(argv[1]);

    TokenList list;
    token_list_init(&list);
    tokenize_source(&list, source);

    Parser parser;
    parser_init(&parser, &list);
    parse_program(&parser);

    free(parser.symbols);
    free(list.tokens);
    free(source);

    return EXIT_SUCCESS;
}
