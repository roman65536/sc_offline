#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


#include "expr.h"
#include "queue.h"
#include "slab.h"

#define NEW 1

#define HASH_NR   64

struct Ent {
    double val;
    char * label;
    char * formula;
    SExpression * exp;
    short flag;
    short col;
    short row;
    struct Ent *n_hash;
};


#define RP_FORMULA  1
#define VAL         2
#define RP_LABEL    4


struct Sheet {
    struct Ent *** tbl;
    void **hash;
    int nr_hash;
    char * name;
    int col, row;
    int ccol , crow;
    short flags;
    int maxcol, maxrow;
    struct Sheet * next;
    struct Sheet * prev;
    struct Objs_cache cache_ent;
};

#define HASH(row,col) (((row*65537)+col) % HASH_NR)

struct roman
{
    char * name;
    int open;
    struct Sheet * first_sh;
    struct Sheet * last_sh;
    struct Sheet * cur_sh;
    struct Ent ** cache;
    int cache_nr;
    int id; //session_id
    struct roman * next; // to chain to other roman structs..
};

#define PLUG_IN 1
#define PLUG_OUT 2
#define PLUG_FUNC 2



struct plugin 
{
 char *name; 		//plugin name
 int type;		//Plugin Type can be ored
 char **ending;		//for auto detection of file .xlsx .html .sc
 struct plugin *next;
 int (*init)(struct plugin *); 	//  called when plugin loaded, for example to initial function calls 
 int (*read)(struct roman *, char *);  //  
 int (*write)(struct roman *, char *);  // 

};


#define FUNC_RANGE      1   /* Range Function */
#define FUNC_MATH1      2   /* Math function 1 arg */
#define FUNC_MATH2      3   /* Math function 2 arg */
#define FUNC_STR        4   /* String function */
struct Functions {
    const char * name;
    double (*func) (struct roman * p, int argc, const char ** argv);
    int type;
    struct Functions * next;
};


#if OLD
#define ATBL(sh, tbl, row, col)     (*(tbl + row) + (col))
#else
#define ATBL(sh,tbl,row,col) atbl(sh,tbl,row,col)
struct Ent ** atbl(struct Sheet *sh, struct Ent ***tbl, int row, int col) ;
#endif


#define GROWNEW         1       /* first time table */
#define GROWROW         2       /* add rows */
#define GROWCOL         3       /* add columns */
#define GROWBOTH        4       /* grow both */

#define LINES       16384
#define RESROW          1
#define MINROWS      2024
#define COLS         1024
#define MINCOLS      2024
#define DEFWIDTH        8
#define GROWAMT        16
#define ABSMAXCOLS   8192
#define rescol          1
#define TRUE            1
#define FALSE           0

#ifndef NULL
#define NULL (0)
#endif

#define BOOL int

#define INSERT(NEW, FIRST,LAST,NEXT,PREV) \
do { \
    if(FIRST == 0) FIRST=LAST=NEW; \
    else { \
    NEW->PREV=LAST; \
    NEW->NEXT=LAST->NEXT; \
    LAST->NEXT=NEW; \
    if(NEW->NEXT !=NULL) NEW->NEXT->PREV=NEW; \
    LAST=NEW; \
    }\
} while(0) 


#define INSERT_BEFORE(NEW, FIRST,LAST,NEXT,PREV) \
do { \
    if(FIRST == 0) FIRST=LAST=NEW; \
    else { \
    NEW->NEXT=FIRST; \
    NEW->PREV=FIRST->PREV;\
    FIRST->PREV=NEW; \
    FIRST=NEW; \
    }\
} while(0)



#define REMOVE(ELEM,FIRST,LAST,NEXT,PREV) \
  do { \
   if(ELEM==FIRST) { \
        FIRST=ELEM->NEXT; \
        if (FIRST == 0)  LAST=0; \
        else ELEM->NEXT->PREV=0; \
   } else { \
     ELEM->PREV->NEXT=ELEM->NEXT; \
     if (ELEM->NEXT ==0) LAST=ELEM->PREV; \
     else ELEM->NEXT->PREV=ELEM->PREV; } \
    } while(0) 




void add_function(char * name, double (*funct) (struct roman *, int, char **), int type);
void * search_func(char * name,int * type);


extern struct Ent * lookat(struct Sheet * sh, int row, int col);
extern struct Sheet * new_sheet(struct roman * doc , char * name);
extern struct Sheet * Search_sheet(struct roman * doc, char * name);
extern int Get_sheets( struct roman *doc, int *len, char **name);
extern void recalc();
extern void export(struct roman * p, FILE *pt ,char * start, char * end, char * start_col, char * end_col, char * start_row, char * end_row);

SExpression * parse(char ** ptr);
SExpression * getAST(const char * expr, struct roman * p);
