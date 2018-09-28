/*
 * Expression.c
 * Implementation of functions used to build the syntax tree.
 */

#include "expr.h"
#include "rpsc.h"

#include <stdlib.h>
#include <string.h>


struct Symbol *Symbol_first;

struct Symbol *create_symbol(char *name) {
    struct Symbol * tmp;

    tmp= (struct Symbol *)malloc(sizeof(struct Symbol));
    tmp->name=strdup(name);
    tmp->next=Symbol_first;
    Symbol_first=tmp;

    return tmp;
}


struct Symbol *find_symbol(char *name) {
    struct Symbol *tmp;

    for(tmp=Symbol_first; tmp != ( struct Symbol *)0 ;tmp=tmp->next)
        if (strncmp(name,tmp->name,strlen(tmp->name)) == 0)
            return tmp;

    return( (struct Symbol *) 0);
}


/**
 * @brief Allocates space for expression
 * @return The expression or NULL if not enough memory
 */
static SExpression * allocateExpression() {
    SExpression * b = (SExpression *) malloc(sizeof(SExpression));

    if (b == NULL)
        return NULL;

    b->type = eVALUE;
    b->value = 0;
    b->next=0;

    b->left = NULL;
    b->right = NULL;

    return b;
}

SExpression * createNumber(double value) {
    SExpression * b = allocateExpression();

    if (b == NULL)
        return NULL;

    b->type = eVALUE;
    b->value = value;

    return b;
}


SExpression *createVar(struct Symbol *sym) {
    SExpression *b = allocateExpression();

    if (b == NULL)
        return NULL;

    b->type = eVAR;
    b->ptr = (void *) sym;

    return b;
}


SExpression *createStr( char *sym) {
    SExpression *b = allocateExpression();

    if (b == NULL)
        return NULL;

    b->type = eSTR;
    b->ptr = (void *) strdup(sym);

    return b;
}





SExpression *createEnt(struct Ent *ent) {
    SExpression *b = allocateExpression();

    if (b == NULL)
        return NULL;

    b->type = eENT;
    b->ptr = (void *) ent;

    return b;
}



SExpression *createOperation(EOperationType type, SExpression *left, SExpression *right) {
    SExpression *b = allocateExpression();

    if (b == NULL)
        return NULL;

    b->type = type;
    b->left = left;
    b->right = right;
    b->next=left;
    if(right != 0)
    left->next=right;

    return b;
}



SExpression *createFunction(EOperationType type, void * f, int argn, char **argc) {
    SExpression *b = allocateExpression();
    SExpression *c = allocateExpression();
    SExpression *d = allocateExpression();

    if (b == NULL)
        return NULL;

    b->type = type;
    b->ptr=f;
    b->next=c;
    c->ptr=(void *) argn;
    c->next=d;
    d->ptr=argc;
    return b;
}


SExpression *createFunction2(EOperationType type, void * f,  SExpression *left) {
    SExpression *b = allocateExpression();


    if (b == NULL)
        return NULL;

    b->type = type;
    b->ptr=f;
    b->next=left;

    return b;
}


void deleteExpression(SExpression *b) {
    if (b == NULL)
        return;

    deleteExpression(b->left);
    deleteExpression(b->right);

    free(b);
}
