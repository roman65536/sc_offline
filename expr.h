/*
 * Expression.h
 * Definition of the structure used to build the syntax tree.
 */
#ifndef __EXPRESSION_H__
#define __EXPRESSION_H__

/**
 * @brief The operation type
 */
typedef enum tagEOperationType
{
    eVALUE,
    eMULTIPLY,
    eDIVIDE,
    ePLUS,
    eMINUS,
    eSUM,
    eIF,
    eVAR,
    eENT,
    eSTR,
    eMORE
} EOperationType;

/**
 * @brief The expression structure
 */
typedef struct tagSExpression
{
    EOperationType type;///< type of operation

    double value;///< valid only when type is eVALUE
    void *ptr;
    struct tagSExpression *left; ///< left side of the tree
    struct tagSExpression *right;///< right side of the tree
    struct tagSExpression *next;
    struct tagSExpression *if_t;
} SExpression;


struct Symbol
{
    char *name;
    int type;
    double val;
    struct Symbol *next;
};

/*
#define ATBL(tbl, row, col)     (*(tbl + row) + (col))
*/

extern struct Symbol *Symbol_first;
struct Symbol *create_symbol(char *name);
struct Symbol *find_symbol(char *name);


/**
 * @brief It creates an identifier
 * @param value The number value
 * @return The expression or NULL in case of no memory
 */
SExpression *createNumber(double value);
SExpression *createVar(struct Symbol *sym);
//SExpression *createEnt(struct Ent *ent);

/**
 * @brief It creates an operation
 * @param type The operation type
 * @param left The left operand
 * @param right The right operand
 * @return The expression or NULL in case of no memory
 */
SExpression *createOperation(EOperationType type, SExpression *left, SExpression *right);
SExpression *createFunction(EOperationType type, void * f, int argn, char **argc);
SExpression *createFunction2(EOperationType type, void * f,  SExpression *left);

/**
 * @brief Deletes a expression
 * @param b The expression
 */
void deleteExpression(SExpression *b);

#endif // __EXPRESSION_H__
