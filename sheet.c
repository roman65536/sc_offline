#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "rpsc.h"
#include "sheet.h"


struct Ent * lookat(struct Sheet * sh, int row, int col) {
    register struct Ent ***tbl=sh->tbl;
    register struct Ent **pp;
    struct Ent *tmp;
#if OLD
    checkbounds(sh, &row, &col);
    pp = ATBL(sh , tbl, row, col);
     if (*pp == NULL) {
        *pp = (struct Ent *) calloc(1, (unsigned) sizeof(struct Ent));
        if (row > sh->row) sh->row = row;
        if (col > sh->col) sh->col = col;
        (*pp)->label = (char *)0;
        (*pp)->formula = (char *)0;
        (*pp)->flag = 0;
        (*pp)->row = row;
        (*pp)->col = col;
        (*pp)->val = (double) 0.0;
    }
    return (*pp);

#else
    tmp=sh->hash[HASH(row,col)];
    for(;tmp!=0;tmp=(tmp)->n_hash)
	if(( (tmp)->row== row) && ((tmp)->col == col))
	    return (tmp);
    //tmp = (struct Ent * ) calloc(1, (unsigned) sizeof(struct Ent));
    tmp = (struct Ent * ) objs_cache_alloc(&sh->cache_ent);
     if (row > sh->row) sh->row = row;
     if (col > sh->col) sh->col = col;
    (tmp)->label = (char *)0;
    (tmp)->formula = (char *)0;
    (tmp)->flag=0;
    (tmp)->row=row;
    (tmp)->col=col;
    (tmp)->val=(double)0.0;
    (tmp)->n_hash=sh->hash[HASH(row,col)];
    sh->hash[HASH(row,col)]=(tmp);
    return (tmp);
#endif
   
}

#if NEW
struct Ent ** atbl(struct Sheet *sh, struct Ent ***tbl, int row, int col) {
    static struct Ent * tmp;
    long a=HASH(row,col);
    tmp=sh->hash[a];
    for(;tmp!=0;tmp=(tmp)->n_hash)
	if(( (tmp)->row== row) && ((tmp)->col == col))
	    return (&tmp);

    return ((struct Ent *) 0);
}
#endif

struct Sheet * new_sheet(struct roman * doc, char * name) {
    struct Sheet * sh;
    sh = (struct Sheet *) calloc(1, sizeof(struct Sheet));
    INSERT(sh, (doc->first_sh), (doc->last_sh), next, prev);
    sh->name = strdup(name);
    sh->tbl = 0;
    sh->hash= (void *) calloc(HASH_NR,sizeof(void *));
    sh->nr_hash=HASH_NR;
    sh->maxcol = sh->maxrow = 0;
    sh->ccol = 16;
    sh->crow = 32768;
    objs_cache_init(&sh->cache_ent, sizeof(struct Ent), NULL);

    return sh;
}

struct Sheet * Search_sheet(struct roman *doc, char *name) {
    struct Sheet *sh;

    for(sh=doc->first_sh; sh != 0; sh=sh->next) {
  //      printf("sheet: %s %s\n", name, sh->name);
        if(! strcmp(name,sh->name) ) return sh;
    }

    return 0;
}


void delete_sheet(struct roman *doc, struct Sheet *sh)
{
    int a;
    struct Ent *ent;
    REMOVE(sh,(doc->first_sh),(doc->last_sh),next,prev);
    for(a=0;a<HASH_NR;a++)
	{
	    if(sh->hash[a] !=0 )
		for(ent=sh->hash[a]; ent !=0; ent=ent->n_hash){
		    if(ent->label !=0 ) free(ent->label);
		    if(ent->formula !=0 ) free(ent->formula);
		    /* free_exp add here */
		    if(ent->exp !=0 ) deleteExpression(ent->exp);
		    objs_cache_free(&sh->cache_ent,ent);
		}
	}
	free(sh->hash);
    free(sh->name);
    objs_cache_destroy(&sh->cache_ent);
    free(sh);

}


int convert(int * col, int * row, char * s, int size) {
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



////// TO REVIEW

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
        struct ent *** lnullit;
        int lcnt;

        //GROWALLOC(row_hidden2, row_hidden, newrows, char, nolonger);
        //memset(row_hidden+sh->row, 0, (newrows-sh->row)*sizeof(char));

        /*
         * alloc tbl row pointers, per net.lang.c, calloc does not
         * necessarily fill in NULL pointers
         */
        GROWALLOC(sh->tbl, sh->tbl, newrows, struct Ent ** , nolonger);
        for (lnullit = sh->tbl+sh->crow, lcnt = 0; lcnt < newrows-sh->crow; lcnt++, lnullit++)
            *lnullit = (struct ent **) NULL;
        /* memset(tbl+maxrows, (char *)NULL, (newrows-maxrows)*(sizeof(struct ent **)));*/
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
    for (nullit = ATBL(sh,sh->tbl, i, sh->ccol), cnt = 0;
            cnt < newcols-sh->ccol; cnt++, nullit++)
        *nullit = (struct Ent *)NULL;
    memset((char *)ATBL(sh,sh->tbl,i,sh->ccol), 0,
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

