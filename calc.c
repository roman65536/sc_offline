#include <math.h>
#include "rpsc.h"
#include "Parser.h"
#include "Lexer.h"

double eval(struct roman *pp, SExpression *s, struct Ent *p) {
    double (*func)(struct roman *,int, char **);
    double (*funct)(double);
    switch (s->type) {
        case eVALUE:   return s->value;
        case ePLUS:    return eval(pp,s->left,p)+eval(pp,s->right,p); break;
        case eMINUS:   return eval(pp,s->left,p)-eval(pp,s->right,p); break;
        case eMULTIPLY: return eval(pp,s->left,p)*eval(pp,s->right,p); break;
        case eDIVIDE:  return eval(pp,s->left,p)/eval(pp,s->right,p); break;
        case '>':      return (eval(pp,s->left,p)>eval(pp,s->right,p)); break;
        case '<':      return (eval(pp,s->left,p)<eval(pp,s->right,p)); break;
        case '=':      return eval(pp,s->left,p)==eval(pp,s->right,p); break;
        case '^':      return pow(eval(pp,s->left,p),eval(pp,s->right,p)); break;
        case eIF:      return eval(pp,s->left,p)?eval(pp,s->if_t->next,p):eval(pp,s->if_t->next->next,p); break;

        case 's':
                       {
                           char * str = (char *) s->right;
                           struct Ent * rp = s->left->ptr;
                           if (rp->label == 0) return 0;
                           return strcmp(rp->label, str) ? 0 : 1;
                       }
                       break;

        case 'f':

                       func = s->ptr;
                       return func(pp, s->next->ptr, s->next->next->ptr);
                       break;

        case'm':
                       funct = s->ptr;
                       return funct(eval(pp, s->next, p));
                       break;
        case eENT:
                       {
                           struct Ent * ppd = (struct Ent *) s->ptr;

                           if (ppd == p ) {
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


void recalc(struct roman * p) {
    register struct Ent ** pp;
    int x, y;
    int a=0;

    if (p->cache_nr !=0) {
        for(a=0; a < p->cache_nr; a++) {
            p->cache[a]->val = eval(p, (p->cache[a])->exp, p->cache[a]);
        }
    }
    else{
        for(p->cur_sh = p->first_sh; p->cur_sh != 0; p->cur_sh = p->cur_sh->next) {
            for(x=0; x <= p->cur_sh->col; x++)
                for(y=0; y <= p->cur_sh->row; y++) {
                    pp = ATBL(p->cur_sh->tbl, y, x);
                    if(*pp != 0) {
                        //printf("%s1 %s %d %d  %x val : %f\n",__FUNCTION__,p->cur_sh->name,x,y,(*pp)->flag, (*pp)->val);
                        if (((*pp)->flag & RP_FORMULA) == RP_FORMULA) {
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
