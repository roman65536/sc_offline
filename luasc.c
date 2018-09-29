#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lua.h"
#include "lauxlib.h"


typedef struct
{
  short closed;
} env_data;



typedef struct
{
  short closed;
  int env;                /* reference to environment */
  short auto_commit;        /* 0 for manual commit */
  char * test;
} sc_data;


static env_data *getenvironment(lua_State *L) {
  env_data *env = (env_data *)luaL_checkudata(L, 1, "SC_ENV");
  luaL_argcheck(L, env != NULL, 1, "environment expected");
  luaL_argcheck(L, !env->closed, 1,"environment is closed");
  return env;
}




static int cur_gc(lua_State *L)
{
  sc_data *cur = (sc_data *)luaL_checkudata(L, 1, "SC_DATA");
  if (cur != NULL && !(cur->closed))
    {
//      sqlite3_finalize(cur->sql_vm);
//      cur_nullify(L, cur);
    }
  return 0;
}


/*
** Close the cursor on top of the stack.
** Return 1
*/
static int cur_close(lua_State *L)
{
  sc_data *cur = (sc_data *)luaL_checkudata(L, 1, "SC_DATA");
  luaL_argcheck(L, cur != NULL, 1, "SC_data expected");
  if (cur->closed) {
    lua_pushboolean(L, 0);
    return 1;
  }
//  sqlite3_finalize(cur->sql_vm);
//  cur_nullify(L, cur);
  lua_pushboolean(L, 1);
  return 1;
}

static int sc_exec(lua_State *L)
{
 sc_data *conn = (sc_data *)luaL_checkudata (L, 1, "SC_DATA");
 const char *statement = luaL_checkstring(L, 2);
  luaL_argcheck(L, conn != NULL, 1, "SC_DATA connection expected");
  luaL_argcheck(L, !conn->closed, 1, "SC_DATA connection is closed");
  printf("EXEC:%s",conn->test);
 return 1;
 
 }

static int create_connection(lua_State *L )
{
 char * name;

 getenvironment(L);  /* validate environment */

  name = luaL_checkstring(L, 2);
  
  sc_data *sc = (sc_data*)lua_newuserdata(L, sizeof(sc_data));
  luasql_setmeta(L, "SC_DATA");

  /* fill in structure */
  sc->closed = 0;
  sc->env = LUA_NOREF;
  sc->auto_commit = 1;
  sc->test=strdup(name);
  
  lua_pushvalue (L, 1);
  sc->env = luaL_ref (L, LUA_REGISTRYINDEX);
  return 1;
}

/*
** Environment object collector function.
*/
static int env_gc (lua_State *L)
{
  env_data *env = (env_data *)luaL_checkudata(L, 1, "SC_ENV");
  if (env != NULL && !(env->closed))
    env->closed = 1;
  return 0;
}


/*
** Close environment object.
*/
static int env_close (lua_State *L)
{
  env_data *env = (env_data *)luaL_checkudata(L, 1, "SC_ENV");
  luaL_argcheck(L, env != NULL, 1, "SC_ENV environment expected");
  if (env->closed) {
    lua_pushboolean(L, 0);
    return 1;
  }
  env_gc(L);
  lua_pushboolean(L, 1);
  return 1;
}



static void create_metatables (lua_State *L)
{
  struct luaL_Reg environment_methods[] = {
    {"__gc", env_gc},
    {"close", env_close},
    {"connect", create_connection},
    {NULL, NULL},
  };
 
 
  struct luaL_Reg connection_methods[] = {
    {"__gc", cur_gc},
    {"close", cur_close},
    
    {"execute", sc_exec},
        {NULL, NULL},
  };
 
  luasql_createmeta(L, "SC_ENV", environment_methods);
  luasql_createmeta(L,  "SC_DATA", connection_methods);
  
  lua_pop (L, 3);
}

/*
** Creates an Environment and returns it.
*/
static int create_environment (lua_State *L)
{
  sc_data *env = (sc_data *)lua_newuserdata(L, sizeof(sc_data));
  luasql_setmeta(L, "SC_ENV");

  /* fill in structure */
  env->closed = 0;
  return 1;
}


/*
** Creates the metatables for the objects and registers the
** driver open method.
*/
 int luaopen_sc(lua_State *L)
{
  struct luaL_Reg driver[] = {
    {"sc", create_environment},
    {NULL, NULL},
  };
  create_metatables (L);
  lua_newtable (L);
  luaL_setfuncs (L, driver, 0);
  luasql_set_info (L);
  return 1;
}




typedef struct { short  closed; } pseudo_data;

static int luasql_tostring (lua_State *L) {
	char buff[100];
	pseudo_data *obj = (pseudo_data *)lua_touserdata (L, 1);
	if (obj->closed)
		strcpy (buff, "closed");
	else
		sprintf (buff, "%p", (void *)obj);
	lua_pushfstring (L, "%s (%s)", lua_tostring(L,lua_upvalueindex(1)), buff);
	return 1;
}


#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
/*
** Adapted from Lua 5.2.0
*/
void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
	luaL_checkstack(L, nup+1, "too many upvalues");
	for (; l->name != NULL; l++) {	/* fill the table with given functions */
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)	/* copy upvalues to the top */
			lua_pushvalue(L, -(nup + 1));
		lua_pushcclosure(L, l->func, nup);	/* closure with those upvalues */
		lua_settable(L, -(nup + 3));
	}
	lua_pop(L, nup);	/* remove upvalues */
}
#endif



/*
** Create a metatable and leave it on top of the stack.
*/
int luasql_createmeta (lua_State *L, const char *name, const luaL_Reg *methods) {
	if (!luaL_newmetatable (L, name))
		return 0;

	/* define methods */
	luaL_setfuncs (L, methods, 0);

	/* define metamethods */
	lua_pushliteral (L, "__index");
	lua_pushvalue (L, -2);
	lua_settable (L, -3);

	lua_pushliteral (L, "__tostring");
	lua_pushstring (L, name);
	lua_pushcclosure (L, luasql_tostring, 1);
	lua_settable (L, -3);

	lua_pushliteral (L, "__metatable");
	lua_pushliteral (L, "SC you're not allowed to get this metatable");
	lua_settable (L, -3);

	return 1;
}


/*
** Define the metatable for the object on top of the stack
*/
void luasql_setmeta (lua_State *L, const char *name) {
	luaL_getmetatable (L, name);
	lua_setmetatable (L, -2);
}


/*
** Assumes the table is on top of the stack.
*/
 void luasql_set_info (lua_State *L) {
	lua_pushliteral (L, "_COPYRIGHT");
	lua_pushliteral (L, "Copyright (C) 2003-2012 Kepler Project");
	lua_settable (L, -3);
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "SC is a simple interface from Lua to a DBMS");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "SC 0.3.0");
	lua_settable (L, -3);
}

