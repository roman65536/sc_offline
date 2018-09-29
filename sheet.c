#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rpsc.h"

// prototypes
int growtbl(struct Sheet * sh, int rowcol, int toprow, int topcol);
void checkbounds(struct Sheet *sh, int *rowp, int *colp);

struct Ent * lookat(struct Sheet * sh, int row, int col) {
    register struct Ent ***tbl=sh->tbl;
    register struct Ent **pp;

    checkbounds(sh, &row, &col);
    pp = ATBL(tbl, row, col);
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

struct Sheet * Search_sheet(struct roman *doc, char *name) {
    struct Sheet *sh;

    for(sh=doc->first_sh; sh != 0; sh=sh->next) {
        printf("sheet: %s %s\n", name, sh->name);
        if(! strcmp(name,sh->name) ) return sh;
    }

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

