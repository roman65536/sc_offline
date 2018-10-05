%{
/*
 * Parser.y file
 * To generate the parser run: "bison Parser.y"
 */

#define YY_DECL int yylex (YYSTYPE * yylval_param, yyscan_t yyscanner, struct roman *p)

#include "rpsc.h"
#include "expr.h"
#include "Parser.h"
#include "Lexer.h"
#include <stdio.h>

int yyerror(SExpression **expression, yyscan_t scanner, struct roman *p, const char *msg) {
// Add error handling routine as needed
printf("Error: [%s]  \n",msg);
}

SExpression *createEnt(struct Ent *ent);
#define YYDEBUG 1
#define YYERROR_VERBOSE 1
%}

%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

}

%output  "Parser.c"
%defines "Parser.h"

%define parse.error verbose
%define api.pure
%lex-param   { yyscan_t scanner } { struct roman *p }

%parse-param { SExpression ** expression }
%parse-param { yyscan_t scanner }
%parse-param { struct roman *p }

%union {
    double value;
    SExpression *expression;
    struct Symbol *sym;
    struct Ent *ent;
    char *dstr;
}

%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_PLUS
%token TOKEN_MINUS
%token TOKEN_MULTIPLY
%token TOKEN_DIVIDE
%token TOKEN_SUM
%token TOKEN_IF
%token <ent> TOKEN_TBL
%token <value> TOKEN_NUMBER
%token <sym> TOKEN_VAR
%token <sym> TOKEN_FUNCR
%token <sym> TOKEN_FUNCM1
%token <sym> TOKEN_TEXT
%token <dstr> TOKEN_DSTRING

%type <expression> expr

%left TOKEN_PLUS TOKEN_MINUS
%left TOKEN_MULTIPLY TOKEN_DIVIDE

%%
input
    : expr { *expression = $1; }
    ;

expr
    : expr TOKEN_PLUS expr                                      { $$ = createOperation(ePLUS, $1, $3 ); }
    | expr TOKEN_MINUS expr                                     { $$ = createOperation(eMINUS, $1, $3 ); }
    | expr TOKEN_MULTIPLY expr                                  { $$ = createOperation(eMULTIPLY, $1, $3 ); }
    | expr TOKEN_DIVIDE expr                                    { $$ = createOperation(eDIVIDE, $1, $3 ); }
    | expr '>' expr                                             { $$ = createOperation('>', $1, $3); }
    | expr '<' expr                                             { $$ = createOperation('<', $1, $3); }
    | expr '=' expr                                             { $$ = createOperation('=', $1, $3); }
    | expr '^' expr                                             { $$ = createOperation('^', $1, $3); }
    | expr '=' TOKEN_DSTRING                                    { $$ = createOperation('s', $1, $3); }
    | TOKEN_IF TOKEN_LPAREN expr ',' expr ',' expr TOKEN_RPAREN { $$ = createOperation(eIF, $3, 0);
                                                                  $$->if_t = createOperation(eMORE, $5, $7);
                                                                }
    | TOKEN_LPAREN expr TOKEN_RPAREN                            { $$ = $2; }
    | TOKEN_NUMBER                                              { $$ = createNumber($1); }
    | TOKEN_TBL                                                 { $$ = createEnt($1); }
    | TOKEN_VAR                                                 { $$ = createVar($1); }
    | TOKEN_FUNCR TOKEN_LPAREN TOKEN_TEXT TOKEN_RPAREN          { int argn; char ** argv;
      //printf("Function %p %s %p\n",$1,$3,&argv[0]);
                                                                  makeArgs($3, &argn, &argv);
                                                                  $$ = createFunction('f', $1, argn, argv);
                                                                }
    | TOKEN_FUNCM1 TOKEN_LPAREN expr TOKEN_RPAREN               { $$ = createFunction2('m', $1, $3);
 }

    ;
%%
