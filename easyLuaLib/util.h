#pragma once

//#define YIELD_ACROSS_C_BOUNDARY

static const auto AAAA = 121;
struct lua_State;

int craeteClass(lua_State *L);
int ywlSetMetaTable(lua_State *L);

int checkIsInstance(lua_State *L);
void checkInstance(lua_State *L, int objIdx, int clsIdx);



int checkIsClass(lua_State *L);
int checkIsSubClass(lua_State *L);

bool createOrNewTable(lua_State *L, const void * key);
