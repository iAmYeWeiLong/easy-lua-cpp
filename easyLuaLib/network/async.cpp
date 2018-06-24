#include "watcher.h"
#include "../util.h"
#include "handle.h"
#include <lua.hpp>
#include <uv.h>

static void deleter(uv_async_t * handle) {
	//printf("async.deleter  \n");
	//uv_close�������stop����
	uv_close((uv_handle_t*)handle, handle_close_cb);
}

static int __new__(lua_State* L) {
	auto const ARG_CLS = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	//�жϴ���������ϲ��Ϸ�
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);
	lua_pushvalue(L, THIS_CLASS);
	lua_call(L, 2, 0);
	//---end

	auto size = sizeof(std::shared_ptr<uv_async_t>**);
	auto self = (std::shared_ptr<uv_async_t>**)lua_newuserdata(L, size);//[-0, +1, m]
	*self = new std::shared_ptr<uv_async_t>(new uv_async_t, deleter);
	(**self)->data = new cHandleInfo();

	lua_pushvalue(L, ARG_CLS);//�Ѳ����д��������ิ�Ƶ�ջ��
	lua_setmetatable(L, -2);//[-1, +0, -]

	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid async");
		
	checkInstance(L, SELF, THIS_CLASS);//��鴫������asyncʵ��
	//printf("async.__gc  \n");
	delete (*self);
	return 0;
}
static int __init__(lua_State* L) {
	auto const SELF = 1, LOOP_INSTANCE = 2, CALL_BACK = 3, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid async");

	auto loop = (uv_loop_t**)lua_touserdata(L, LOOP_INSTANCE);
	luaL_argcheck(L, loop, LOOP_INSTANCE, "invalid loop");

	auto b = lua_isfunction(L, CALL_BACK);
	luaL_argcheck(L, b, CALL_BACK, "invalid function");

	//��鴫������asyncʵ��
	checkInstance(L, SELF, THIS_CLASS);

	//loopʵ������������,�ű�ʹ��ʱ�Լ�С�ĵ�

	auto info = (cHandleInfo*)(**self)->data;
		
	if (info->callback_handle != LUA_NOREF) {//����˵,���ж�Ҳ����������
		luaL_unref(L, LUA_REGISTRYINDEX, info->callback_handle); //[-0, +0, �C]
		info->callback_handle = LUA_NOREF;
	}
	
	auto ret = uv_async_init(*loop, &***self, uv_async_cb(common_handle_cb));//����е�����,��init��Ҫ����callback
	if (ret != 0) {
		luaL_error(L, "cAsync��__init__ʧ��");
	}
	//�Ѳ����������Ļص������Ž�"����ϵͳ"(�ű�start��һ���ǵ�stop,��Ȼ�ű��ص�����һֱ��ǿ����,����ص������ֱհ��˶���,������һ���ӳ�������)
	lua_pushvalue(L, CALL_BACK);
	auto callback_handle = luaL_ref(L, LUA_REGISTRYINDEX);//[-1, +0, e]
	info->setInfo(callback_handle);
	
	return 0;
}

static int send(lua_State* L) {
	auto const SELF = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]	
	auto self = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid async");

	//��鴫������asyncʵ��
	checkInstance(L, SELF, THIS_CLASS);
	auto ret = uv_async_send(&***self);
	lua_pushboolean(L, !ret);//0��Ȼ��ʾ�ɹ�
	return 1;
}

static const struct luaL_Reg async_m[] = {
	{ "__new__", __new__ },
	{ "__gc", __gc },
	{ "__init__", __init__ },
	{ "send", send },
	//{ "start", start },libuv��async���ṩstart
	//{ "stop", stop },libuv��async���ṩstop
	{ nullptr, nullptr }
};

int create_async_class(lua_State *L) {
	auto const BASE_CLS = 1;
	
	//��鸸���ǲ�����
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, BASE_CLS);
	lua_call(L, 1, 0);
	//---end

	lua_pushcfunction(L, craeteClass);
	lua_pushvalue(L, BASE_CLS);//��������
	lua_call(L, 1, 1);//������

	//��鴴���������ǲ�����
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);
	//--end

	lua_pushvalue(L, -1);//����һ������Ϊ������Ա������upvalue	
	lua_pushvalue(L, 1);//����
	luaL_setfuncs(L, async_m, 2);//������Ϊȫ����Ա������upvalue.[-nup, +0, e]
	return 1;
}