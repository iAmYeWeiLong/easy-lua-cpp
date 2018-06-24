#include "watcher.h"
#include "../util.h"
#include "handle.h"
#include <lua.hpp>
#include <uv.h>

static void deleter(uv_async_t * handle) {
	//printf("async.deleter  \n");
	//uv_close里面包含stop动作
	uv_close((uv_handle_t*)handle, handle_close_cb);
}

static int __new__(lua_State* L) {
	auto const ARG_CLS = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	//判断传过来的类合不合法
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

	lua_pushvalue(L, ARG_CLS);//把参数中传过来的类复制到栈顶
	lua_setmetatable(L, -2);//[-1, +0, -]

	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid async");
		
	checkInstance(L, SELF, THIS_CLASS);//检查传过来的async实例
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

	//检查传过来的async实例
	checkInstance(L, SELF, THIS_CLASS);

	//loop实例不方便检查了,脚本使用时自己小心点

	auto info = (cHandleInfo*)(**self)->data;
		
	if (info->callback_handle != LUA_NOREF) {//书上说,不判断也不会有问题
		luaL_unref(L, LUA_REGISTRYINDEX, info->callback_handle); //[-0, +0, –]
		info->callback_handle = LUA_NOREF;
	}
	
	auto ret = uv_async_init(*loop, &***self, uv_async_cb(common_handle_cb));//这个有点另类,在init就要传递callback
	if (ret != 0) {
		luaL_error(L, "cAsync的__init__失败");
	}
	//把参数传进来的回调函数放进"引用系统"(脚本start后一定记得stop,不然脚本回调函数一直被强引用,如果回调函数又闭包了对象,连对象一起被延长了生命)
	lua_pushvalue(L, CALL_BACK);
	auto callback_handle = luaL_ref(L, LUA_REGISTRYINDEX);//[-1, +0, e]
	info->setInfo(callback_handle);
	
	return 0;
}

static int send(lua_State* L) {
	auto const SELF = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]	
	auto self = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid async");

	//检查传过来的async实例
	checkInstance(L, SELF, THIS_CLASS);
	auto ret = uv_async_send(&***self);
	lua_pushboolean(L, !ret);//0竟然表示成功
	return 1;
}

static const struct luaL_Reg async_m[] = {
	{ "__new__", __new__ },
	{ "__gc", __gc },
	{ "__init__", __init__ },
	{ "send", send },
	//{ "start", start },libuv的async不提供start
	//{ "stop", stop },libuv的async不提供stop
	{ nullptr, nullptr }
};

int create_async_class(lua_State *L) {
	auto const BASE_CLS = 1;
	
	//检查父类是不是类
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, BASE_CLS);
	lua_call(L, 1, 0);
	//---end

	lua_pushcfunction(L, craeteClass);
	lua_pushvalue(L, BASE_CLS);//给个父类
	lua_call(L, 1, 1);//创建类

	//检查创建出来的是不是类
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);
	//--end

	lua_pushvalue(L, -1);//复制一个类作为各个成员函数的upvalue	
	lua_pushvalue(L, 1);//父类
	luaL_setfuncs(L, async_m, 2);//把类设为全部成员函数的upvalue.[-nup, +0, e]
	return 1;
}