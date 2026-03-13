#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// mocked version
//
//

#if LUA_VERSION_NUM >= 502
#undef luaL_register
#define luaL_register(L, n, l) luaL_setfuncs(L, (l), 0)
#define luaL_register_namespace(L, n, l)                                       \
  lua_getglobal(L, n);                                                         \
  if (lua_isnil(L, -1)) {                                                      \
    lua_pop(L, 1);                                                             \
    lua_newtable(L);                                                           \
  }                                                                            \
  luaL_setfuncs(L, (l), 0);                                                    \
  lua_pushvalue(L, -1);                                                        \
  lua_setglobal(L, n);
#else
#define luaL_register_namespace(L, n, l) luaL_register(L, n, (l));
#endif

void print_message(const char *type, const char *msg);
void luaopen_msg_mock(lua_State *L);
void probe(const char *stream);
int read_stream(char *c, const char *uri);

// instead of logging system lets just redirect to our stdout for now
void print_message(const char *type, const char *msg) {
  printf("[%s] : %s\n", type, msg);
}

// copied from original implementation
/*****************************************************************************
 * Messaging facilities
 *****************************************************************************/
static int vlclua_msg_dbg(lua_State *L) {
  int i_top = lua_gettop(L);
  int i;
  for (i = 1; i <= i_top; i++)
    print_message("debug", luaL_checkstring(L, i));
  return 0;
}

static int vlclua_msg_warn(lua_State *L) {
  int i_top = lua_gettop(L);
  int i;
  for (i = 1; i <= i_top; i++)
    print_message("warn", luaL_checkstring(L, i));
  return 0;
}

static int vlclua_msg_err(lua_State *L) {
  int i_top = lua_gettop(L);
  int i;
  for (i = 1; i <= i_top; i++)
    print_message("err", luaL_checkstring(L, i));
  return 0;
}

static int vlclua_msg_info(lua_State *L) {
  int i_top = lua_gettop(L);
  int i;
  for (i = 1; i <= i_top; i++)
    print_message("info", luaL_checkstring(L, i));
  return 0;
}

static const luaL_Reg vlclua_msg_reg_mock[] = {{"dbg", vlclua_msg_dbg},
                                               {"warn", vlclua_msg_warn},
                                               {"err", vlclua_msg_err},
                                               {"info", vlclua_msg_info},
                                               {NULL, NULL}};

static int vlclua_demux_peek(lua_State *L) {
  // gets the first argument value
  size_t value = luaL_checkinteger(L, 1);
  char *faked_stream = malloc(1000);
  if (!faked_stream) {
    print_message("err", "Couldn't create the stream");
    lua_pushnil(L);
    return 0;
  }
  int ret = read_stream(faked_stream, TOP_SRCDIR "/test/modules/lua/test.txt");

  if (ret) {
    print_message("err", "Couldn't read the stream");
    free(faked_stream);
    return 0;
  }

  if (strlen(faked_stream) < value) {
    print_message("err", "Unexpected peek value");
    free(faked_stream);
    return 0;
  }

  lua_pushlstring(L, &faked_stream[0], value);
  free(faked_stream);
  return 1;
}

// luaopen_msg_mock is a replacement for how luaopen_msg would register lua
// callbacks interally
void luaopen_msg_mock(lua_State *L) {
  lua_newtable(L);
  luaL_register(L, NULL, vlclua_msg_reg_mock);
  lua_setfield(L, -2, "msg");
}

void probe(const char *stream) {
  /* Initialise Lua state structure */
  lua_State *L = luaL_newstate();
  if (!L) {
    print_message("err", "Lua state cannot be created");
    return;
  }

  /* Load Lua libraries */
  luaL_openlibs(L); /* FIXME: Don't open all the libs? */

  /* Functions to register */
  static const luaL_Reg p_reg[] = {{"peek", vlclua_demux_peek}, {NULL, NULL}};

  lua_newtable(L);
  luaL_register(L, NULL, p_reg);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "vlc");

  // for now lets only consider msg there are others as well like variables,
  // stream which is not mocked yet
  luaopen_msg_mock(L);
  // hardcoded the path
  lua_pushstring(L, "length=100000000000000000");
  lua_setfield(L, -2, "path");

  // hardcoded the access as well
  lua_pushstring(L, "mock");
  lua_setfield(L, -2, "access");

  lua_pop(L, 1);

  /* Load and run the script(s) */
  // originally it uses vlclua but we are reaplcing it simplly with lua_dofile

  if (luaL_dostring(L, stream)) {
    print_message("err", lua_tostring(L, -1));
    lua_close(L);
    return;
  }

  lua_getglobal(L, "probe");

  if (!lua_isfunction(L, -1)) {
    print_message("err", "Error running the script no probe function found");
    lua_close(L);
    return;
  }

  if (lua_pcall(L, 0, 1, 0)) {
    print_message("err", lua_tostring(L, -1));
    lua_close(L);
    return;
  }

  if (lua_gettop(L)) {
    if (lua_toboolean(L, 1)) {

      lua_pop(L, 1);
    }
  }

  lua_close(L);
}

int read_stream(char *c, const char *uri) {
  FILE *fp;
  int thisChar;
  int i = 0;

  fp = fopen(uri, "r");
  if (fp == NULL) {
    print_message("err", "Error opening the file");
    return 1;
  }

  while ((thisChar = fgetc(fp)) != EOF) {
    c[i++] = (char)thisChar;
  }

  c[i] = '\0';

  fclose(fp);
  return 0;
}

int main() {

  // the very first thing we need is obv stream
  // instead of a real stream that vlc uses which includes read from the file or
  // some http resource but for now just for example lets just consider char
  // array as a mocked stream
  // mock://length=100000000000000000
  // its the content inisde the uri
  char *mocked_stream = malloc(1000);
  if (!mocked_stream)
    return 1;

  int ret = read_stream(mocked_stream,
                        TOP_SRCDIR "/test/modules/lua/playlist/parser.lua");

  if (ret) {
    free(mocked_stream);
    return 1;
  }

  probe(mocked_stream);
  free(mocked_stream);
  return 0;
}
