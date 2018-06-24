#include "handle.h"
#include "loop.h"
#include "../util.h"

#include <lua.hpp>
#include <uv.h>

static int __new__(lua_State* L) {
	luaL_error(L, "cHandle是抽象类,不能创建实例");
	return 0;
}

static int isActive(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//检查传过来的handle实例
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_is_active(*handle);
	lua_pushboolean(L, ret);
	return 1;
}
static int ref(lua_State* L) {	
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//检查传过来的handle实例
	checkInstance(L, SELF, THIS_CLASS);

	uv_ref(*handle);
	return 0;
}


static int unRef(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//检查传过来的handle实例
	checkInstance(L, SELF, THIS_CLASS);

	uv_unref(*handle);
	return 0;
}

static int hasRef(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//检查传过来的handle实例
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_has_ref(*handle);
	lua_pushboolean(L, ret);
	return 1;
}

static int __tostring(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//检查传过来的handle实例
	checkInstance(L, SELF, THIS_CLASS);
	lua_pushfstring(L, "cHandle object: %p", handle);//[-0, +1, m]
	return 1;
}
static const struct luaL_Reg handle_m[] = {
	{ "__new__", __new__ },
	{ "__tostring", __tostring },
	{ "isActive", isActive },
	{ "ref", ref },
	{ "unRef", unRef },
	{ "hasRef", hasRef },
	{ nullptr, nullptr }
	//不暴露uv_close给脚本,uv_close是释放资源的语义.利用lua的__gc释放资源即可.
	//即是脚本进行stop,然后保证不持有对象,__gc自然会被调用.
};

int create_handle_class(lua_State *L) {	
	lua_pushcfunction(L, craeteClass);
	lua_call(L, 0, 1);//创建类
	
	//检查创建的类是不是类
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);
	//---end
	
	lua_pushvalue(L, -1);//复制一个类作为各个成员函数的upvalue
	luaL_setfuncs(L, handle_m, 1);//把类设为全部成员函数的upvalue.[-nup, +0, e]
	return 1;
}
#ifdef YIELD_ACROSS_C_BOUNDARY
int uv_run_(lua_State* L);//声明,定义在loop.cpp
//延续函数
int continuation(lua_State *L, int status, lua_KContext ctx) {

	//printf("continuation....status==%d  \n", status);
	//printf("continuation....lua_gettop==%d  \n", lua_gettop(L));
	//auto loop = (uv_loop_t*)ctx;
	//printf("continuation  loop.alive=%d\n",uv_loop_alive(loop));
	//uv_run(loop, uv_run_mode::UV_RUN_DEFAULT);
	if (status == LUA_YIELD) {
		return uv_run_(L);
	}
	return 0;
}
#endif

//通用的回调,单个参数的 timer,async,prepare,check,idle,
//poll.cpp也调到这里
void common_handle_cb(uv_handle_t * handle) {
	//这里一进来栈上就有2个值,是从脚本调用uv_run来的.
	//保证此函数不要往栈上压value,否则会stack overflow

	auto data = (cHandleInfo*)handle->data;//好危险的感觉
	auto callback_handle = data->callback_handle;
	if (LUA_NOREF == callback_handle) { //脚本故意传了一个nil作回调函数,允许的行为
		return;
	}
	auto loopInfo = (cLoopInfo*)handle->loop->data;
	auto coroutine = loopInfo->coroutine;

	//printf("common_handle_cb lua_gettop begin==%d\n", lua_gettop(coroutine));
#ifdef YIELD_ACROSS_C_BOUNDARY
	//只能用callk或pcallk,不能用call或pcall,否则报错attempt to yield across a C-call boundary
	
	lua_rawgeti(coroutine, LUA_REGISTRYINDEX, callback_handle);//value,[-0, +1, –] //取出回调函数	
	lua_callk(coroutine, 0, 0, (lua_KContext)0, continuation);//有可能会yield出uv_run循环;[-(nargs + 1), +nresults, e]
	//continuation(coroutine,  LUA_OK, (lua_KContext)0);//因为没有后续代码了,所以注释掉

	//lua_pcallk(coroutine, 0, 0, 0, (lua_KContext)0, continuation);//会强行跳出uv_run循环;//[-(nargs + 1), +(nresults|1), –]
#else
	auto top = lua_gettop(coroutine);
	auto msg_queue_handle = loopInfo->msg_queue_handle;//消息队列的句柄
	lua_rawgeti(coroutine, LUA_REGISTRYINDEX, msg_queue_handle);//[-0, +1, –]
	size_t len = lua_rawlen(coroutine, -1);//[-0, +0, –]
	lua_pushinteger(coroutine, len + 1);//key,[-0, +1, –]
	lua_rawgeti(coroutine, LUA_REGISTRYINDEX, callback_handle);//value,[-0, +1, –] //回调函数
	lua_settable(coroutine, -3);//[-2, +0, e]
	//lua_pop(coroutine, 1);//不用这个,用下面的更文艺一点
	//这个函数退出后,必须保证栈中没有增多
	auto add = lua_gettop(coroutine) - top;
	lua_pop(coroutine, add);
#endif
	//printf("common_handle_cb lua_gettop end==%d\n", lua_gettop(coroutine));
}


void handle_close_cb(uv_handle_t * handle) {
	auto handleInfo = (cHandleInfo*)handle->data;
	auto callback_handle = handleInfo->callback_handle;
	if (callback_handle != LUA_NOREF) {//书上说,不判断也不会有问题	
		auto loopInfo = (cLoopInfo*)handle->loop->data;
		luaL_unref(loopInfo->coroutine, LUA_REGISTRYINDEX, callback_handle); //[-0, +0, –]
		handleInfo->callback_handle = LUA_NOREF;
	}
	delete handleInfo;
}