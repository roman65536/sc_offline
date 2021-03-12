/*
csvtest - reads CSV data from stdin and output properly formed equivalent
          useful for testing the library
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <csv.h>


#include "rpsc.h"
#include "xlsx.h"
#include "sheet.h"


#define MAX(a,b) (((a)>(b))?(a):(b))



static struct plugin *pl;

#define new_sheet_pl(...) (*pl->new_sheet)( __VA_ARGS__)
#define lookat_pl(...) (*pl->lookat)( __VA_ARGS__)
#define search_sheet_pl(...) (*pl->search_sheet)( __VA_ARGS__)
#define getAST_pl(...) (*pl->getAST)( __VA_ARGS__)
#define deleteExpression_pl(...) (*pl->deleteExpression)( __VA_ARGS__)


static int put_comma;
static int line, col;
static struct roman *rp;

void cb1 (void *s, size_t i, void *p) {
  char *tmp=s;
   struct Ent *ent;
	 char *next;
	 int len=0;
	 
	 // if (put_comma)
	//    putc(',', stdout);
//  csv_fwrite(stdout, s, i);
    tmp[i]=0;
	 ent=lookat_pl(rp->cur_sh,line,col);
	 rp->cur_sh->maxcol=MAX(rp->cur_sh->maxcol,col);
         rp->cur_sh->maxrow=MAX(rp->cur_sh->maxrow,line);
		 //ent->val=strtod(tmp,&next);
		 //if ((tmp == next) )
		 //{
		/* not a number */
		 ent->label=strndup(tmp,i);
		ent->flag |= RP_LABEL;
		//}
		//else ent->flag |= VAL;
	
		//	    printf("[%d:%d] : %s %d",line,col,tmp,i);
	 col++;
  put_comma = 1;
}

void cb2 (int c, void *p) {
  put_comma = 0;
  //   putc('\n', stdout);
  line++;
  col=0;
}



static char *csv_ending[]= {
{".csv"},
{0}
};


static csv_read(struct roman *p1, char *name);

static char csv_name[] ="csv";

 int init_csv(struct plugin *ptr)
{
ptr->ending=csv_ending;
ptr->type=PLUG_IN;
ptr->read=csv_read;
ptr->name=csv_name;

pl=ptr;

}


static csv_read(struct roman *p1, char *name)
{

  FILE *fp;
  struct csv_parser p;
  char buf[1024];
  int bytes_read;
  int retval;
  int i;
  char c;

  rp=p1;
  csv_init(&p, 0);
  line=col=0;
  fp=fopen(name,"rb");
  if(!fp)
	{
	  fprintf(stderr,"Failed to open %s\n",name);
	}

  p1->cur_sh=new_sheet_pl(p1,"CSV DEFAULT");
  
  while ((bytes_read=fread(buf,1,1024,fp))>0) {
    
    if (retval=csv_parse(&p, buf, bytes_read, cb1, cb2, NULL) != bytes_read) {
      fprintf(stderr, "Error: %s\n", csv_strerror(csv_error(&p)));
      exit(EXIT_FAILURE);
    }
  }
  fclose(fp);
  csv_fini(&p, cb1, cb2, NULL);
  csv_free(&p);

  return EXIT_SUCCESS;
}

