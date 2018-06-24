#pragma once
//#include <lua.hpp> //为了用上LUA_NOREF

struct lua_State;

int create_loop_class(lua_State *L);

//用于uv_handle_t的data域
class cLoopInfo {
public:
	cLoopInfo(int msg_queue_handle, lua_State * coroutine) {
		this->msg_queue_handle = msg_queue_handle;//LUA_NOREF
		this->coroutine = coroutine;
	}
	int msg_queue_handle;
	lua_State * coroutine;
};
