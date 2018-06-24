#include "watcher.h"
#include "../util.h"
#include "handle.h"
#include <lua.hpp>
#include <uv.h>

static void deleter(uv_poll_t * handle) {
	//uv_close里面包含stop动作
	uv_close((uv_handle_t*)handle, handle_close_cb);
}

static int __new__(lua_State* L) {
	auto const ARG_CLS = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	//判断传过来的类合不合法
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);
	lua_pushvalue(L, THIS_CLASS);
	lua_call(L, 2, 0);
	//---end

	auto size = sizeof(std::shared_ptr<uv_poll_t>**);
	auto self = (std::shared_ptr<uv_poll_t>**)lua_newuserdata(L, size);//[-0, +1, m]
	*self = new std::shared_ptr<uv_poll_t>(new uv_poll_t, deleter);
	(**self)->data = new cHandleInfo();

	lua_pushvalue(L, ARG_CLS);//把参数中传过来的类复制到栈顶
	lua_setmetatable(L, -2);//[-1, +0, -]

	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_poll_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid poll");

	//检查传过来的poll实例
	checkInstance(L, SELF, THIS_CLASS);

	delete (*self);
	return 0;
}

static int __init__(lua_State* L) {
	auto const SELF = 1, LOOP_INSTANCE = 2, FD = 3, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_poll_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid poll");

	auto loop = (uv_loop_t**)lua_touserdata(L, LOOP_INSTANCE);
	luaL_argcheck(L, loop, LOOP_INSTANCE, "invalid loop");

	auto const fd = static_cast<int>(lua_tointeger(L, FD));
	//检查传过来的poll实例
	checkInstance(L, SELF, THIS_CLASS);

	//loop实例不方便检查了,脚本使用时自己小心点
	//auto ret = uv_poll_init(*loop, *self, fd);//实验证明,这个无法工作
	auto ret = uv_poll_init_socket(*loop, &***self, fd);//luasocket的socket对象getfd()得到的fd只能用这个uv_poll_init_socket_的进行初始化
	if (ret!=0) {
		luaL_error(L, "cPoll的__init__失败");
	}
	return 0;
}
static void uv_poll_cb_(uv_poll_t * handle, int status, int events) {
	//回调的2个参数int status, int events好像没有什么卵用
	common_handle_cb((uv_handle_t *)handle);
}
static int start(lua_State* L) {
	auto const SELF = 1, CALL_BACK = 2, EVENTS = 3,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_poll_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid poll");
	auto const events = (LUA_INTEGER)lua_tointeger(L, EVENTS);
	//检查传过来的poll实例
	checkInstance(L, SELF, THIS_CLASS);

	auto b = lua_isfunction(L, CALL_BACK);
	luaL_argcheck(L, b, CALL_BACK, "invalid function");

	auto info = (cHandleInfo*)(**self)->data;
	//防止脚本重复地start
	//auto stopRet = uv_poll_stop(*self);// 没有必要真的stop,更换掉脚本的回调函数就可以了
	if (info->callback_handle != LUA_NOREF) {//书上说,不判断也不会有问题
		luaL_unref(L, LUA_REGISTRYINDEX, info->callback_handle); //[-0, +0, –]
		info->callback_handle = LUA_NOREF;
	}
	auto ret = uv_poll_start(&***self,(int)events, uv_poll_cb_);

	//把参数传进来的回调函数放进"引用系统"(脚本start后一定记得stop,不然脚本回调函数一直被强引用,如果回调函数又闭包了对象,连对象一起被延长了生命)
	lua_pushvalue(L, CALL_BACK);
	auto callback_handle = luaL_ref(L, LUA_REGISTRYINDEX);//[-1, +0, e]
	info->setInfo(callback_handle);
	lua_pushboolean(L, !ret);//0竟然表示成功
	return 1;
}
static int stop(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_poll_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid poll");

	//检查传过来的poll实例
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_poll_stop(&***self);

	auto info = (cHandleInfo*)(**self)->data;
	if (info->callback_handle != LUA_NOREF) {//书上说,不判断也不会有问题
		luaL_unref(L, LUA_REGISTRYINDEX, info->callback_handle); //[-0, +0, –]
		info->callback_handle = LUA_NOREF;
	}
	lua_pushboolean(L, !ret);//0竟然表示成功
	return 1;
}

static const struct luaL_Reg poll_m[] = {
	{ "__new__", __new__ },
	{ "__gc", __gc },	
	{ "__init__", __init__ },
	{ "start", start },
	{ "stop", stop },
	{ nullptr, nullptr }
};

int create_poll_class(lua_State *L) {
	auto const BASE_CLS = 1;
	//printf("create_poll_class lua_gettop==%d ...\n", lua_gettop(L));

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
	luaL_setfuncs(L, poll_m, 2);//把类设为全部成员函数的upvalue.[-nup, +0, e]
	return 1;
}