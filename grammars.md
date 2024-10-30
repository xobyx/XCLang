# Language Grammar

## Lexical Elements

### Keywords
- `if`
- `else` 
- `while`
- `return`
- `print`
- `int`
- `float` 
- `string`

### Types
- Integer literals (e.g., `123`)
- String literals (e.g., `"hello"`)
- Arrays (e.g., `int[5]`)

### Operators
- Arithmetic: `+`, `-`, `*`, `/`
- Comparison: `<`, `>`, `<=`, `>=`, `==`
- Assignment: `=`

### Delimiters
- Parentheses: `(`, `)`
- Braces: `{`, `}`
- Brackets: `[`, `]`
- Comma: `,`
- Semicolon: `;`

## Grammar Rules

### Program
```
program ::= {declaration | statement}*
```

### Declarations
```
declaration ::= type_specifier identifier array_declaration
              | type_specifier identifier '=' expression
              | function_declaration

type_specifier ::= 'int' | 'float' | 'string'

array_declaration ::= '[' integer_literal ']'

function_declaration ::= type_specifier identifier '(' parameter_list ')' block

parameter_list ::= ε 
                | type_specifier identifier 
                | type_specifier identifier ',' parameter_list
```

### Statements
```
statement ::= if_statement
            | while_statement
            | return_statement
            | print_statement
            | expression_statement
            | block

if_statement ::= 'if' '(' expression ')' block ['else' block]

while_statement ::= 'while' '(' expression ')' block

return_statement ::= 'return' expression

print_statement ::= 'print' expression

expression_statement ::= expression

block ::= '{' {statement}* '}'
```

### Expressions
```
expression ::= assignment_expression
            | arithmetic_expression
            | comparison_expression
            | function_call
            | array_access

assignment_expression ::= identifier '=' expression
                      | array_access '=' expression

arithmetic_expression ::= term {('+' | '-') term}*

term ::= factor {('*' | '/') factor}*

factor ::= integer_literal
         | identifier
         | array_access
         | function_call
         | '(' expression ')'

array_access ::= identifier '[' expression ']'

function_call ::= identifier '(' argument_list ')'

argument_list ::= ε 
                | expression 
                | expression ',' argument_list
```

## Built-in Functions
```
native_functions ::= 'sqrt' '(' expression ')'
                  | 'pow' '(' expression ',' expression ')'
```

## Type System
- Static typing
- Basic types: int, float, string
- Array types: int[], float[], string[]
- Function types with return type and parameter types

## Scoping Rules
- Block-level scoping
- Function parameters have local scope
- Variables must be declared before use

## Examples

1. Function Definition and Call:
```
int add(int x, int y) {
    return x + y;
}
print add(2, 3);
```

2. Array Declaration and Use:
```
int arr[5];
arr[0] = 1;
print arr[0];
```

3. Control Flow:
```
if (x < 5) {
    print x;
} else {
    print x + 1;
}

while (x < 10) {
    x = x + 1;
}
```

4. Native Function Call:
```
int x = pow(2, 3);
print sqrt(x);
```