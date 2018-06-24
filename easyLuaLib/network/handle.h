#pragma once
//handle��libuv����ĸ���,�Ǻܶ�watcher��"����"
//�����handle���а�װ

#include <lua.hpp> //Ϊ������LUA_NOREF
#include "uv.h" //Ϊ��ʹ��uv_handle_t
#include <memory> //todo:�ĳ�����,��include

struct lua_State;

int create_handle_class(lua_State *L);

//����uv_handle_t��data��
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