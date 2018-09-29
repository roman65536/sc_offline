#include <string.h>
#include <math.h>

#include "Parser.h"
#include "Lexer.h"

double do_sum(struct roman *p, int argn, char ** argc);
double do_prod(struct roman *p, int argn, char ** argc);
double do_cnt(struct roman *p,int argn, char ** argc);
double do_min(struct roman * p, int argn, char ** argc);
double do_max(struct roman * p, int argn, char ** argc);
double do_avg(struct roman *p, int argn, char ** argc);
double do_stdev(struct roman *p, int argn, char ** argc);
