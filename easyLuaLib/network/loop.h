#pragma once
//#include <lua.hpp> //Ϊ������LUA_NOREF

struct lua_State;

int create_loop_class(lua_State *L);

//����uv_handle_t��data��
class cLoopInfo {
public:
	cLoopInfo(int msg_queue_handle, lua_State * coroutine) {
		this->msg_queue_handle = msg_queue_handle;//LUA_NOREF
		this->coroutine = coroutine;
	}
	int msg_queue_handle;
	lua_State * coroutine;
};
