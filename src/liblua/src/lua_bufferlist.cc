/*
 * Lua module wrapping librados::bufferlist
 */
#include <errno.h>
#include <string>
#include <sstream>
#include <math.h>
#include "liblua/lua.hpp"
#include "include/types.h"
#include "include/buffer.h"
#include "objclass/objclass.h"
#include "cls/lua/cls_lua.h"

#define LUA_BUFFERLIST "ClsLua.Bufferlist"

struct bufferlist_wrap {
  bufferlist *bl;
  int gc; /* do garbage collect? */
};

static inline struct bufferlist_wrap *to_blwrap(lua_State *L, int pos = 1)
{
  return (bufferlist_wrap *)luaL_checkudata(L, pos, LUA_BUFFERLIST);
}

bufferlist *clslua_checkbufferlist(lua_State *L, int pos)
{
  struct bufferlist_wrap *blw = to_blwrap(L, pos);
  return blw->bl;
}

/*
 * Pushes a new bufferlist userdata object onto the stack. If @set is non-null
 * it is assumed to be a bufferlist that should not be garbage collected.
 */
bufferlist *clslua_pushbufferlist(lua_State *L, bufferlist *set)
{
  bufferlist_wrap *blw = (bufferlist_wrap *)lua_newuserdata(L, sizeof(*blw));
  blw->bl = set ? set : new bufferlist();
  blw->gc = set ? 0 : 1;
  luaL_getmetatable(L, LUA_BUFFERLIST);
  lua_setmetatable(L, -2);
  return blw->bl;
}

/*
 * Create a new bufferlist
 */
static int bl_new(lua_State *L)
{
  clslua_pushbufferlist(L, NULL);
  return 1;
}

/*
 * Convert bufferlist to Lua string
 */
static int bl_str(lua_State *L)
{
  bufferlist *bl = clslua_checkbufferlist(L);
  lua_pushlstring(L, bl->c_str(), bl->length());
  return 1;
}

/*
 * Append a Lua string to bufferlist
 */
static int bl_append(lua_State *L)
{
  bufferlist *bl = clslua_checkbufferlist(L);
  luaL_checktype(L, 2, LUA_TSTRING);

  size_t len;
  const char *data = lua_tolstring(L, 2, &len);
  bl->append(data, len);

  return 0;
}

/*
 * Perform byte-for-byte bufferlist equality test
 */
static int bl_eq(lua_State *L)
{
  bufferlist *bl1 = clslua_checkbufferlist(L, 1);
  bufferlist *bl2 = clslua_checkbufferlist(L, 2);
  lua_pushboolean(L, *bl1 == *bl2 ? 1 : 0);
  return 1;
}

/*
 * Garbage collect bufferlist
 */
static int bl_gc(lua_State *L)
{
  struct bufferlist_wrap *blw = to_blwrap(L);
  assert(blw);
  assert(blw->bl);
  if (blw->gc)
    delete blw->bl;
  return 0;
}

static const struct luaL_Reg bufferlist_methods[] = {
  {"str", bl_str},
  {"append", bl_append},
  {"__gc", bl_gc},
  {"__eq", bl_eq},
  {NULL, NULL}
};

static const struct luaL_Reg bllib_f[] = {
  {"new", bl_new},
  {NULL, NULL}
};

LUALIB_API int luaopen_bufferlist(lua_State *L)
{
  /* Setup bufferlist user-data type */
  luaL_newmetatable(L, LUA_BUFFERLIST);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, bufferlist_methods);
  lua_pop(L, 1);

  luaL_register(L, "bufferlist", bllib_f);

  return 1;
}
