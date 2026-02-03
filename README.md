

---

## 🧩 What is this program, in ONE line?

👉 It **reads a file**,
👉 **understands a small language** (`int`, `print`, math expressions),
👉 **executes it**,
👉 and **prints results**.

Example input file (`test.txt`):

```txt
int a = 10 + 5;
int b = a * 2;
print(b);
```

Output:

```
30
```

You wrote a **lexer + parser + evaluator**. That’s literally **Compiler Design** fundamentals.

---

## 🧠 Big Picture Architecture (VERY IMPORTANT)

```
Source Code (.txt)
      ↓
   LEXER (Tokenizer)
      ↓
   TOKENS
      ↓
   PARSER
      ↓
  EXECUTION (values + print)
```

Keep this mental map — examiners LOVE this.

---

## 1️⃣ Grammar (CFG) – WHAT language you support

At the top:

```c
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
```

This defines:

* ✅ Variables
* ✅ Math (`+ - * /`)
* ✅ Parentheses
* ✅ Print statements

This is **recursive descent parsing** 💡

---

## 2️⃣ Tokens – Vocabulary of the language

```c
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
```

Think of tokens as **words**:

* `int` → `TOKEN_INT`
* `+` → `TOKEN_PLUS`
* `123` → `TOKEN_NUMBER`

---

## 3️⃣ Lexer – Breaking text into tokens 🧩

Key function:

```c
static Token next_token(Lexer *lexer)
```

What it does:

* Skips spaces
* Reads characters
* Groups them into tokens

Examples:

* `"int"` → `TOKEN_INT`
* `"print"` → `TOKEN_PRINT`
* `"abc"` → `TOKEN_IDENT`
* `"123"` → `TOKEN_NUMBER`

If it sees garbage → ❌ `TOKEN_INVALID`

🔥 This is **lexical analysis**

---

## 4️⃣ TokenList – Dynamic array of tokens

```c
typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;
```

Why?

* You don’t know how many tokens in advance
* So you `realloc()` dynamically

This is **classic C memory management** question material.

---

## 5️⃣ Parser – Understanding meaning 🧠

```c
typedef struct {
    TokenList list;
    size_t index;
    Symbol *symbols;
} Parser;
```

Parser does **three jobs**:

1. Checks syntax
2. Evaluates expressions
3. Manages variables (symbol table)

---

## 6️⃣ Symbol Table – Variable storage 🗃️

```c
typedef struct {
    char name[64];
    long value;
} Symbol;
```

Example:

```txt
int x = 5;
```

Stored as:

```
name = "x"
value = 5
```

If you use undeclared variable → 💥 semantic error

---

## 7️⃣ Expression Parsing (THIS IS EXAM GOLD 🏆)

### `parse_expression`

Handles `+` and `-`

### `parse_term`

Handles `*` and `/`

### `parse_factor`

Handles:

* numbers
* variables
* parentheses

Why split?
👉 **Operator precedence**

```
* /  >  + -
```

This is **recursive descent parsing**, textbook-perfect.

---

## 8️⃣ Statements

### Declaration

```c
int x = 10 + 5;
```

Handled by:

```c
parse_declaration()
```

### Print

```c
print(x);
```

Handled by:

```c
parse_print()
```

---

## 9️⃣ Error Handling (VERY clean 👌)

### Syntax errors:

```c
syntax_error(token, "Expected ';'");
```

### Semantic errors:

```c
semantic_error(token, "Use of undeclared variable");
```

This is a HUGE plus for marks.

---

## 🔚 main() – The control center

```c
int main(int argc, char *argv[])
```

Steps:

1. Read file
2. Tokenize source
3. Parse & execute
4. Free memory

Clean. Professional. No leaks. 💯

---


