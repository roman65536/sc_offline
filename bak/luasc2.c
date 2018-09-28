#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>

int foo_gc();
int foo_index();
int foo_newindex();
int foo_dosomething();
int foo_new();
int foo_read();

struct foo {
    int x;
};

typedef struct foo Foo;

static const luaL_Reg _meta[] = {
    {"__gc", foo_gc},
    {"__index", foo_index},
    {"__newindex", foo_newindex},
    { NULL, NULL }
};
static const luaL_Reg _methods[] = {
    {"read", foo_read},
    {"new", foo_new},
    {"dosomething", foo_dosomething},
    { NULL, NULL }
};


int foo_read(lua_State* L) {
 Foo *f=luaL_checkudata (L, 1, "Foo");
    printf("## read %x\n",f);
    return 0;
}


int foo_gc(lua_State* L) {
    printf("## __gc\n");
    return 0;
}
int foo_newindex(lua_State* L) {
    printf("## __newindex\n");
    return 0;
}
int foo_index(lua_State* L) {
    printf("## __index\n");
    return 0;
}
int foo_dosomething(lua_State* L) {
    printf("## dosomething\n");
    return 0;
}
int foo_new(lua_State* L) {
    printf("## new\n");

    lua_newuserdata(L,sizeof(Foo));
    luaL_getmetatable(L, "Foo");
    lua_setmetatable(L, -2); 

    return 1;
}

#if 0
void register_foo_class(lua_State* L) {
    luaL_newlib(L, _methods); 
    luaL_newmetatable(L, "Foo");
    luaL_setfuncs(L, _meta, 0);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "Foo");
}
#endif

void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
        luaL_checkstack(L, nup+1, "too many upvalues");
        for (; l->name != NULL; l++) {  /* fill the table with given functions */
                int i;
                lua_pushstring(L, l->name);
                for (i = 0; i < nup; i++)       /* copy upvalues to the top */
                        lua_pushvalue(L, -(nup + 1));
                lua_pushcclosure(L, l->func, nup);      /* closure with those upvalues */
                lua_settable(L, -(nup + 3));
        }
        lua_pop(L, nup);        /* remove upvalues */
}

 int luaopen_sc(lua_State *L)
{
 register_foo_class(L);
}


void register_foo_class(lua_State* L) {
    int lib_id, meta_id;

    /* newclass = {} */
    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    /* metatable = {} */
    luaL_newmetatable(L, "Foo");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, _meta, 0);

    /* metatable.__index = _methods */
    luaL_newlib(L, _methods);
    lua_setfield(L, meta_id, "__index");    

    /* metatable.__metatable = _meta */
    luaL_newlib(L, _meta);
    lua_setfield(L, meta_id, "__metatable");

    /* class.__metatable = metatable */
    lua_setmetatable(L, lib_id);

    /* _G["Foo"] = newclass */
    lua_setglobal(L, "Foo");
}
