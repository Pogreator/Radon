#ifndef __ASTNODES_H__
#define __ASTNODES_H__

#include <malloc.h>
#include <string.h>

typedef struct ast_node ast_node;
typedef struct ast_statement ast_statement;
typedef struct ast_expression ast_expression;
typedef struct ast_function ast_function;
typedef struct ast_program ast_program;

typedef enum { 
    FUNC_VOID, FUNC_INT
}function_return_type;

typedef enum { 
    EXP_BINARY, EXP_INT_LITERAL,  EXP_FLOAT_LITERAL 
}expression_type;

typedef enum { 
    STMT_EXPRESSION, STMT_RETURN, STMT_BLOCK
}statement_type;

typedef enum { 
    NODE_FUNC_DEF, NODE_STMT 
} node_type;


typedef struct ast_function {
    function_return_type type;
    char name[64];
    char **arguments;
    int argument_count;
    ast_statement *body;
    ast_function *next;
    ast_function *previous;
}ast_function;

typedef struct ast_expression {
    expression_type type;
    union{
        struct {
            char op[64];
            ast_expression *left;
            ast_expression *right;
        } binary;

        int int_value;
        float float_value;
    }data;
}ast_expression;

typedef struct ast_statement {
    statement_type type;
    union{
        struct {struct ast_expression *exp;} expression;
        struct {struct ast_statement *statements;} block;
        struct {struct ast_expression *expret;} ret;
    }data;
    struct ast_statement *next;
}ast_statement;

typedef struct ast_node {
    node_type type;
    union{
        struct {struct ast_function *functions;} function_data;
    }data;
}ast_node;

typedef struct ast_program {
    char entry_function[64];
    ast_node *first_item;
} ast_program;

// Helpers

inline ast_expression *create_int_literal_expression(int value) {
    ast_expression *exp = (ast_expression *)malloc(sizeof(ast_expression));
    exp->type=EXP_INT_LITERAL;
    exp->data.int_value=value;
    return exp;
};

inline ast_expression *create_binary_expression(char oper[64], ast_expression *left_exp, ast_expression *right_exp) {
     ast_expression *exp = (ast_expression *)malloc(sizeof(ast_expression));
    exp->type=EXP_BINARY;
    strcpy(exp->data.binary.op,oper);
    exp->data.binary.left=left_exp;
    exp->data.binary.right=right_exp;
    return exp;
};

#endif // __ASTNODES_H__