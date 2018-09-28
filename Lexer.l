%{
/*
 * Lexer.l file
 * To generate the lexical analyzer run: "flex Lexer.l"
 */


#define YY_DECL int yylex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner,struct roman *p)
#include "expr.h"
#include "rpsc.h"
#include "Parser.h"

#include <stdio.h>
#include <ctype.h>



//extern struct Sheet *cur_sh;


%}

%option outfile="Lexer.c" header-file="Lexer.h"
%option warn nodefault

%option reentrant noyywrap never-interactive nounistd
%option bison-bridge


LPAREN      "("
RPAREN      ")"
PLUS        "+"
MINUS       "-"
MULTIPLY    "*"
DIVIDE      "/"
SUM         "@sum"
IF          "IF"

NUMBER      [0-9]+
WS          [ \r\n\t]*

%%

{WS}        { /* Skip blanks. */ }
{NUMBER}|([0-9]*[.][0-9]+)       { 
    //sscanf(yytext, "%lf", &yylval->value); 
    yylval->value=atof(yytext);
    //printf("Found NUMBER %s %lg\n",yytext,yylval->value); 
    return TOKEN_NUMBER; }

{IF}        { return TOKEN_IF; }

\".*\" {
    yylval->dstr = (char*)calloc(strlen(yytext)-1, sizeof(char));
    strncpy(yylval->dstr, &yytext[1], strlen(yytext)-2);
    return TOKEN_DSTRING;
}


[a-zA-Z]{1,2}[0-9]+:[a-zA-Z]{1,2}[0-9]+ {
    yylval->sym=strdup(yytext);
    return TOKEN_TEXT;
}


[a-zA-Z]{1,2}[0-9]+ {
    char tmp[4];
    int tmpi;
    int len;
    int col;
    int ret;

    len=strlen(yytext);
    convert(&col,&tmpi,yytext,len);

    //printf("FOUND tbl ref. %d %d -> %d %s\n",len,col,tmpi,yytext);
    yylval->ent=lookat(p->cur_sh,tmpi,col);
    //	yylval->ent->col=col;
    //	yylval->ent->row=tmpi;
    return TOKEN_TBL;
}

[a-zA-Z0-9 ]+![a-zA-Z]{1,2}[0-9]+  {
    char tbl[32];
    char ref[32];
    char tmp[4];
    int tmpi;
    int len;
    int col;
    int ret;

    sscanf(yytext,"%32[a-z|A-Z|0-9]!%s",tbl,ref);
    printf("FOUND tbl ref %s %s %s\n", yytext, tbl, ref);

    len=strlen(ref);
    convert(&col,&tmpi,ref,len);

    printf("FOUND tbl ref. %d %d -> %d %s\n", len, col, tmpi, ref);
    yylval->ent=lookat(Search_sheet(p,tbl), tmpi, col);

    return TOKEN_TBL;
}

'[a-zA-Z0-9 ]+'![a-zA-Z]{1,2}[0-9]+   {
    char tbl[32];
    char ref[32];
    char tmp[4];
    int tmpi;
    int len;
    int col;
    int ret;

    sscanf(yytext,"\'%32[^\']\'!%s",tbl,ref);
    printf("FOUND tbl ref %s %s %s\n",yytext,tbl,ref);

    len=strlen(ref);
    convert(&col, &tmpi, ref, len);

    printf("FOUND tbl ref. %d %d -> %d %s\n",len,col,tmpi,ref);
    yylval->ent=lookat(Search_sheet(p,tbl),tmpi,col);

    return TOKEN_TBL;
}


[a-zA-Z_]+  {  int type;
    yylval->sym=search_func(yytext,&type);
    if(yylval->sym != 0) {
        if(type==FUNC_RANGE)
            return TOKEN_FUNCR;
        if(type==FUNC_MATH1)
            return TOKEN_FUNCM1;
    }
    yylval->sym=find_symbol(yytext);
    if(yylval->sym !=0 ) return TOKEN_VAR;
    yylval->sym=strdup(yytext);
    return TOKEN_TEXT;
}

{MULTIPLY}      { return TOKEN_MULTIPLY; }
{DIVIDE}        { return TOKEN_DIVIDE; }
{PLUS}          { return TOKEN_PLUS; }
{MINUS}         { return TOKEN_MINUS; }
{LPAREN}        { return TOKEN_LPAREN; }
{RPAREN}        { return TOKEN_RPAREN; }


.               {  return yytext[0]; }

%%
int yyerror(void *p1, void *p2 , void *p3, const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}
