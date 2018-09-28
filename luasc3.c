#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define LUA_LIB
#define LUA_SC   "SC210968*"
#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"



#include "rpsc.h"
#include "xlsx.h"


#include "Parser.h"
#include "Lexer.h"



int growtbl(struct Sheet * sh,int rowcol, int toprow, int topcol);

int convert( int *col, int *row , char * s, int size) {
    int val, i;
    int temp;
    int val2;

    temp = 0;
    val = 0;
    val2=0;

    for (i=0; (i<size) && (s[i] != '\0'); i++) {
        if (((s[i]<='Z') && (s[i]>='A')) || ((s[i]<='z') && (s[i]>='a'))) {
            if (islower(s[i]))
                s[i] = toupper(s[i]);
            val = (temp*26) + ( s[i]-'A');
            temp = temp + 1;
            *col=val;
        }
        else if ((s[i]<='9') && (s[i]>='0'))
        {
            val2 = (val2*10) + (s[i]-'0');
            *row=val2;
        }
    }
    return (val);
}

int coltoa(int col, char * rname) {
    register char * p = rname;

    if (col > 25) {
        *p++ = col/26 + 'A' - 1;
        col %= 26;
    }
    *p++ = col+'A';
    *p = '\0';
    return 0;
}

double do_sum(struct roman *p, int argn, char ** argc) {
    double sum=0.0;
    int row1, col1, row2, col2;
    char sheet[32], from[10], to[10];

    struct Sheet *sh=p->cur_sh;

    // printf("%s: %s %s\n",__FUNCTION__,p->cur_sh->name,argc[0]);
    int a = sscanf(argc[0], "%[0-9a-zA-Z]:%[0-9a-zA-Z]", from, to);
    if(a!=2)
        a = sscanf(argc[0], "%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]", sheet, from, to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(; col1<=col2; col1++)
        for(; row1<=row2; row1++) {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
                sum += p->val;
        }
    return sum;
}

double do_prod(struct roman *p, int argn, char **argc) {
    double sum=1.0;
    int row1,col1,row2,col2;
    char sheet[32],from[10],to[10];

    struct Sheet *sh=p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(;col1<=col2;col1++)
        for(;row1<=row2;row1++)
        {
            struct Ent *p=lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
                sum*=  p->val;
        }
    return sum;
}


double do_cnt(struct roman *p,int argn, char **argc)
{

    double sum=1.0;
    int row1,col1,row2,col2;
    char sheet[32],from[10],to[10];
    int cnt=0;

    struct Sheet *sh=p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(;col1<=col2;col1++)
        for(;row1<=row2;row1++)
        {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
                cnt++;
        }
    return cnt;
}



double do_min(struct roman *p,int argn, char **argc)
{

    double sum=0.0;
    int row1,col1,row2,col2;
    char sheet[32],from[10],to[10];
    int cnt=0;

    struct Sheet *sh=p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(;col1<=col2;col1++)
        for(;row1<=row2;row1++)
        {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
            {
                if (! cnt) {
                    sum= p->val;
                    cnt ++;
                } else if (p->val < sum) 
                    sum=  p->val;
            }
        }

    return sum;
}


double do_max(struct roman *p,int argn, char **argc)
{

    double sum=0.0;
    int row1,col1,row2,col2;
    char sheet[32],from[10],to[10];
    int cnt=0;

    struct Sheet *sh=p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(;col1<=col2;col1++)
        for(;row1<=row2;row1++)
        {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
            {
                if (! cnt) {
                    sum= p->val;
                    cnt ++;
                } else if (p->val > sum)
                    sum=  p->val;
            }
        }

    return sum;
}


double do_avg(struct roman *p, int argn, char **argc) {
    double sum=0.0;
    int row1,col1,row2,col2;
    char sheet[32],from[10],to[10];
    int cnt=0;

    struct Sheet *sh=p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(;col1<=col2;col1++)
        for(;row1<=row2;row1++)
        {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
            {

                sum+=  p->val;
                cnt++;
            }
        }

    return sum/(double)cnt;
}

double do_stdev(struct roman *p, int argn, char **argc) {
    double lp, rp, v, nd;
    int row1,col1,row2,col2;
    char sheet[32],from[10],to[10];
    int cnt=0;

    lp=0.0;
    rp=0.0;

    struct Sheet *sh=p->cur_sh;

    int a = sscanf(argc[0],"%[0-9a-zA-Z]:%[0-9a-zA-Z]",from,to);
    if(a!=2)
        a = sscanf(argc[0],"%[0-9a-zA-Z_]!%[0-9a-zA-Z]:%[0-9a-zA-Z]",sheet,from,to);
    if(a == 3) {
        sh=Search_sheet(p,sheet);
    }

    convert(&col1,&row1,from,strlen(from));
    convert(&col2,&row2,to,strlen(to));

    for(;col1<=col2;col1++)
        for(;row1<=row2;row1++)
        {
            struct Ent * p = lookat(sh, row1, col1);
            if((p->flag & VAL) == VAL)
            {

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


void checkbounds(struct Sheet *sh, int *rowp, int *colp) {
    if (*rowp < 0)
        *rowp = 0;
    else if (*rowp >= sh->crow) {
        if (*colp >= sh->ccol) {
            if (!growtbl(sh,GROWBOTH, *rowp, *colp)) {
                *rowp = sh->crow - 1;
                *colp = sh->ccol - 1;
            }
            return;
        } else {
            if (!growtbl(sh,GROWROW, *rowp, 0))
                *rowp = sh->crow - 1;
            return;
        }
    }
    if (*colp < 0)
        *colp = 0;
    else if (*colp >= sh->ccol) {
        if (!growtbl(sh,GROWCOL, 0, *colp))
            *colp = sh->ccol - 1;
    }
}


#define GROWALLOC(newptr, oldptr, nelem, type, msg) \
    newptr = (type *)realloc((void *)oldptr, \
            (size_t)(nelem * sizeof(type))); \
    if (newptr == ((type *)NULL)) { \
        //error(msg);		  \
        return (FALSE); \
    } \
    oldptr = newptr /* wait incase we can't alloc */




struct Ent * lookat(struct Sheet * sh, int row, int col) {
    register struct Ent ***tbl=sh->tbl;
    register struct Ent **pp;

    checkbounds(sh,&row, &col);
    pp = ATBL(tbl, row, col);
    if (*pp == NULL) {
        *pp = (struct Ent *) calloc(1,(unsigned)sizeof(struct Ent));
    if (row > sh->row) sh->row = row;
    if (col > sh->col) sh->col = col;
    (*pp)->label = (char *)0;
    (*pp)->formula=(char *)0;
    (*pp)->flag=0;
    (*pp)->row = row;
    (*pp)->col = col;
    (*pp)->val = (double) 0.0;
    }
    return (*pp);
}


int growtbl(struct Sheet * sh,int rowcol, int toprow, int topcol) {
    int *fwidth2;
    int *precision2;
    int *realfmt2;
    int newcols;
#ifndef PSC
    struct Ent ***tbl2;
    struct Ent ** nullit;
    int cnt;
    char *col_hidden2;
    char *row_hidden2;
    int newrows;
    int i;

    newrows = sh->crow;
#endif /* !PSC */
    newcols = sh->ccol;
    if (rowcol == GROWNEW) {
#ifndef PSC
        sh->crow = toprow = 0;
        /* when we first start up, fill the screen w/ cells */
        {   int startval;
            startval = LINES - RESROW;
            newrows = startval > MINROWS ? startval : MINROWS;
            startval = ((COLS) - rescol) / DEFWIDTH;
            newcols = startval > MINCOLS ? startval : MINCOLS;
        }
#else
        newcols = MINCOLS;
#endif /* !PSC */
        sh->ccol = topcol = 0;
    }
#ifndef PSC
    /* set how much to grow */
    if ((rowcol == GROWROW) || (rowcol == GROWBOTH)) {
        if (toprow > sh->crow)
            newrows = GROWAMT + toprow;
        else
            newrows += GROWAMT;
    }
#endif /* !PSC */
    if ((rowcol == GROWCOL) || (rowcol == GROWBOTH)) {
        if ((rowcol == GROWCOL) && ((sh->ccol == ABSMAXCOLS) ||
                    (topcol >= ABSMAXCOLS))) {
            //	    error(nowider);
            printf("%s Couldn't grow\n",__FUNCTION__);
            return (FALSE);
        }

        if (topcol > sh->ccol)
            newcols = GROWAMT + topcol;
        else
            newcols += GROWAMT;

        if (newcols > ABSMAXCOLS)
            newcols = ABSMAXCOLS;
    }

#ifndef PSC
    if ((rowcol == GROWROW) || (rowcol == GROWBOTH) || (rowcol == GROWNEW)) {
        struct	ent *** lnullit;
        int	lcnt;

        //GROWALLOC(row_hidden2, row_hidden, newrows, char, nolonger);
        //memset(row_hidden+sh->row, 0, (newrows-sh->row)*sizeof(char));

        /*
         * alloc tbl row pointers, per net.lang.c, calloc does not
         * necessarily fill in NULL pointers
         */
        GROWALLOC(sh->tbl, sh->tbl, newrows, struct Ent ** , nolonger);
        for (lnullit = sh->tbl+sh->crow, lcnt = 0; lcnt < newrows-sh->crow;
                lcnt++, lnullit++)
            *lnullit = (struct ent **)NULL;
        /*	memset(tbl+maxrows, (char *)NULL, (newrows-maxrows)*(sizeof(struct ent **)));*/
    }
#endif /* !PSC */



    /*    
          if ((rowcol == GROWCOL) || (rowcol == GROWBOTH) || (rowcol == GROWNEW)) {
          GROWALLOC(fwidth2, fwidth, newcols, int, nowider);
          GROWALLOC(precision2, precision, newcols, int, nowider);
          GROWALLOC(realfmt2, realfmt, newcols, int, nowider);
//#ifdef PSC
memset(fwidth+maxcols, 0, (newcols-maxcols)*sizeof(int));
memset(precision+maxcols, 0, (newcols-maxcols)*sizeof(int));
memset(realfmt+maxcols, 0, (newcols-maxcols)*sizeof(int));
}
#else
GROWALLOC(col_hidden2, col_hidden, newcols, char, nowider);
memset(col_hidden+maxcols, 0, (newcols-maxcols)*sizeof(char));
for (i = maxcols; i < newcols; i++) {
fwidth[i] = DEFWIDTH;
precision[i] = DEFPREC;
realfmt[i] = DEFREFMT;
}

#endif
*/
/* [re]alloc the space for each row */
for (i = 0; i < sh->crow; i++) {
    if ((sh->tbl[i] = (struct Ent **)realloc((char *)sh->tbl[i],
                    (unsigned)(newcols * sizeof(struct Ent **)))) == (struct Ent **)0) {
        //	    error(nowider);
        return(FALSE);
    }
    for (nullit = ATBL(sh->tbl, i, sh->ccol), cnt = 0;
            cnt < newcols-sh->ccol; cnt++, nullit++)
        *nullit = (struct Ent *)NULL;
    memset((char *)ATBL(sh->tbl,i,sh->ccol), 0,
            (newcols-sh->ccol)*sizeof(struct ent **));

}
}
else
i = sh->crow;

/* fill in the bottom of the table */
for (; i < newrows; i++) {
    if ((sh->tbl[i] = (struct Ent **)calloc(1,(unsigned)(newcols *
                        sizeof(struct Ent **)))) == (struct Ent **)0) {
        //error(nowider);
        return(FALSE);
    }
    for (nullit = sh->tbl[i], cnt = 0; cnt < newcols; cnt++, nullit++)
        *nullit = (struct Ent *)NULL;
    memset((char *)sh->tbl[i], 0, newcols*sizeof(struct ent **));
}

//FullUpdate++;
sh->crow = newrows;

//  if (maxrows > 1000) rescol = 5;
//  if (maxrows > 10000) rescol = 6;
//#endif /* PSC */

sh->ccol = newcols;
return (TRUE);
}

struct Sheet * new_sheet(struct roman * doc, char * name) {
    struct Sheet * sh;
    sh = (struct Sheet *) calloc(1, sizeof(struct Sheet));
    INSERT(sh, (doc->first_sh), (doc->last_sh), next, prev);
    sh->name = strdup(name);
    sh->tbl = 0;
    sh->maxcol = sh->maxrow = 0;
    sh->ccol = 16;
    sh->crow = 32768;

    return sh;
}

struct Sheet *Search_sheet(struct roman *doc,char *name) {
    struct Sheet *sh;

    for(sh=doc->first_sh; sh != 0; sh=sh->next) {
        printf("sheet: %s %s\n", name, sh->name);
        if(! strcmp(name,sh->name) ) return sh;
    }

    return 0;
}




double eval(struct roman *pp,SExpression *s, struct Ent *p) {

  double (*func)(struct roman *,int, char **);
  double (*funct)(double);
  switch (s->type)
    {
    case eVALUE: return s->value;
    case ePLUS:  return eval(pp,s->left,p)+eval(pp,s->right,p); break;
    case eMINUS:  return eval(pp,s->left,p)-eval(pp,s->right,p); break;
    case eMULTIPLY:  return eval(pp,s->left,p)*eval(pp,s->right,p); break;
    case eDIVIDE:  return eval(pp,s->left,p)/eval(pp,s->right,p); break;
    case '>':      return (eval(pp,s->left,p)>eval(pp,s->right,p)); break;
    case '<':      return (eval(pp,s->left,p)<eval(pp,s->right,p)); break;
    case '=':      return eval(pp,s->left,p)==eval(pp,s->right,p); break;
    case '^':      return pow(eval(pp,s->left,p),eval(pp,s->right,p)); break;
    case eIF:      return eval(pp,s->left,p)?eval(pp,s->if_t->next,p):eval(pp,s->if_t->next->next,p); break;

    case 's':
                   {
                       char * str=(char *)s->right;
                       struct Ent *rp=s->left->ptr;
                       if(rp->label == 0) return 0;
                       return strcmp(rp->label,str)?0:1;
                   }
                   break;

    case 'f':

                   func=s->ptr;
                   return func(pp,s->next->ptr,s->next->next->ptr);
                   break;

    case'm':
                   funct=s->ptr;
                   return funct(eval(pp,s->next,p));
                   break;
    case eENT:
                   {
                       struct Ent *ppd=(struct Ent *)s->ptr;

                       if (ppd==p ) {
                           //printf("Recurssion detected\n");
                           return p->val;
                       }
                       /*
                          if ((pp)->exp != 0 )
                          return eval((pp)->exp,pp);
                          else */
                       return ppd->val;
                   }
                   break;
    }
}


void recalc(struct roman *p) {
    register struct Ent **pp;
    int x, y;
    int a=0;


    if((p->cache_nr !=0 )) {
        for(a=0;a<p->cache_nr;a++)
        {
            p->cache[a]->val=eval(p,(p->cache[a])->exp,p->cache[a]);	
        }
    }
    else{

        for(p->cur_sh=p->first_sh;p->cur_sh != 0;p->cur_sh=p->cur_sh->next)
        {
            for(x=0;x<=p->cur_sh->col;x++)
                for(y=0;y<=p->cur_sh->row;y++)
                {
                    pp = ATBL(p->cur_sh->tbl, y, x);
                    if(*pp != 0)
                    {
                        //printf("%s1 %s %d %d  %x val : %f\n",__FUNCTION__,p->cur_sh->name,x,y,(*pp)->flag, (*pp)->val);
                        if (((*pp)->flag & RP_FORMULA) == RP_FORMULA)
                        {
                            int sz=0;
                            char *ptr=(*pp)->formula;
                            if ((*pp)->exp !=0 ) {
                                (*pp)->val=eval(p,(*pp)->exp, *pp);
                                //	    printf("%s2 %s %d %d val : %f\n",__FUNCTION__,p->cur_sh->name,x,y,(*pp)->val);
                                sz=sizeof(struct Ent *);
                                //printf("%s %d %p\n",__FUNCTION__,sz*a,p->cache);
                                if((sz*a) < 4096) sz=4096;
                                p->cache=realloc((void *)p->cache,sz);
                                //printf("%s1 %d %p\n",__FUNCTION__,sz*a,p->cache);
                                p->cache[a]=*pp;
                                a++;
                            }
                        }

                    }

                }
            //printf("%s %d\n",__FUNCTION__,a);

        }
        p->cache_nr=a;
    }

    p->cur_sh=p->first_sh;
}



void export(struct roman *p,char *start,char *end,char*start_col,char*end_col,char*start_row,char*end_row) {
    register struct Ent **pp;
    int x,y;
    int a=0;
    char tmp[3];


    if (start !=0 ) printf("%s",start);

    for (x=0;x<p->cur_sh->col;x++) {
        if (start_col != 0) printf("%s",start_col);
        for (y=0;y<p->cur_sh->row;y++) {
            coltoa(y,tmp);
            if (start_row != 0) printf("%s %s",start_row,tmp);
            pp = ATBL(p->cur_sh->tbl,y,x);
            if (*pp != 0) {
                if (((*pp)->flag & RP_FORMULA) == RP_FORMULA) {
                    char *ptr=(*pp)->formula;
                    printf("%g",(*pp)->val);	

                }
                if (((*pp)->flag & RP_LABEL) == RP_LABEL) printf("%s",(*pp)->label);

            }
            if (end_row != 0) printf("%s",end_row);

        }
        if (end_col != 0) printf("%s",end_col);

    }
    if(end !=0 ) printf("%s",end);
}


SExpression *getAST(const char * expr, struct roman * p) {

    SExpression *expression=0;

    yyscan_t scanner;
    YY_BUFFER_STATE state;

    if (yylex_init(&scanner)) {
        // couldn't initialize
        return NULL;
    }

    //printf("PARSING: [%s]\n",expr);
    state = yy_scan_string(expr, scanner);

    if (yyparse(&expression, scanner,p)) {
        // error parsing
        return NULL;
    }

    yy_delete_buffer(state, scanner);

    yylex_destroy(scanner);

    return expression;
}





io_open(lua_State *L)
{
}

io_close(lua_State *L)
{
}

static int f_gc (lua_State *L) {
    struct roman *p = ((struct roman *)luaL_checkudata(L, 1, LUA_SC)); 
    printf("%s %s %d\n",__FUNCTION__,p->name,p->open);

    return 0;
}

io_sheet(lua_State *L) {
    struct Sheet *sh;
    const char *name = luaL_checkstring(L, 2);
    struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    // printf("%s %s %s\n",__FUNCTION__,p->name,name);
    sh=Search_sheet(p,name);
    p->cur_sh=sh;
    return 0;
}


io_newsheet(lua_State *L) {
    struct Sheet *sh;
    const char *name = luaL_checkstring(L, 2);
    struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    printf("%s %s %s\n",__FUNCTION__,p->name,name);
    sh=new_sheet(p,name);
    p->cur_sh=sh;
    return 0;
}

io_loadxl(lua_State *L) {
    struct Sheet *sh;
    const char *name = luaL_checkstring(L, 2);
    struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    printf("%s %s %s\n",__FUNCTION__,p->name,name);
    open_xlsx(p,name,"");

    return 0;
}

/*
   io_loadxls(lua_State *L)
   {
   struct Sheet *sh;
   const char *name = luaL_checkstring(L, 2);
   struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
   printf("%s %s %s\n",__FUNCTION__,p->name,name);
   open_xls(p,name,"");

   return 0;
   }
   */

io_getsheets(lua_State *L) {
    struct Sheet *sh;
    int i=1;

    struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    printf("%s %s\n",__FUNCTION__,p->name);
    if(p->first_sh == 0) {  /* error opening the directory? */
        lua_pushnil(L);  /* return nil and ... */
        lua_pushstring(L, "No Sheets");  /* error message */
        return 2;  /* number of results */
    }
    lua_newtable(L);
    for(sh=p->first_sh;sh !=0; sh=sh->next)
    {
        lua_pushnumber(L, i++);  /* push key */
        lua_pushstring(L, sh->name);  /* push value */
        lua_settable(L, -3);
    }

    return 1;
}

static int io_recalc(lua_State *L)
{
    struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    recalc(p);
    return 0;
}


static int l_getnum (lua_State *L) {
    int r,c;
    struct Ent **pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    c = lua_tointeger(L, 2);      /* get argument */
    r = lua_tointeger(L, 3);
    // sc_debug("getnum !!");
    pp = ATBL(d->cur_sh->tbl,r,c);

    p = *pp;
    if (p == 0) return 0;
    if (p->flag & VAL) {
        //printf("%s val %f",__FUNCTION__,p->val);
        lua_pushnumber(L, p->val);  /* push result */
        return 1;                 /* number of results */
    } else return 0;
}

static int l_setnum (lua_State *L) {
    int r,c;
    double val;
    //struct ent ** pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    c = lua_tointeger(L, 2);  /* get argument */
    r = lua_tointeger(L, 3);
    val=lua_tonumber(L,4);
    //sc_debug("getnum !!");

    p=lookat(d->cur_sh,r,c);
    //printf("%s  %s %d %d old val %g",__FUNCTION__,cur_sh->name, r,c, p->val);
    p->val=val;
    p->flag |= VAL;
    //printf("%s new val %g",__FUNCTION__,p->val);

    return 0;
}

static int l_setstr (lua_State *L) {
    int r,c;
    char * val;
    //struct ent ** pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    c = lua_tointeger(L, 2);  /* get argument */
    r = lua_tointeger(L, 3);
    val=(char *) lua_tostring(L,4);
    //sc_debug("setstr !!");

    p=lookat(d->cur_sh,r,c);
    p->label=strdup(val);
    p->flag |= RP_LABEL;

    return 0;
}


static int l_getstr (lua_State *L) {
    int r,c;

    //struct ent ** pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    c = lua_tointeger(L, 2);  /* get argument */
    r = lua_tointeger(L, 3);

    //sc_debug("setstr !!");

    p=lookat(d->cur_sh,r,c);
    if(p == 0) return 0;
    if(p->label !=0) {
        lua_pushstring(L,p->label);
        return 1;
    }

    return 0;
}

static int l_export_html (lua_State *L) {
    int r,c;

    //struct ent ** pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));

    //sc_debug("setstr !!");

    export(d,"<table>","</table>\n","<tr>","</tr>\n","<td>","</td>");

    return 0;
}

static int l_maxcol (lua_State *L) {
    int r,c;
    struct Ent **pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));

    lua_pushnumber(L, d->cur_sh->maxcol);  /* push result */
    return 1;                 /* number of results */
}


static int l_maxrow (lua_State *L) {
    int r,c;
    struct Ent **pp;
    struct Ent *p;
    struct roman *d= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));

    lua_pushnumber(L, d->cur_sh->maxrow);  /* push result */
    return 1;                 /* number of results */
}


static const luaL_Reg iolib[] = {
    {"open", io_open},
    {NULL, NULL}
};

static const luaL_Reg flib[] = {
    {"close", io_close},
    {"sheet", io_sheet},
    {"newsheet", io_newsheet},
    {"loadxlsx", io_loadxl},
    // {"loadxls", io_loadxls},
    {"getsheets",io_getsheets},
    {"recalc",io_recalc},
    {"lgetnum",l_getnum},
    {"lsetnum",l_setnum},
    { "lsetstr", l_setstr },
    { "lgetstr", l_getstr },
    { "lexporthtml", l_export_html },
    { "lmaxcol", l_maxcol },
    { "lmaxrow", l_maxrow },
    {"__gc", io_close},
    {NULL, NULL}
};



static void createmeta (lua_State *L) {
    luaL_newmetatable(L, LUA_SC);  /* create metatable for file handles */
    lua_pushvalue(L, -1);  /* push metatable */
    lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
    luaL_setfuncs(L, flib, 0);  /* add file methods to new metatable */
    lua_pop(L, 1);  /* pop new metatable */
}




LUAMOD_API int luaopen_sc (lua_State *L) {
    luaL_newlib(L, iolib);  /* new module */
    createmeta(L);
    add_function("SUM",&do_sum,FUNC_RANGE);
    add_function("PRODUCT",&do_prod,FUNC_RANGE);
    add_function("MIN",&do_min,FUNC_RANGE);
    add_function("MAX",&do_max,FUNC_RANGE);
    add_function("AVERAGE",&do_avg,FUNC_RANGE);
    add_function("CNT",&do_cnt,FUNC_RANGE);
    add_function("COUNT",&do_cnt,FUNC_RANGE);
    add_function("STDEV",&do_stdev,FUNC_RANGE);
    add_function("SIN",&sin,FUNC_MATH1);
    add_function("LOG",&log10,FUNC_MATH1);
    add_function("EXP",&exp,FUNC_MATH1);
    /* create (and set) default files */
    return 1;
}

