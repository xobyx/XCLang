#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MAX_TOKENS 1000
#define MAX_STACK_SIZE 1000
#define MAX_VARS 100
#define MAX_FUNCTIONS 50
#define MAX_BYTECODE 1000

typedef enum {
    OP_PUSH, OP_LOAD, OP_STORE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_LT,
    OP_PRINT, OP_JMP, OP_JMPF, OP_CALL, OP_RET, OP_ENTER, OP_LEAVE
} OpCode;

typedef enum {
    TOKEN_INT, TOKEN_ID, TOKEN_FUNCTION, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN,
    TOKEN_PRINT, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV, TOKEN_LT, TOKEN_COMMA, TOKEN_ASSIGN, TOKEN_EOF,TOKEN_NATIVE_FUNC
} TokenType;

typedef struct {
	
    TokenType type;
    char str[64];
    int value;
} Token;
/*
typedef struct {
    char name[32];
    int address;
    int param_count;
    int local_count;
} Function;
*/
typedef int (*NativeFunction)(int* args, int arg_count);  // Function pointer type for native functions

typedef struct {
    char name[32];
    int address;
    int param_count;
    int local_count;
    NativeFunction native_func;  // Add this field
    int is_native;              // Add this field
} Function;
typedef struct {
    OpCode op;
    int arg;
} Bytecode;

Token tokens[MAX_TOKENS];
int token_count = 0;
int current_token = 0;

char var_names[MAX_VARS][32];
int var_count = 0;

Function functions[MAX_FUNCTIONS];
int function_count = 0;

Bytecode bytecode[MAX_BYTECODE];
int bc_count = 0;

int stack[MAX_STACK_SIZE];
int sp = 0;

typedef struct {
    int bp;          // Base pointer
    int ret_addr;    // Return address
    int prev_bp;     // Previous base pointer
} Frame;

Frame call_stack[MAX_STACK_SIZE];
int fp = 0;  // Frame pointer

int native_sqrt(int* args, int arg_count) {
    if (arg_count != 1) {
        printf("sqrt expects 1 argument\n");
        return 0;
    }
    return 60;
    //return (int)sqrt(args[0]);
}

int native_pow(int* args, int arg_count) {
    if (arg_count != 2) {
        printf("pow expects 2 arguments\n");
        return 0;
    }
    return 60;
    //return (int)pow(args[0], args[1]);
}
void register_native_function(const char* name, NativeFunction func, int param_count) {
    Function* f = &functions[function_count];
    strcpy(f->name, name);
    f->native_func = func;
    f->param_count = param_count;
    f->is_native = 1;
    function_count++;
}
// Track where main code starts
int main_code_start = 0;
void parse_function() ;
int find_function(const char* name);
void emit(OpCode op, int arg) {
    bytecode[bc_count].op = op;
    bytecode[bc_count].arg = arg;
    bc_count++;
}
 
void tokenize(const char* input) {
    const char* p = input;
    while (*p) {
        if (isspace(*p) || *p == ';') {
            p++;
            continue;
        }
        
        Token* t = &tokens[token_count++];
        
        if (isdigit(*p)) {
            t->type = TOKEN_INT;
            t->value = strtol(p, (char**)&p, 10);
            continue;
        }
        
        if (isalpha(*p)) {
            int i = 0;
            while (isalnum(*p)) {
                t->str[i++] = *p++;
            }
            t->str[i] = 0;
            
            if (strcmp(t->str, "function") == 0) t->type = TOKEN_FUNCTION;
            else if (strcmp(t->str, "if") == 0) t->type = TOKEN_IF;
            else if (strcmp(t->str, "return") == 0) t->type = TOKEN_RETURN;
            else if (strcmp(t->str, "print") == 0) t->type = TOKEN_PRINT;
            else t->type = TOKEN_ID;
            continue;
        }
        
        switch (*p) {
            case '(': t->type = TOKEN_LPAREN; break;
            case ')': t->type = TOKEN_RPAREN; break;
            case '{': t->type = TOKEN_LBRACE; break;
            case '}': t->type = TOKEN_RBRACE; break;
            case '+': t->type = TOKEN_PLUS; break;
            case '-': t->type = TOKEN_MINUS; break;
            case '*': t->type = TOKEN_MUL; break;
            case '/': t->type = TOKEN_DIV; break;
            case '<': t->type = TOKEN_LT; break;
            case '=': t->type = TOKEN_ASSIGN; break;
            case ',': t->type = TOKEN_COMMA; break;
            default: 
                printf("Skipping character: %c\n", *p);
                p++;
                continue;
        }
        p++;
    }
    tokens[token_count].type = TOKEN_EOF;
}
void print_token(Token* t) {
    printf("Token: ");
    switch(t->type) {
        case TOKEN_INT: printf("INT(%d)", t->value); break;
        case TOKEN_ID: printf("ID(%s)", t->str); break;
        case TOKEN_FUNCTION: printf("FUNCTION"); break;
        case TOKEN_IF: printf("IF"); break;
        case TOKEN_RETURN: printf("RETURN"); break;
        case TOKEN_PRINT: printf("PRINT"); break;
        case TOKEN_LPAREN: printf("("); break;
        case TOKEN_RPAREN: printf(")"); break;
        case TOKEN_LBRACE: printf("{"); break;
        case TOKEN_RBRACE: printf("}"); break;
        case TOKEN_PLUS: printf("+"); break;
        case TOKEN_MINUS: printf("-"); break;
        case TOKEN_MUL: printf("*"); break;
        case TOKEN_DIV: printf("/"); break;
        case TOKEN_LT: printf("<"); break;
        case TOKEN_ASSIGN: printf("="); break;
        case TOKEN_COMMA: printf(","); break;
        case TOKEN_EOF: printf("EOF"); break;
        default: printf("UNKNOWN"); break;
    }
    printf("\n");
}

void print_bytecode(int count) {
    printf("\nBytecode dump:\n");
    for(int i = 0; i < count; i++) {
        printf("%d: ", i);
        switch(bytecode[i].op) {
            case OP_PUSH: printf("PUSH %d", bytecode[i].arg); break;
            case OP_LOAD: printf("LOAD %d", bytecode[i].arg); break;
            case OP_STORE: printf("STORE %d", bytecode[i].arg); break;
            case OP_ADD: printf("ADD"); break;
            case OP_SUB: printf("SUB"); break;
            case OP_MUL: printf("MUL"); break;
            case OP_DIV: printf("DIV"); break;
            case OP_LT: printf("LT"); break;
            case OP_PRINT: printf("PRINT"); break;
            case OP_JMP: printf("JMP %d", bytecode[i].arg); break;
            case OP_JMPF: printf("JMPF %d", bytecode[i].arg); break;
            case OP_CALL: printf("CALL %d", bytecode[i].arg); break;
            case OP_RET: printf("RET"); break;
            case OP_ENTER: printf("ENTER %d", bytecode[i].arg); break;
            case OP_LEAVE: printf("LEAVE"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");
    }
}

void stack_push(int value) {
    if (sp >= MAX_STACK_SIZE) {
        printf("Stack overflow!\n");
        exit(1);
    }
    stack[sp++] = value;
}

int stack_pop() {
    if (sp <= 0) {
        printf("Stack underflow!\n");
        exit(1);
    }
    return stack[--sp];
}

int find_var(const char* name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_names[i], name) == 0) return i;
    strcpy(var_names[var_count], name);
    return var_count++;
}

void parse_expression();

void parse_return_statement() {
    current_token++; // skip return
    
    parse_expression();  // This will handle the recursive calls properly
    
    emit(OP_RET, 0);
}

  // Forward declaration
void parse_block();   
void parse_term();
void parse_factor();

void parse_factor() {
    Token t = tokens[current_token];
    
    if (t.type == TOKEN_INT) {
        emit(OP_PUSH, t.value);
        current_token++;
    }
    else if (t.type == TOKEN_ID) {
        current_token++; // consume ID
        
        if (tokens[current_token].type == TOKEN_LPAREN) {
            // Function call
            char func_name[32];
            strcpy(func_name, t.str);
            current_token++; // skip (
            
            // Parse arguments
            int arg_count = 0;
            if (tokens[current_token].type != TOKEN_RPAREN) {
                parse_expression();
                arg_count++;
                
                while (tokens[current_token].type == TOKEN_COMMA) {
                    current_token++; // skip comma
                    parse_expression();
                    arg_count++;
                }
            }
            
            if (tokens[current_token].type != TOKEN_RPAREN) {
                printf("Expected )\n");
                exit(1);
            }
            current_token++; // skip )
            
            int func_idx = find_function(func_name);
            if (func_idx >= 0) {
                Function* f = &functions[func_idx];
                if (arg_count != f->param_count) {
                    printf("Function %s expects %d arguments but got %d\n", 
                           func_name, f->param_count, arg_count);
                    exit(1);
                }
                emit(OP_CALL, func_idx);
            } else {
                printf("Unknown function: %s\n", func_name);
                exit(1);
            }
        } else {
            // Variable
            emit(OP_LOAD, find_var(t.str));
        }
    }
    else if (t.type == TOKEN_LPAREN) {
        current_token++; // skip (
        parse_expression();
        
        if (tokens[current_token].type != TOKEN_RPAREN) {
            printf("Expected )\n");
            exit(1);
        }
        current_token++; // skip )
    }
    else {
        printf("Unexpected token in factor\n");
        print_token(&t);
        exit(1);
    }
}

void parse_term() {
    parse_factor();
    
    while (current_token < token_count && 
           (tokens[current_token].type == TOKEN_MUL || 
            tokens[current_token].type == TOKEN_DIV)) {
        TokenType op = tokens[current_token].type;
        current_token++; // skip operator
        
        parse_factor();
        
        if (op == TOKEN_MUL)
            emit(OP_MUL, 0);
        else
            emit(OP_DIV, 0);
    }
}

void parse_expression() {
    // First handle comparison
    parse_term();
    
    while (current_token < token_count) {
        TokenType op = tokens[current_token].type;
        if (op != TOKEN_PLUS && op != TOKEN_MINUS && op != TOKEN_LT)
            break;
            
        current_token++; // skip operator
        parse_term();
        
        switch (op) {
            case TOKEN_PLUS: emit(OP_ADD, 0); break;
            case TOKEN_MINUS: emit(OP_SUB, 0); break;
            case TOKEN_LT: emit(OP_LT, 0); break;
            default: break;
        }
    }
}

void parse_if_statement() {
    current_token++; // skip if
    current_token++; // skip (
    
    parse_expression(); // condition
    
    current_token++; // skip )
    
    emit(OP_JMPF, bc_count + 2); // We'll patch this later
    int jump_addr = bc_count - 1;
    
    parse_block();
    
    // Patch the jump address
    bytecode[jump_addr].arg = bc_count;
}

void parse_statement() {
    switch (tokens[current_token].type) {
        case TOKEN_IF:
            parse_if_statement();
            break;
            
        case TOKEN_RETURN:
            parse_return_statement();
            break;
            
        case TOKEN_PRINT:
            current_token++; // skip print
            parse_expression();
            emit(OP_PRINT, 0);
            break;
            
        default:
            parse_expression();
            break;
    }
}

void parse_block() {
    current_token++; // skip {
    while (tokens[current_token].type != TOKEN_RBRACE) {
        parse_statement();
    }
    current_token++; // skip }
}

void register_function() {
    current_token++; // skip function keyword
    Function* f = &functions[function_count];
    strcpy(f->name, tokens[current_token].str);
    f->address = bc_count;
    f->param_count = 0; // Will be updated during actual parsing
    f->is_native = 0;   // Not a native function
    function_count++;
    
    // Count parameters for better initialization
    current_token++; // move to function name
    current_token++; // skip (
    
    if (tokens[current_token].type != TOKEN_RPAREN) {
        f->param_count = 1; // First parameter
        
        while (tokens[current_token].type != TOKEN_RPAREN) {
            if (tokens[current_token].type == TOKEN_COMMA) {
                f->param_count++;
            }
            current_token++;
        }
    }
    
    // Skip the rest of function declaration and body
    int brace_count = 0;
    while (brace_count > 0 || tokens[current_token].type != TOKEN_RBRACE) {
        if (tokens[current_token].type == TOKEN_LBRACE) brace_count++;
        if (tokens[current_token].type == TOKEN_RBRACE) brace_count--;
        current_token++;
    }
    current_token++; // skip final }
}

void parse_function() {
    current_token++; // skip function
    char name[32];
    strcpy(name, tokens[current_token++].str);
    
    // Find the function in our registry
    int func_idx = find_function(name);
    if (func_idx == -1) {
        printf("Internal error: Function %s not found in registry\n", name);
        exit(1);
    }
    
    Function* f = &functions[func_idx];
    f->address = bc_count; // Update the actual address
    
    current_token++; // skip (
    
    // Handle parameters
    f->param_count = 0;
    if (tokens[current_token].type != TOKEN_RPAREN) {
        // First parameter
        find_var(tokens[current_token].str);
        current_token++;
        f->param_count++;
        
        // Additional parameters
        while (tokens[current_token].type == TOKEN_COMMA) {
            current_token++; // skip comma
            find_var(tokens[current_token].str);
            current_token++;
            f->param_count++;
        }
    }
    
    current_token++; // skip )
    
    emit(OP_ENTER, f->param_count);
    parse_block();
}
void parse() {
    // First pass: register all functions
    int original_token = current_token;
    while (tokens[current_token].type != TOKEN_EOF) {
        if (tokens[current_token].type == TOKEN_FUNCTION) {
            register_function();
        } else {
            current_token++;
        }
    }
    
    // Debug: Print registered functions
    printf("\nRegistered functions:\n");
    for (int i = 0; i < function_count; i++) {
        printf("Function %s at address %d with %d params\n", 
               functions[i].name, 
               functions[i].address, 
               functions[i].param_count);
    }
    
    // Reset for second pass
    current_token = original_token;
    
    // Second pass: parse actual code
    while (tokens[current_token].type != TOKEN_EOF) {
        if (tokens[current_token].type == TOKEN_FUNCTION) {
            parse_function();
        } else {
            parse_statement();
        }
    }
}



// And modify find_function to print debug info:
int find_function(const char* name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            return i;
        }
    }
    printf("Function %s not found. Available functions:\n", name);
    for (int i = 0; i < function_count; i++) {
        printf("- %s\n", functions[i].name);
    }
    return -1;
}
void execute() {
    int pc = main_code_start;
    int bp = 0;
    
    printf("Starting execution from PC=%d\n", pc);
    
    while (pc < bc_count) {
        Bytecode* bc = &bytecode[pc];
        printf("Executing PC=%d SP=%d BP=%d FP=%d | ", pc, sp, bp, fp);
        
        switch (bc->op) {
            case OP_PUSH:
                printf("PUSH %d\n", bc->arg);
                stack_push(bc->arg);
                break;
                
            case OP_LOAD: {
                int addr = bp + bc->arg;
                printf("LOAD from %d (bp=%d + %d)\n", addr, bp, bc->arg);
                if (addr < 0 || addr >= sp) {
                    printf("Error: Invalid load address %d (sp=%d)\n", addr, sp);
                    exit(1);
                }
                stack_push(stack[addr]);
                break;
            }
                /*
            case OP_CALL: {
                Function* f = &functions[bc->arg];
                printf("CALL %s\n", f->name);
                
                call_stack[fp].bp = bp;
                call_stack[fp].ret_addr = pc + 1;
                fp++;
                
                bp = sp - f->param_count;
                pc = f->address - 1;
                break;
            }
            */
            case OP_CALL: {
                Function* f = &functions[bc->arg];
                
                if (f->is_native) {
                    // Handle native function call
                    int args[MAX_STACK_SIZE];
                    for (int i = f->param_count - 1; i >= 0; i--) {
                        args[i] = stack_pop();
                    }
                    
                    int result = f->native_func(args, f->param_count);
                    stack_push(result);
                    
                } else {
                    // Original bytecode function handling
                    call_stack[fp].bp = bp;
                    call_stack[fp].ret_addr = pc + 1;
                    fp++;
                    
                    bp = sp - f->param_count;
                    pc = f->address - 1;
                }
                break;
            }
                
            case OP_RET: {
                int return_value = stack_pop();
                printf("RET %d\n", return_value);
                
                if (fp > 0) {
                    fp--;
                    sp = bp;
                    bp = call_stack[fp].bp;
                    pc = call_stack[fp].ret_addr - 1;
                    stack_push(return_value);
                } else {
                    stack_push(return_value);
                    return;
                }
                break;
            }
                
            case OP_ADD: {
                int b = stack_pop();
                int a = stack_pop();
                printf("ADD %d + %d\n", a, b);
                stack_push(a + b);
                break;
            }
                
            case OP_SUB: {
                int b = stack_pop();
                int a = stack_pop();
                printf("SUB %d - %d\n", a, b);
                stack_push(a - b);
                break;
            }
                
            case OP_LT: {
                int b = stack_pop();
                int a = stack_pop();
                printf("LT %d < %d\n", a, b);
                stack_push(a < b ? 1 : 0);
                break;
            }
                
            case OP_JMPF: {
                int condition = stack_pop();
                printf("JMPF %d -> %d\n", condition, bc->arg);
                if (!condition) {
                    pc = bc->arg - 1;
                }
                break;
            }
                
            case OP_PRINT: {
                int value = stack_pop();
                printf("Output: %d\n", value);
                break;
            }
                
            case OP_ENTER:
                printf("ENTER\n");
                break;
        }
        
        pc++;
        
        printf("Stack:");
        for (int i = 0; i < sp; i++) {
            printf(" %d", stack[i]);
        }
        printf("\n");
    }
}
char input[] = "function fib(n) { if (n < 2) { return n; } return fib(n-1) + fib(n-2); } print fib(8);";
//char input[] = "print pow(2,2);";
int main() {
    // Initialize all counters
    token_count = 0;
    current_token = 0;
    var_count = 0;
    function_count = 0;
    bc_count = 0;
    sp = 0;
    fp = 0;
    main_code_start = 0;
    register_native_function("sqrt", native_sqrt, 1);
    register_native_function("pow", native_pow, 2);

    printf("Tokenizing program...\n");
    tokenize(input);//program);
    
    printf("\nTokens:\n");
    for (int i = 0; i < token_count; i++) {
        print_token(&tokens[i]);
    }
    
    printf("\nParsing program...\n");
    parse();
    
    printf("\nBytecode:\n");
    print_bytecode(bc_count);
    printf("$ start at %d\n",main_code_start);
    printf("\nExecuting program...\n");
    main_code_start = 17;
    execute();
    
    return 0;
}