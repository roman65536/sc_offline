#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>   // for isdigit
#include <stdlib.h>  // for atoi

#include "rpsc.h"

#include <xls.h>


#define MAX(a,b) (((a)>(b))?(a):(b))


int open_xls(struct roman *p,char * fname, char * encoding) {


 xlsWorkBook * pWB;
 xlsWorkSheet * pWS;
 int sh;
 int r,c;
 struct Ent *ent;


pWB = xls_open(fname, encoding);

 for(sh=0;sh<pWB->sheets.count;sh++)
 {
   struct Sheet *this_sh;

  printf("Reading sheet: [%s] \n",pWB->sheets.sheet[sh].name );
        this_sh=new_sheet(p,pWB->sheets.sheet[sh].name);
        growtbl(this_sh,GROWNEW, 0, 0);
        p->cur_sh=this_sh;


    pWS=xls_getWorkSheet(pWB,sh);
        xls_parseWorkSheet(pWS);

       for (r = 0; r <= pWS->rows.lastrow; r++) { // rows
        for (c = 0; c <= pWS->rows.lastcol; c++) { // cols
           xlsCell * cell = xls_cell(pWS, r, c); 
	   if ((! cell) || (cell->isHidden)) continue;
  	   struct st_xf_data * xf = &pWB->xfs.xf[cell->xf];

	   
  	if (((xf->format >= 14 && xf->format <= 22) ||
                (xf->format >= 165 && xf->format <= 180) ||
                xf->format == 278 || xf->format == 185 || xf->format == 196 || xf->format == 217 || xf->format == 326 )
               && cell->id != 0x06 
		//&& cell->id != 0x27e 
	       && cell->id != 0x0BD && cell->id != 0x203 ) {

                     ent=lookat(p->cur_sh,r+1,c);
                     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
                     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r+1);
		     ent->val=(cell->d-25569)*86400-1;
                     ent->flag|=VAL;
		}
		 else if (cell->id == 0x27e || cell->id == 0x0BD || cell->id == 0x203) {
 		     ent=lookat(p->cur_sh,r+1,c);
                     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
                     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r+1);
                     ent->val=cell->d;
                     ent->flag|=VAL;
		   
		}
		else if (cell->str != NULL) {

		     ent=lookat(p->cur_sh,r+1,c);
                     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
                     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r+1);
                     ent->label=strdup(cell->str);
                     ent->flag|=RP_LABEL;
		}

	  
  

   }
  }
 }
}

