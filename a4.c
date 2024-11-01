#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define MAX_TOKENS 1000
#define MAX_STACK_SIZE 1000
#define MAX_VARS 100
#define MAX_FUNCTIONS 50
#define MAX_BYTECODE 1000
#define MAX_STRINGS 1000
#define MAX_STRING_LENGTH 256
typedef enum {
    OP_PUSH, OP_LOAD, OP_STORE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_LT,OP_GT,OP_LTE,OP_GTE,
    OP_PRINT, OP_JMP, OP_JMPF, OP_CALL, OP_RET, OP_ENTER, OP_LEAVE,OP_EQL
    ,OP_ARRAY_LOAD,    // Load from array
    OP_ARRAY_STORE,   // Store to array
    OP_STRING_PUSH    // Push string literal
} OpCode;

typedef enum {
    TOKEN_VALUE, TOKEN_ID/*,TOKEN_FUNCTION*/, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN,
    TOKEN_PRINT, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV, TOKEN_LT,TOKEN_GT,TOKEN_LTE,TOKEN_GTE, TOKEN_COMMA, TOKEN_ASSIGN, TOKEN_EOF,TOKEN_NATIVE_FUNC,TOKEN_EQL,
    TOKEN_INT_TYPE,    // "int" keyword
    TOKEN_FLOAT_TYPE,  // "float" keyword
    TOKEN_STRING_TYPE, // "string" keyword
    TOKEN_LBRACKET, TOKEN_RBRACKET,    // [] for array access
    TOKEN_STRING_LITERAL,               // For string literals
    TOKEN_ARRAY_TYPE       
} TokenType;
/*
typedef struct {
	
    TokenType type;
    char str[64];
    int value;
} Token;
*/
typedef struct {
    TokenType type;
    
    int value;          // For numeric values
    char str[64];    
    int length;            // For string literals and arrays
} Token;
typedef int (*NativeFunction)(int* args, int arg_count);  // Function pointer type for native functions
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_INT_ARRAY,
    TYPE_FLOAT_ARRAY,
    TYPE_STRING_ARRAY
} VarType;
typedef struct {
    char name[32];
    VarType type;
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


typedef struct {
    char data[MAX_STRING_LENGTH];
    int length;
    int id;
} String;

String string_pool[MAX_STRINGS];
int string_count = 0;

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


typedef struct {
    char name[32];
    VarType type;
    int offset;  // Stack offset
    int size;       // Array size (1 for non-arrays)
    int is_array;   // Flag for arrays
} Variable;

Variable variables[MAX_VARS];

int create_string(const char* str) {
    if (string_count >= MAX_STRINGS) {
        printf("String pool overflow\n");
        return -1;
    }
    
    int id = string_count++;
    strncpy(string_pool[id].data, str, MAX_STRING_LENGTH - 1);
    string_pool[id].data[MAX_STRING_LENGTH - 1] = '\0';
    string_pool[id].length = strlen(string_pool[id].data);
    string_pool[id].id = id;
    
    return id;
}
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
int main_code_start_t = 0;
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
        if (*p == '"') {  // String literal
            p++;  // Skip opening quote
            int i = 0;
            while (*p && *p != '"') {
            	
                t->str[i++] = *p++;
            }
            t->str[i] = '\0';
            t->length = i;
            t->type = TOKEN_STRING_LITERAL;
            p++;  // Skip closing quote
            continue;
        }
        if (isdigit(*p)) {
            t->type = TOKEN_VALUE;
            t->value = strtol(p, (char**)&p, 10);
            continue;
        }
        
        if (isalpha(*p)) {
            int i = 0;
            while (isalnum(*p)) {
                t->str[i++] = *p++;
            }
            t->str[i] = 0;
            
            //if (strcmp(t->str, "function") == 0) t->type = TOKEN_FUNCTION;
            if (strcmp(t->str, "if") == 0) t->type = TOKEN_IF;
            else if (strcmp(t->str, "else") == 0) t->type = TOKEN_ELSE;
            else if (strcmp(t->str, "while") == 0) t->type = TOKEN_WHILE;
            else if (strcmp(t->str, "return") == 0) t->type = TOKEN_RETURN;
            else if (strcmp(t->str, "print") == 0) t->type = TOKEN_PRINT;
            else if (strcmp(t->str, "int") == 0) t->type = TOKEN_INT_TYPE;
        	else if (strcmp(t->str, "float") == 0) t->type = TOKEN_FLOAT_TYPE;
        	else if (strcmp(t->str, "string") == 0) t->type = TOKEN_STRING_TYPE;
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
            case '<': {if(*(p + 1) == '='){t->type = TOKEN_LTE;p++;} else{t->type = TOKEN_LT;} break;}
            case '>': {if(*(p + 1) == '='){t->type = TOKEN_GTE;p++;} else{t->type = TOKEN_GT;} break;}
            case '=': {if(*(p + 1) == '='){t->type = TOKEN_EQL;p++;} else{t->type = TOKEN_ASSIGN;} break;}
            case ',': t->type = TOKEN_COMMA; break;
            case '[': t->type = TOKEN_LBRACKET; break;
            case ']': t->type = TOKEN_RBRACKET; break;
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
        case TOKEN_VALUE: printf("VALUE(%d)", t->value); break;
        case TOKEN_ID: printf("ID(%s)", t->str); break;
        //case TOKEN_FUNCTION: printf("FUNCTION"); break;
        case TOKEN_IF: printf("IF"); break;
        case TOKEN_ELSE: printf("ELSE"); break;
        case TOKEN_WHILE: printf("WHILE"); break;
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
        case TOKEN_GT: printf(">"); break;
        case TOKEN_LTE: printf("<="); break;
        case TOKEN_GTE: printf(">="); break;
        case TOKEN_EQL: printf("=="); break;
        case TOKEN_ASSIGN: printf("="); break;
        case TOKEN_COMMA: printf(","); break;
        case TOKEN_EOF: printf("EOF"); break;
        case TOKEN_INT_TYPE: printf("INTEGER"); break;
    	case TOKEN_FLOAT_TYPE: printf("FLOAT"); break;
    	case TOKEN_STRING_TYPE: printf("STRING"); break;
    	case TOKEN_LBRACKET: printf("["); break;
		case TOKEN_RBRACKET: printf("]"); break;    // [] for array access
    	case TOKEN_STRING_LITERAL: printf("%s",t->str); break;               // For string literals
   	 case TOKEN_ARRAY_TYPE: printf("STRING"); break;    
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
            case OP_GT: printf("GT"); break;
            case OP_LTE: printf("LTE"); break;
            case OP_GTE: printf("GTE"); break;
            case OP_EQL: printf("EQL"); break;
            case OP_PRINT: printf("PRINT"); break;
            case OP_JMP: printf("JMP %d", bytecode[i].arg); break;
            case OP_JMPF: printf("JMPF %d", bytecode[i].arg); break;
            case OP_CALL: printf("CALL %d", bytecode[i].arg); break;
            case OP_RET: printf("RET"); break;
            case OP_ENTER: printf("ENTER %d", bytecode[i].arg); break;
            case OP_LEAVE: printf("LEAVE"); break;
            case OP_ARRAY_LOAD: printf("ARRAY_LOAD %d", bytecode[i].arg); break;   // Load from array
    		case OP_ARRAY_STORE: printf("ARRAY_STORE %d", bytecode[i].arg); break;   // Store to array
            case OP_STRING_PUSH: printf("OP_STRING_PUSH"); break;    // Push string literal
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
/*
int find_var(const char* name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_names[i], name) == 0) return i;
    strcpy(var_names[var_count], name);
    return var_count++;
}
*/
Variable* find_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) 
            return &variables[i];
    }
    return NULL;
}

Variable* add_var(const char* name, VarType type) {
    if (var_count >= MAX_VARS) {
        printf("Too many variables\n");
        exit(1);
    }
    strcpy(variables[var_count].name, name);
    variables[var_count].type = type;
    variables[var_count].offset = var_count;
    return &variables[var_count++];
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

VarType token_to_type(TokenType token) {
    switch (token) {
        case TOKEN_INT_TYPE: return TYPE_INT;
        case TOKEN_FLOAT_TYPE: return TYPE_FLOAT;
        case TOKEN_STRING_TYPE: return TYPE_STRING;
        default:
            printf("Invalid type token\n");
            exit(1);
    }
}
static void parse_array_initializer(Variable* var, int size) {
    if (tokens[current_token].type != TOKEN_ASSIGN) {
        for (int i = 0; i < size; i++) {
	           /* emit(OP_PUSH, 0);
	            emit(OP_STORE, var->offset + i);*/
	emit(OP_PUSH, i);
        emit(OP_PUSH, 0);
        emit(OP_ARRAY_STORE, var->offset);
        
	        }// No initializer
    }
    current_token++; // skip =
    
    if (tokens[current_token].type != TOKEN_LBRACE) {
        printf("Expected { for array initializer\n");
        exit(1);
    }
    current_token++; // skip {
    
    int count = 0;
    if (tokens[current_token].type != TOKEN_RBRACE) {
        // Parse first value
        Token t = tokens[current_token];
        if (t.type != TOKEN_VALUE) {
            printf("Expected numeric value in array initializer\n");
            exit(1);
        }
        emit(OP_PUSH, count);
        printf("\n\n🪲🪲token :");
        print_token(&tokens[current_token]);
        printf("\n\n🪲🪲🪲🪲🪲🪲🪲🪲🪲🪲 :");
        //emit(OP_PUSH, t.value);
        parse_expression(); 
        printf("\n\n🪲🪲token :");
        print_token(&tokens[current_token]);
        printf("\n\n🪲🪲🪲🪲🪲🪲🪲🪲🪲🪲 :");
        //parse_expression();    // Parse and evaluate the expression
        emit(OP_ARRAY_STORE, var->offset);
        count++;
        //current_token++;
        
        // Parse remaining values
        while (tokens[current_token].type == TOKEN_COMMA) {
            current_token++; // skip comma
            t = tokens[current_token];
            if (t.type != TOKEN_VALUE) {
                printf("Expected numeric value in array initializer\n");
                exit(1);
            }
            // For each element, emit its index and value
            emit(OP_PUSH, count);
            parse_expression(); 
            //emit(OP_PUSH, t.value);
            emit(OP_ARRAY_STORE, var->offset);
            count++;
            printf("\n\n🪲🪲token :");
            print_token(&tokens[current_token]);
            //current_token++;
        }
    }
    
    if (tokens[current_token].type != TOKEN_RBRACE) {
        printf("Expected } after array initializer got :");
        print_token(&tokens[current_token]);
        exit(1);
    }
    current_token++; // skip }
    
    if (count > size) {
        printf("Too many initializers for array size %d\n", size);
        exit(1);
    }
    
    // Fill remaining elements with 0 if any
    while (count < size) {
        emit(OP_PUSH, count);
        emit(OP_PUSH, 0);
        emit(OP_ARRAY_STORE, var->offset);
        count++;
    }
}
void parse_declaration() {
    // Get the type
    VarType type = token_to_type(tokens[current_token].type);
    current_token++; // Skip type token
    
    // Get variable name
    if (tokens[current_token].type != TOKEN_ID) {
        printf("Expected identifier after type\n");
        exit(1);
    }
    char* var_name = tokens[current_token].str;
    current_token++; // Skip identifier
    
    if (tokens[current_token].type != TOKEN_LPAREN) { 
	    if (tokens[current_token].type == TOKEN_LBRACKET) {
	        // Array declaration
	        current_token++; // Skip [
	        int size = tokens[current_token].value;
	printf("\n\n•••••var size %d\n\n",size);
	        current_token++; // Skip size
	        current_token++; // Skip ]
	        // change array type base on int[y]
	        VarType array_type;
	        switch (type) {
	            case TYPE_INT: array_type = TYPE_INT_ARRAY; break;
	            case TYPE_FLOAT: array_type = TYPE_FLOAT_ARRAY; break;
	            case TYPE_STRING: array_type = TYPE_STRING_ARRAY; break;
	            default:
	                printf("Invalid array base type\n");
	                exit(1);
	        }
	        
	        Variable* var = add_var(var_name, array_type);
	        var->size = size;
	        var->is_array = 1;
	        parse_array_initializer(var,size);
	        // Initialize array with zeros
	        
	    } else {
	        // Regular variable declaration
	        Variable* var = add_var(var_name, type);
	        var->size = 1;
	        var->is_array = 0;
	        
	        if (tokens[current_token].type == TOKEN_ASSIGN) {
	            current_token++; // Skip =
	            if (type == TYPE_STRING && tokens[current_token].type == TOKEN_STRING_LITERAL) {
	                emit(OP_STRING_PUSH, create_string (tokens[current_token].str));
	                current_token++;
	            } else {
	                parse_expression();
	            }
	            emit(OP_STORE, var->offset);
	        }
	    }
    
    
    
    }else{//function definition 
		Function* f = &functions[function_count];
	    strcpy(f->name, var_name);
	    f->address = bc_count;
	    f->is_native = 0;
	    f->type=type;
	    
	    current_token++; // skip (
	    
    	f->param_count = 0;
	    if (tokens[current_token].type != TOKEN_RPAREN) {
	    	
	        VarType type = token_to_type(tokens[current_token].type);
	        current_token++; // Skip type token
	        Variable* var = add_var(tokens[current_token].str, type);
	        current_token++;//skip name
			f->param_count++;
	        
	        
	        while (tokens[current_token].type == TOKEN_COMMA) {
	            current_token++; // skip comma
	            VarType type = token_to_type(tokens[current_token].type);
	            current_token++; // Skip type token
	            Variable* var = add_var(tokens[current_token].str, type);
	            current_token++;
	            f->param_count++;
	            
	        }
	    }
	    
	    function_count++;
		printf("Registered function %s with %d parameters at address %d\n", 
		           f->name, f->param_count, f->address);
		           
	    
	    current_token++; // skip )
	    emit(OP_ENTER, f->param_count);
	    
	    // Parse function body
	    parse_block();
		if(bytecode[bc_count-1].op!=OP_RET)
		{
			printf("Error:function must have return\n");
			exit(-1);
		}
	    main_code_start_t=bc_count;
	    
	    // Make sure we have a return statement at the end
	    //emit(OP_RET, 0);
	    

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
    
    while (current_token < token_count && tokens[current_token].type!=TOKEN_COMMA ) {
        TokenType op = tokens[current_token].type;
        if (op != TOKEN_PLUS && op != TOKEN_MINUS && op != TOKEN_LT && op != TOKEN_GT && op != TOKEN_GTE && op != TOKEN_LTE && op != TOKEN_EQL )
            break;
            
        current_token++; // skip operator
        
        parse_term();
        
        switch (op) {
            case TOKEN_PLUS: emit(OP_ADD, 0); break;
            case TOKEN_MINUS: emit(OP_SUB, 0); break;
            case TOKEN_LT: emit(OP_LT, 0);break;
            case TOKEN_GT: emit(OP_GT, 0);break;
            case TOKEN_GTE: emit(OP_GTE, 0);break;
            case TOKEN_LTE: emit(OP_LTE, 0);break;
            case TOKEN_EQL: emit(OP_EQL, 0);break;
				
				
				
            default: break;
        }
    }
}

void _parse_if_statement() {
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

void parse_if_statement() {
    current_token++; // Skip 'if'
    current_token++; // Skip '('
    parse_expression();
    current_token++; // Skip ')'
    
    //int jmpf_addr = bc_count;
    //emit(OP_JMPF, 0);
    emit(OP_JMPF, bc_count + 2); // We'll patch this later
    int jmpf_addr = bc_count - 1;
    
    parse_block();
    
    if (tokens[current_token].type == TOKEN_ELSE) {
        current_token++; // Skip 'else'
        int jmp_addr = bc_count;
        emit(OP_JMP, 0);
        bytecode[jmpf_addr].arg = bc_count;
        parse_block();
        bytecode[jmp_addr].arg = bc_count;
    } else {
        bytecode[jmpf_addr].arg = bc_count;
    }
}

void parse_while_statement() {
    current_token++; // Skip 'while'
    current_token++; // Skip '('
    
    int loop_start = bc_count;
    parse_expression();
    current_token++; // Skip ')'
    
    int jmpf_addr = bc_count;
    emit(OP_JMPF, 0);
    
    parse_block();
    
    emit(OP_JMP, loop_start);
    bytecode[jmpf_addr].arg = bc_count;
}
void parse_statement() {
    switch (tokens[current_token].type) {
        case TOKEN_IF:
            parse_if_statement();
            break;
        case TOKEN_WHILE: 
        	parse_while_statement();
        	break;
    	
        case TOKEN_RETURN:
            parse_return_statement();
            break;
            
        case TOKEN_PRINT:
            current_token++; // skip print
            parse_expression();
            emit(OP_PRINT, 0);
            break;
        case TOKEN_INT_TYPE:
        case TOKEN_FLOAT_TYPE:
        case TOKEN_STRING_TYPE:
            
        	parse_declaration();
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


void parse() {
    // First pass: register all functions
    int original_token = current_token;
    while (current_token < token_count) {
       // if (tokens[current_token].type == TOKEN_FUNCTION) {
            //register_function();
      //  } else {
            current_token++;
      //  }
    }
    
    // Reset for second pass
    current_token = original_token;
    var_count = 0;  // Reset variable count for second pass
    
    // Second pass: parse function bodies and main code
    while (current_token < token_count) {
       // if (tokens[current_token].type == TOKEN_FUNCTION) {
           // parse_function();
      //  } else {
            // This is main code - remember where it starts
            main_code_start = bc_count;
            parse_statement();
       // }
    }
}
void check_types(VarType left, VarType right, const char* operation) {
    if (left != right) {
        printf("Type mismatch in %s operation\n", operation);
        exit(1);
    }
}

static void parse_function_call(Token* id_token) {
    int func_idx = find_function(id_token->str);
    if (func_idx == -1) {
        printf("Unknown function: %s\n", id_token->str);
        exit(1);
    }
    
    Function* f = &functions[func_idx];
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
    
    if (arg_count != f->param_count) {
        printf("Error: Function %s expects %d arguments but got %d\n",
               id_token->str, f->param_count, arg_count);
        exit(1);
    }
    
    current_token++; // skip )
    emit(OP_CALL, func_idx);
}

static void parse_array_operation(Variable* var) {
    current_token++; // skip [
    parse_expression(); // parse index expression
    
    if (tokens[current_token].type != TOKEN_RBRACKET) {
        printf("Expected ]\n");
        exit(1);
    }
    current_token++; // skip ]
    
    if (tokens[current_token].type == TOKEN_ASSIGN) {
        // Array assignment: arr[idx] = expr
        current_token++; // skip =
        parse_expression();
        emit(OP_ARRAY_STORE, var->offset);
    } else {
        // Array access: arr[idx]
        emit(OP_ARRAY_LOAD, var->offset);
    }
}

static Variable* get_variable(Token* id_token) {
    Variable* var = find_var(id_token->str);
    if (!var) {
        printf("Undefined variable: %s\n", id_token->str);
        exit(1);
    }
    return var;
}

static void parse_identifier(Token* id_token) {
    
    current_token++; // consume ID
    //function 
    if (tokens[current_token].type == TOKEN_LPAREN) {
    	printf("\n•••• %s\n\n",id_token->str);
        parse_function_call(id_token);
        
    } 
    else if (tokens[current_token].type == TOKEN_LBRACKET) {
    	Variable* var = get_variable(id_token);
        parse_array_operation(var);
    }
    else if (tokens[current_token].type == TOKEN_ASSIGN) {
    	Variable* var = get_variable(id_token);
        // Regular variable assignment
        current_token++; // skip =
        parse_expression();
        emit(OP_STORE, var->offset);
    } 
    else {
    	Variable* var = get_variable(id_token);
        // Regular variable access
        emit(OP_LOAD, var->offset);
    }
}

static void parse_parenthesized_expr() {
    current_token++; // skip (
    parse_expression();
    if (tokens[current_token].type != TOKEN_RPAREN) {
        printf("Expected )\n");
        exit(1);
    }
    current_token++; // skip )
}

void parse_factor() {
    Token t = tokens[current_token];
    if (tokens[current_token].type==TOKEN_COMMA)
    	return;
    if (t.type == TOKEN_VALUE) {
        emit(OP_PUSH, t.value);
        current_token++;
    }
    else if (t.type == TOKEN_ID) {
    	printf("\n•••• %s\n\n",t.str);
        parse_identifier(&t);
    }
    else if (t.type == TOKEN_LPAREN) {
        parse_parenthesized_expr();
    }
    else {
        printf("Unexpected token in factor: ");
        print_token(&t);
        exit(1);
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
        // Add bounds checking
        if (pc < 0 || sp >= MAX_STACK_SIZE || fp >= MAX_STACK_SIZE) {
            printf("Runtime error: Stack overflow or invalid PC\n");
            printf("PC: %d, SP: %d, FP: %d\n", pc, sp, fp);
            return;
        }

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
                    return;
                }
                stack_push(stack[addr]);
                break;
            }
            case OP_STORE: {

                int value = stack_pop();

                int addr = bp + bc->arg;

                printf("Storing value %d at address %d\n", value, addr);

                while (addr >= sp) {

                    stack_push(0);

                }

                stack[addr] = value;

                break;

            }            
                
            case OP_CALL: {
                Function* f = &functions[bc->arg];
                printf("CALL %s\n", f->name);
                
                if (f->is_native) {
                    // Handle native function call
                    int args[MAX_STACK_SIZE];
                    for (int i = f->param_count - 1; i >= 0; i--) {
                        args[i] = stack_pop();
                    }
                    
                    int result = f->native_func(args, f->param_count);
                    stack_push(result);
                    
                } else {
                    // Check stack space before call
                    if (fp >= MAX_STACK_SIZE - 1) {
                        printf("Error: Call stack overflow\n");
                        return;
                    }
                    
                    // Save current context
                    call_stack[fp].bp = bp;
                    call_stack[fp].ret_addr = pc + 1;
                    call_stack[fp].prev_bp = bp;
                    fp++;
                    
                    // Set up new frame
                    bp = sp - f->param_count;
                    pc = f->address - 1;
                }
                break;
            }
                
            case OP_RET: {
                if (sp <= 0) {
                    printf("Error: Stack underflow on return\n");
                    return;
                }
                
                int return_value = stack_pop();
                printf("RET %d\n", return_value);
                
                if (fp > 0) {
                    // Restore previous frame
                    fp--;
                    sp = bp;
                    bp = call_stack[fp].bp;
                    pc = call_stack[fp].ret_addr - 1;
                    
                    // Push return value to caller's stack
                    stack_push(return_value);
                } else {
                    // Main function return
                    stack_push(return_value);
                    return;
                }
                break;
            }
                
            case OP_ADD: {
                if (sp < 2) {
                    printf("Error: Stack underflow on add\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("ADD %d + %d\n", a, b);
                stack_push(a + b);
                break;
            }
                
            case OP_SUB: {
                if (sp < 2) {
                    printf("Error: Stack underflow on subtract\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("SUB %d - %d\n", a, b);
                stack_push(a - b);
                break;
            }
            case OP_MUL: {
                if (sp < 2) {
                    printf("Error: Stack underflow on add\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("MUL %d * %d\n", a, b);
                stack_push(a * b);
                break;
            }
                
            case OP_LT: {
                if (sp < 2) {
                    printf("Error: Stack underflow on less than\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("LT %d < %d\n", a, b);
                stack_push(a < b ? 1 : 0);
                break;
            }
            case OP_GT: {
                if (sp < 2) {
                    printf("Error: Stack underflow on less than\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("GT %d < %d\n", a, b);
                stack_push(a > b ? 1 : 0);
                break;
            }
            case OP_LTE: {
                if (sp < 2) {
                    printf("Error: Stack underflow on less than\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("LTE %d < %d\n", a, b);
                stack_push(a <= b ? 1 : 0);
                break;
            }
            case OP_GTE: {
                if (sp < 2) {
                    printf("Error: Stack underflow on less than\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("GTE %d < %d\n", a, b);
                stack_push(a >= b ? 1 : 0);
                break;
            }
            case OP_EQL: {
                if (sp < 2) {
                    printf("Error: Stack underflow on less than\n");
                    return;
                }
                int b = stack_pop();
                int a = stack_pop();
                printf("EQL %d < %d\n", a, b);
                stack_push(a == b ? 1 : 0);
                break;
            }
                
            case OP_JMPF: {
                if (sp < 1) {
                    printf("Error: Stack underflow on conditional jump\n");
                    return;
                }
                int condition = stack_pop();
                printf("JMPF %d -> %d\n", condition, bc->arg);
                if (!condition) {
                    pc = bc->arg - 1;
                }
                break;
            }
            case OP_JMP: {
                if (sp < 1) {
                    printf("Error: Stack underflow on conditional jump\n");
                    return;
                }
                //int condition = stack_pop();
                printf("JMP -> %d\n", bc->arg);
                
                pc = bc->arg - 1;
                
                break;
            }            
                
            case OP_PRINT: {
                if (sp < 1) {
                    printf("Error: Stack underflow on print\n");
                    return;
                }
                int value = stack_pop();
                printf("\n💥 Output: %d\n", value);
                break;
            }
                
            case OP_ENTER:
                printf("ENTER frame with %d parameters\n", bc->arg);
                break;
            case OP_ARRAY_LOAD: {
	            int index = stack_pop();
	            int base_addr = bp + bc->arg;
	printf("\n\n•load••••var size %d\n\n",variables[bc->arg].size);
	
	            if (index < 0 || index >= variables[bc->arg].size) {
	                printf("Array index out of bounds\n");
	                exit(1);
	            }
	
	            stack_push(stack[base_addr + index]);
	            break;
	        }
        
	        case OP_ARRAY_STORE: {
	            int value = stack_pop();
	            int index = stack_pop();
	            int base_addr = bp + bc->arg;
	printf("\n\n•••••store var size %d\n\n",variables[bc->arg].size);
	
	            if (index < 0 || index >= variables[bc->arg].size) {
	                printf("Array index out of bounds\n");
	                exit(1);
	            }
	
	            stack[base_addr + index] = value;
	            break;
	        }
	        
	        case OP_STRING_PUSH: {
	            printf("Pushing string: %s\n", string_pool[bc->arg].data);
	            // For simplicity, we'll just push the string pool index
	            stack_push(bc->arg);
	            break;
	        }
                
            default:
                printf("Error: Unknown opcode %d\n", bc->op);
                return;
        }
        
        pc++;
        
        printf("Stack:");
        for (int i = 0; i < sp; i++) {
            printf(" %d", stack[i]);
        }
        printf("\n");
    }
}
char input[]="int hello(){return 12;}int test(int x,int y){if (x+y<=5){ return x+y+hello();} return x*y;}print test(2,3);int xn[3]={1*2,2*2,3*2};xn[0]=555;print xn[0];print xn [1];";
//char input[] = "int fib(int n) { if (n < 2) { return n; } return fib(n-1) + fib(n-2); } print fib(8);";
//char input[] = "int u=pow(2,2);if (pow(2,2)<u){print u}else {print u+2;}";
//char input[] = "int u=0;while(u<=10){ if(u==10) {print 6666;}print u;u=u+1;}";
int main() {
	time_t start, end;
    double time_taken;
    
    
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
    start = time(NULL)*1000;
    parse();
    end = time(NULL)*1000;
    time_taken = difftime(end, start);
    printf("parse take: %f seconds\n", time_taken);
    printf("\nBytecode:\n");
    print_bytecode(bc_count);
    printf("main_code_start_t : %d \n",main_code_start_t);
    printf("$ start at %d\n",main_code_start);
    printf("\nExecuting program...\n");
    main_code_start =20;// main_code_start_t;
    start = time(NULL);
    execute();
    end = time(NULL);
    time_taken = difftime(end, start);
    printf("execute take: %f seconds\n", time_taken);
    
    return 0;
}