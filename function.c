#include <string.h>
#include <math.h>

#include "rpsc.h"
#include "Parser.h"
#include "Lexer.h"
#include "sheet.h"

struct Functions *first_func=0;

void add_function(char *name, double (*funct)(struct roman *, int,char **), int type) {

    struct Functions *func=(struct Functions *) malloc(sizeof(struct Functions));
    func->name = name;
    func->func=funct;
    func->type=type;
    func->next=first_func;
    first_func=func;

}

void * search_func(char *name, int *type) {
    struct Functions *func;

    for(func=first_func; func != 0; func=func->next)
        if(strcmp(name,func->name) == 0){
            *type=func->type;
            return (void *) func->func;
        }
    return (void *)0;
}

double do_sum(struct roman *p, int argn, char ** argc) {
    double sum=0.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];

    struct Sheet *sh = p->cur_sh;

    // printf("%s: %s %s\n",__FUNCTION__,p->cur_sh->name,argc[0]);
    int a = sscanf(argc[0], "%[0-9a-zA-Z]:%[0-9a-zA-Z]", from, to);
    if (a!=2)
        a = sscanf(argc[0], "%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]", sheet, from, to);
    if (a == 3) {
        sh = Search_sheet(p,sheet);
    }

    convert(&col1, &row1, from ,strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for(; col1 <= col2; col1++)
        for(; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if ((p->flag & VAL) == VAL)
                sum += p->val;
        }
    return sum;
}

double do_prod(struct roman *p, int argn, char **argc) {
    double sum = 1.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];

    struct Sheet * sh = p->cur_sh;

    int a = sscanf(argc[0], "%[0-9a-zA-Z]:%[0-9a-zA-Z]", from, to);
    if(a!=2)
        a = sscanf(argc[0], "%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]", sheet, from, to);
    if(a == 3) {
        sh = Search_sheet(p, sheet);
    }

    convert(&col1, &row1, from, strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for(; col1 <= col2; col1++)
        for(; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if ((p->flag & VAL) == VAL)
                sum *= p->val;
        }
    return sum;
}


double do_cnt(struct roman *p,int argn, char **argc) {

    double sum = 1.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];
    int cnt = 0;

    struct Sheet * sh = p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1, &row1, from, strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for( ; col1 <= col2; col1++)
        for( ; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if ((p->flag & VAL) == VAL)
                cnt++;
        }
    return cnt;
}



double do_min(struct roman * p, int argn, char ** argc) {

    double sum = 0.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];
    int cnt = 0;

    struct Sheet * sh = p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1, &row1, from, strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for(; col1 <= col2; col1++)
        for(; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if ((p->flag & VAL) == VAL) {
                if (! cnt) {
                    sum= p->val;
                    cnt ++;
                } else if (p->val < sum)
                    sum = p->val;
            }
        }
    return sum;
}


double do_max(struct roman * p, int argn, char ** argc) {

    double sum=0.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];
    int cnt = 0;

    struct Sheet *sh = p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]", from, to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet, from, to);
    if(a == 3) {
        sh = Search_sheet(p, sheet);
    }

    convert(&col1, &row1, from, strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for( ; col1 <= col2; col1++)
        for( ; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if ((p->flag & VAL) == VAL) {
                if (! cnt) {
                    sum = p->val;
                    cnt ++;
                } else if (p->val > sum)
                    sum = p->val;
            }
        }

    return sum;
}


double do_avg(struct roman *p, int argn, char **argc) {
    double sum = 0.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];
    int cnt=0;

    struct Sheet * sh = p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh = Search_sheet(p, sheet);
    }

    convert(&col1, &row1, from, strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for( ; col1 <= col2; col1++)
        for( ; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if ((p->flag & VAL) == VAL) {

                sum +=  p->val;
                cnt ++;
            }
        }

    return sum / (double) cnt;
}

double do_stdev(struct roman *p, int argn, char **argc) {
    double lp, rp, v, nd;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];
    int cnt = 0;

    lp = 0.0;
    rp = 0.0;

    struct Sheet *sh = p->cur_sh;

    int a = sscanf(argc[0], "%[0-9a-zA-Z]:%[0-9a-zA-Z]", from, to);
    if(a!=2)
        a = sscanf(argc[0], "%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]", sheet, from, to);
    if(a == 3) {
        sh = Search_sheet(p, sheet);
    }

    convert(&col1, &row1, from, strlen(from));
    convert(&col2, &row2, to, strlen(to));

    for( ; col1 <= col2; col1++)
        for( ; row1 <= row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL) {

                v=  p->val;
                lp += v*v;
                rp += v;

                cnt++;
            }
        }

    if ((cnt == 0) || (cnt == 1))
        return ((double)0);
    nd = (double)cnt;
    //printf("%s: %g %g %g\n",__FUNCTION__,lp,rp,nd);
    //printf("%g %g %g\n",(nd*lp-rp*rp),(nd*(nd-1)), (nd*lp-rp*rp)/(nd*(nd-1)) );

    return (sqrt((nd*lp-rp*rp)/(nd*(nd-1))));
}




int init_lib()
{
    add_function("SUM", &do_sum, FUNC_RANGE);
    add_function("PRODUCT", &do_prod, FUNC_RANGE);
    add_function("MIN", &do_min, FUNC_RANGE);
    add_function("MAX", &do_max, FUNC_RANGE);
    add_function("AVERAGE", &do_avg, FUNC_RANGE);
    add_function("CNT", &do_cnt, FUNC_RANGE);
    add_function("COUNT", &do_cnt, FUNC_RANGE);
    add_function("STDEV", &do_stdev, FUNC_RANGE);
    add_function("SIN", &sin, FUNC_MATH1);
    add_function("LOG", &log10, FUNC_MATH1);
    add_function("EXP", &exp, FUNC_MATH1);
    add_function("LN", &log, FUNC_MATH1);

}

