#pragma once
struct lua_State;

int create_async_class(lua_State *L);
int create_poll_class(lua_State *L);
int create_timer_class(lua_State *L);
int create_prepare_class(lua_State *L);
int create_idle_class(lua_State *L);