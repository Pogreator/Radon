#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "astnodes.h"

#define true 1
#define false 0

char debug = true;

typedef enum {
    OPEN_BRACE, 
    CLOSE_BRACE, 
    OPEN_PARENTHESIS, 
    CLOSE_PARENTHESIS,
    COLON,
    INDENTATION,
    EOF_TOKEN,
    EMPTY_LINE_TOKEN,
    ERROR,
    INT_KEYWORD, 
    RETURN_FUNCTION,
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    ADD_OPERAND,
    SUB_OPERAND
} token_type;

float binding_power[] = {
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
};

typedef struct{
    token_type type;
    char text[64];
} token;

char *strip(char *s){
    size_t size;
    char *end;

    size = strlen(s);
    if (!size) return s;

    end = s + size - 1;
    while (end >= s && isspace(*end)) end--;
    *(end + 1) = '\0';
    while (*s && isspace(*s)) s++;

    return s;
}

void debug_printf(const char *fmt, ...){
    if (debug) {
        va_list argptr;
        va_start(argptr, fmt);

        vfprintf(stderr, fmt, argptr);

        va_end(argptr);     
    }
}

char is_token_digit(token_type type){
    switch (type){
        case INT_LITERAL: return true;
        case FLOAT_LITERAL: return true;
        default: return false;
        }
}

char is_token_statement(token_type type){
    switch (type){
        case RETURN_FUNCTION: return true;
        default: return false;
        }
}

char is_token_keyword(token_type type){
    switch (type){
        case INT_KEYWORD: return true;
        default: return false;
        }
}

char is_identifier(char c){
    switch (c){
        case '@': return true;
        case '$': return true;
        default: return false;
    }
}

token get_next_token(char **s){
    if (**s == '\0' || **s == '\n') return (token){EOF_TOKEN,";"};

    while (isspace(**s)) (*s)++;

    char c = **s;

    switch (c) {
        case '{': (*s)++; return (token){OPEN_BRACE, "{"};
        case '}': (*s)++; return (token){CLOSE_BRACE, "}"};
        case '(': (*s)++; return (token){OPEN_PARENTHESIS, "("};
        case ')': (*s)++; return (token){CLOSE_PARENTHESIS, ")"};
        case ';': (*s)++; return (token){EOF_TOKEN, ";"};
        case ':': (*s)++; return (token){COLON, ":"};
        case '+': (*s)++; return (token){ADD_OPERAND, "+"};
        case '-': (*s)++; return (token){SUB_OPERAND, "-"};
        case '\t': (*s)++; return (token){INDENTATION,"\t"};
    }

    if (isalpha(c) || is_identifier(c)){
        token t = {IDENTIFIER, ""};
        int i = 0;
        while ((isalnum(**s) || **s == '_' || is_identifier(**s)) && i < 63) {t.text[i++] = **s; (*s)++;}
        t.text[i] = '\0';

        if (strcmp(t.text, "int") == 0) t.type = INT_KEYWORD;
        else if (strcmp(t.text, "return") == 0) t.type = RETURN_FUNCTION;

        return t;
    }

    if (isdigit(c)) {
        token t = {INT_LITERAL, ""};
        int i = 0;
        while (isdigit(**s) && i < 63) {t.text[i++] = **s; (*s)++;}
        t.text[i] = '\0';
        return t;
    }

    (*s)++;
    return (token){ERROR, "Invalid char"};
}

token *lex(FILE *fptr){
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    token *token_list = malloc(10000 * sizeof(token));
    token *start = token_list;
    while ((read = getline(&line, &len, fptr)) != -1) {
        line = strip(line);
        if (!(line[0] == '\0')){
            while (*line) {
                *(token_list++) = get_next_token(&line); 
                debug_printf("%s ", (token_list-1)->text);
            }
            *(token_list++) = get_next_token(&line);
            debug_printf("%s\n", (token_list-1)->text);
        } else {
            *(token_list++) = (token){EMPTY_LINE_TOKEN,"empty"};
        }
    }
    *(token_list++) = (token){EMPTY_LINE_TOKEN,"empty"};
    token_list = start;
    return token_list;
}

ast_expression *parse_expression(token **token_list, float min_bp){
    ast_expression *exp;
    token lhs_token = **token_list;
    switch (lhs_token.type){
        case INT_LITERAL: exp = create_int_literal_expression(atoi(lhs_token.text)); break;
        default: printf("Invalid lhs token <%d>: Aborting\n", lhs_token.type); abort();
    }
    char break_out_of_loop = 0;
    for(;;) {
        token op_token = *(*token_list+1);
        switch (op_token.type){
            case EOF_TOKEN: break_out_of_loop = 1; break;
            case ADD_OPERAND: break;
            case SUB_OPERAND: break;
            default: printf("Invalid op token <%d>: Aborting\n",op_token.type); abort();
        }
        if (break_out_of_loop == 1) break;

        float l_bp = binding_power[op_token.type];
        if (l_bp < min_bp) break;
        float r_bp = l_bp + 0.1;
        (*token_list)++;

        ast_expression *rhs = parse_expression(token_list, r_bp);
        ast_expression *lhs = exp;

        exp = create_binary_expression(op_token.text,lhs,rhs);
    }
    return exp;
}

ast_statement *parse_statement(token **token_list, char main){
    ast_statement *statement = (ast_statement *)malloc(100*sizeof(ast_statement));
    token stmt_token = **token_list;

    switch (stmt_token.type){
        case RETURN_FUNCTION: statement->type=STMT_RETURN; break;
        default: printf("Invalid statement token <%d>: Aborting\n", stmt_token.type); abort();
    }
    char break_out_of_loop = 0;
    char digit;
    for(;;){
        digit = false;
        token next = *(*token_list+1);
        switch (next.type){
            case EOF_TOKEN: break_out_of_loop = 1; break;
            default: 
                if (is_token_digit(next.type)){
                   digit = true; break;
                }else{
                    printf("Invalid token <%d>: Aborting\n",next.type); abort();    
                }
        }
        if (break_out_of_loop == 1) break;

        (*token_list)++;

        if (digit == true){
            statement->data.expression.exp = parse_expression(token_list, -1);
        }

        (*token_list)++;
        
        token check = *(*token_list+1);

        if (check.text[0] == '\0' || (check.type == EMPTY_LINE_TOKEN || is_token_keyword((*token_list+2)->type))) break_out_of_loop = 1;
        if (break_out_of_loop == 1) break;
        (*token_list)++;

        statement->next = parse_statement(token_list, false);
        break_out_of_loop = 1;
        break;
    }
    ast_statement *block = malloc(4096*sizeof(ast_statement));

    if (main == true){
        block->data.block.statements = statement;
    } else {
        block = statement;
    }

    if (break_out_of_loop == 1) return block;
    else printf("Invalid statement: Aborting\n"); abort();
}

ast_function *parse_function(token **token_list){
    ast_function *func = (ast_function *)malloc(sizeof(ast_function));
    token func_type = **token_list;
    switch (func_type.type){
        case INT_KEYWORD: func->type=FUNC_INT; break;
        default: printf("Invalid func_type token <%d>: Aborting\n", func_type.type); abort();
    }
    (*token_list)++;
    
    token func_name = **token_list;
    if (func_name.type != IDENTIFIER) {printf("Invalid function name <%s>: Aborting\n",func_name.text); abort();}
    strcpy(func->name,func_name.text+1);

    (*token_list)++; 
    token open_parenthesis = **token_list;
    if (open_parenthesis.type != OPEN_PARENTHESIS) {printf("Function requires parenthesis after name <()>: Aborting\n"); abort();}
    (*token_list)++; 
    token close_parenthesis = **token_list;
    if (close_parenthesis.type != CLOSE_PARENTHESIS) {printf("Function requires parenthesis after name <()>: Aborting\n"); abort();}
    (*token_list)++;

    token colon = **token_list;
    if (colon.type != COLON) {printf("Function requires colon: Aborting\n"); abort();}
    (*token_list)++;

    if ((**token_list).type == EOF_TOKEN) (*token_list)++;

    ast_statement *block;
    block = parse_statement(token_list,true);
    func->body = block;

    return func;
}

ast_program *parse(token **token_list){
    ast_program *root = (ast_program *)malloc(100*sizeof(ast_program));
    ast_node *root_node = (ast_node *)malloc(100*sizeof(ast_node));

    ast_function **funptr = &root_node->data.function_data.functions;

    (*funptr) = parse_function(token_list);;

    while ((*token_list+1)->type == EMPTY_LINE_TOKEN && (is_token_keyword((*token_list+2)->type))){
        (*token_list)+=2;
        (*funptr)->next = parse_function(token_list);
        (*funptr)->next->previous = (*funptr);
        printf("%s\n",(*funptr)->name);
        (*funptr) = (*funptr)->next;
    }

    root_node->type = NODE_FUNC_DEF;
    while((*funptr)->previous != NULL) (*funptr) = (*funptr)->previous;

    printf("%s\n",(*funptr)->name);
    
    strcpy(root->entry_function, "main");
    root->first_item = root_node;

    return root;
}

void generate_asm(ast_program *prog, FILE *asmfile){
    // Initilasing needed refrences for the generation
    ast_function *function_list = prog->first_item->data.function_data.functions;
    ast_function *start_function_list = function_list;

    printf("%s\n",function_list->name);

    fprintf(asmfile, ".text\n");

    // Generate .global functions
    while (function_list != NULL){
        if (strcmp(function_list->name, prog->entry_function) == 0) fprintf(asmfile, "\t.global main\n");
        else fprintf(asmfile, "\t.global %s\n", function_list->name);
        function_list = function_list->next;
    }
    function_list = start_function_list;

    fprintf(asmfile, "\n");

    // Generate code for each function
    while (function_list != NULL){
        if (strcmp(function_list->name, prog->entry_function) == 0) fprintf(asmfile, "main:\n");
        else fprintf(asmfile, "%s:\n", function_list->name);

        ast_statement *statements = function_list->body->data.block.statements;
        while (statements != NULL){

            switch (statements->type) {
                case STMT_RETURN:
                    fprintf(asmfile, "\tmovl $%d, %%eax\n", statements->data.ret.expret->data.int_value);
                    fprintf(asmfile, "\tret\n");
                default: break;
            }
            statements = statements->next;

        }
        if (strcmp(function_list->name, prog->entry_function) == 0) {
            fprintf(asmfile, "\n\tmovq $60, %%rax\n"); // Syscall exit
            fprintf(asmfile, "\txorq %%rdi, %%rdi\n");
            fprintf(asmfile, "\tsyscall\n");
        }
        function_list = function_list->next;
        fprintf(asmfile, "\n");
    }

    fclose(asmfile);
} 

void compile(char *asmfile_name){
    char *command;
    asprintf(&command, "gcc %s -o out",asmfile_name);
    system(command);
}

void test(){
    printf("\nTesting result\n");

    int return_value;
    int status = system("./out");
    if (status == -1){
        perror("system");
    } else {
        if (WIFEXITED(status)){
            return_value = WEXITSTATUS(status);
        }
    }

    printf("Returned: %d\n", return_value);
}

int main(int argc, char *argv[]){
    char buffer[255];
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *srcptr = fopen(argv[1], "r"); // Source file for program
    char *asmfile_name = strdup(argv[1]);
    FILE *asmfile = fopen(strcat(asmfile_name, ".s"), "w");

    if (srcptr == NULL) {
        printf("Error opening file\n");
        return EXIT_FAILURE;
    }

    printf("Compiling file: %s\n", argv[1]);

    token *token_list = lex(srcptr);
    
    ast_program *prog = parse(&token_list);

    generate_asm(prog, asmfile);
    compile(asmfile_name);

    free(prog);
    fclose(srcptr);

    test();
    return 0;
}