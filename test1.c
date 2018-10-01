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

// reference a sheet in a formula
int main () {
    // this could serve to load default functions (AVG, COUNT) from a lua script
    lua_State *L;
    L = luaL_newstate();
    int a;

#ifdef OLD
    printf("OLD\n");
#else
    printf("NEW\n");
#endif

    struct roman * p = (struct roman *) malloc(sizeof(struct roman));
    p->name=NULL;
    p->open=0; // we are not loading a file here..
    p->first_sh=p->last_sh=p->cur_sh=0;
    p->cache=0;
    p->cache_nr=0;

    // create a sheet
    struct Sheet * sh = new_sheet(p, "sales 1");

    p->cur_sh = sh;
    // set a value of a cell
    struct Ent * t = lookat(p->cur_sh, 0, 0);
    t->flag |= VAL;
    t->val = 4;

    // create a sheet
    struct Sheet * sh1 = new_sheet(p, "sales 2");

    p->cur_sh = sh1;
    // set a value of a cell
    struct Ent * tl = lookat(p->cur_sh, 0, 0);
    tl->flag |= VAL;
    tl->val = 5;

    // add a formula to other cell
    t = lookat(p->cur_sh, 1, 0);
    t->formula= "'sales 1'!A0 + 'sales 2'!A0 + 2 + 1";
    char * ptr = t->formula;
    t->exp = getAST(ptr, p);
    t->flag |= VAL | RP_FORMULA;
    t->val = t->exp->value;

for(a=0;a<1000;a++)
{
    // calc
    recalc(p);

    tl->val=(double)a;
    // show results
    //struct Ent * e1 = lookat(sh1, 1, 0);
    struct Ent * e1 = lookat(Search_sheet(p, "sales 2"), 1, 0);
    printf("result :: %f !!\n", e1->val);
 }

    p->open=0;
    return 0;
}

// simple formula
/*
int main () {
    // this will serve to load with lua default operations
    lua_State *L;
    L = luaL_newstate();
	int a;

    struct roman * p = (struct roman *) malloc(sizeof(struct roman));
    p->name=NULL;
    p->open=0; // we are not loading a file here..
    p->first_sh=p->last_sh=p->cur_sh=0;
    p->cache=0;
    p->cache_nr=0;

    struct Sheet * sh = new_sheet(p, "Sheet 1");
    growtbl(sh, GROWNEW, 0, 0);
    p->cur_sh = sh;

    struct Ent * t = lookat(p->cur_sh, 0, 0);
    t->flag |= VAL;
    t->val = 1;

    t = lookat(p->cur_sh, 1, 0);
    t->flag |= VAL;
    t->val = 2;

    // add a formula
    t = lookat(p->cur_sh, 2, 0);
    t->formula= "A0+A1";
    char * ptr = t->formula;
    t->exp=getAST(ptr, p);
    t->flag |= VAL | RP_FORMULA;
    t->val=t->exp->value;

    recalc(p);

    struct Ent * e1 = lookat(p->cur_sh, 0, 0);
    printf("value A0: %f !!\n", e1->val);

    struct Ent * e2 = lookat(p->cur_sh, 1, 0);
    printf("value A1: %f !!\n", e2->val);

    struct Ent * e3 = lookat(p->cur_sh, 2, 0);
    printf("value A2: %f !!\n", e3->val);

    p->open=0;
    return 0;
}
*/

/* test main
// loading test.xlsx
int main () {
    lua_State *L;
    L = luaL_newstate();

    const char *filename = "test.xlsx";

    struct roman *p = (struct roman *)lua_newuserdata(L, sizeof(struct roman));
    luaL_setmetatable(L, LUA_SC);
    p->name=strdup(filename);
    p->open=1;
    p->first_sh=p->last_sh=p->cur_sh=0;
    p->cache=0;
    p->cache_nr=0;

    printf("%s %s %d\n", __FUNCTION__, p->name, p->open);

    struct Sheet *sh;
    char * name = "Tabelle1";
    //p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    printf("%s %s %s\n", __FUNCTION__, p->name, name);
    open_xlsx(p, filename, "");

    int i=1;

    //p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    printf("%s %s\n",__FUNCTION__,p->name);

    if(p->first_sh == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "No Sheets");
        return 2;
    }

    lua_newtable(L);
    for(sh=p->first_sh;sh !=0; sh=sh->next) {
        lua_pushnumber(L, i++);
        lua_pushstring(L, sh->name);
        lua_settable(L, -3);
    }

    //name = luaL_checkstring(L, 2);
    //p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
    printf("3:%s %s %s\n",__FUNCTION__,p->name,name);
    sh=Search_sheet(p, name);
    p->cur_sh=sh;

    recalc(p);

    int r = 4, c = 0;
    struct Ent ** pp = ATBL(p->cur_sh,p->cur_sh->tbl, r, c);

    struct Ent * e = *pp;

    if (e == 0) printf("null?! \n");

    printf("getnum2\n", e->val);
    if (e->flag & VAL) {
        printf("value: %f !!\n", e->val);
    }

    p->open=0;
    return 0;
}
*/
