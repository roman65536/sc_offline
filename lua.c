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

#include "function.h"

#include "slab.h"
#include "queue.h"


extern struct Objs_cache cache_Userland_slab;


io_open(lua_State *L)
{

 const char *filename = luaL_checkstring(L, 1);
 struct roman *p = (struct roman *)lua_newuserdata(L, sizeof(struct roman));
 luaL_setmetatable(L, LUA_SC);
 p->name=strdup(filename);
 p->open=1;
 p->first_sh=p->last_sh=p->cur_sh=0;
 p->cache=0;
 p->cache_nr=0;
 
 printf("%s %s %d\n",__FUNCTION__,p->name,p->open);
printf( "%x \n ",&cache_Userland_slab);
 return 1;
}

io_close(lua_State *L)
{
 struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
 printf("%s %s %d\n",__FUNCTION__,p->name,p->open);
 p->open=0;
 return 1;


}


static int f_gc (lua_State *L) {
  struct roman *p = ((struct roman *)luaL_checkudata(L, 1, LUA_SC)); 
 printf("%s %s %d\n",__FUNCTION__,p->name,p->open);

  return 0;
}

io_sheet(lua_State *L)
{
 struct Sheet *sh;
 const char *name = luaL_checkstring(L, 2);
 struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
// printf("%s %s %s\n",__FUNCTION__,p->name,name);
 sh=Search_sheet(p,name);
 p->cur_sh=sh;
 return 0;
}


io_newsheet(lua_State *L)
{
 struct Sheet *sh;
 const char *name = luaL_checkstring(L, 2);
 struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
 printf("%s %s %s\n",__FUNCTION__,p->name,name);
 sh=new_sheet(p,name);
 p->cur_sh=sh;
 return 0;
}

io_loadxl(lua_State *L)
{
 struct Sheet *sh;
 const char *name = luaL_checkstring(L, 2);
 struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
 printf("%s %s %s\n",__FUNCTION__,p->name,name);
 //open_xlsx(p,name,"");
 read_plugin(p,name,"xlsx");

 
 return 0;
}


io_loadhtml(lua_State *L)
{
 struct Sheet *sh;
 const char *name = luaL_checkstring(L, 2);
 struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
 printf("%s %s %s\n",__FUNCTION__,p->name,name);
 //open_xlsx(p,name,"");
 read_plugin(p,name,"html");


 return 0;
}


io_loadxls(lua_State *L)
{
 struct Sheet *sh;
 const char *name = luaL_checkstring(L, 2);
 struct roman *p= ((struct roman *)luaL_checkudata(L, 1, LUA_SC));
 printf("%s %s %s\n",__FUNCTION__,p->name,name);
// open_xls(p,name,"");

 return 0;
}

io_getsheets(lua_State *L)
{
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
   pp = ATBL(d->cur_sh,d->cur_sh->tbl,r,c);
if(pp ==0 ) return 0;
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

    export(d,stdout,"<table>","</table>\n","<tr>","</tr>\n","<td>","</td>");

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
  {"loadxls", io_loadxls},
  {"loadhtml", io_loadhtml},
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

fprintf(stderr,"%x \n ",&cache_Userland_slab);
slab_allocator_init();
fprintf(stderr, "%x \n ",&cache_Userland_slab);
ExpressionInit();
fprintf(stderr, "%x \n ",&cache_Userland_slab);
init_lib();
init_plugin();


load_plugin("html");
load_plugin("xlsx");


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

