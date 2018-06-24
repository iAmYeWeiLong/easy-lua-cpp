#pragma once
//handle是libuv里面的概念,是很多watcher的"基类"
//这里对handle进行包装

#include <lua.hpp> //为了用上LUA_NOREF
#include "uv.h" //为了使用uv_handle_t
#include <memory> //todo:改成声明,不include

struct lua_State;

int create_handle_class(lua_State *L);

//用于uv_handle_t的data域
class cHandleInfo {
public:
	cHandleInfo() {
		this->callback_handle = LUA_NOREF;
	}
	void setInfo(int callback_handle) {
		this->callback_handle = callback_handle;
	}
	int callback_handle;
};


//typedef struct uv_handle_s uv_handle_t;

void common_handle_cb(uv_handle_t * handle);
void handle_close_cb(uv_handle_t * handle);