// dllmain.cpp : Defines the entry point for the DLL application.
#include "../stdafx.h"
#include "handle.h"
#include "loop.h"
#include "watcher.h"
#include "../util.h"

#include <lua.hpp>
#include <uv.h>
#include <string>
//#include <iostream>

using namespace std;

static int uv_version_(lua_State* L) {
	auto version = uv_version();
	lua_pushnumber(L, version);
	return 1;
}

static int uv_version_string_(lua_State* L) {
	auto version = uv_version_string();
	lua_pushstring(L, version);
	return 1;
}

static luaL_Reg mod_func[] = {
	{ "uv_version", uv_version_ },
	{ "uv_version_string", uv_version_string_ },
	{ nullptr, nullptr }
};

//禁止名称改编,函数名必须是luaopen_库名
extern "C" _declspec(dllexport) int luaopen_easyLuaLib_network(lua_State* L) {
	luaL_checkversion(L);
	if (lua_gettop(L) != 3) {
		luaL_error(L, "收到%d个参数,期望值是3个", lua_gettop(L));
	}
	auto modName = lua_tostring(L, 1);//模块名字
	auto filePath = lua_tostring(L, 2);//路径
	//printf("luaopen_libuv4lua 模块名字 =%s\n", modName);
	//printf("luaopen_libuv4lua 路径 =%s\n", filePath);
		
	auto const SCRIPT_FUNCTIONS_INDEX = 3;//脚本函数表的位置
		
	//--begin
	lua_pushvalue(L, SCRIPT_FUNCTIONS_INDEX);//复制脚本传来的
	lua_setfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-1, +0, e]
	//---end

	luaL_newlib(L, mod_func);//[-0, +1, e]	
	auto MODULE_INDEX = lua_gettop(L);
	//===begin
	//---begin
	lua_pushinteger(L, uv_run_mode::UV_RUN_DEFAULT);//[-0, +1, –]
	lua_setfield(L, MODULE_INDEX, "UV_RUN_DEFAULT");//[-1, +0, e]
	//---end
	//---begin
	lua_pushinteger(L, uv_run_mode::UV_RUN_NOWAIT);//[-0, +1, –]
	lua_setfield(L, MODULE_INDEX, "UV_RUN_NOWAIT");//[-1, +0, e]
	//---end
	//---begin
	lua_pushinteger(L, uv_run_mode::UV_RUN_ONCE);//[-0, +1, –]
	lua_setfield(L, MODULE_INDEX, "UV_RUN_ONCE");//[-1, +0, e]
	//---end
	//===end

	//创建cLoop类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_loop_class);//[-0, +1, –]	
	lua_call(L, 0, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cLoop");//把类设为目标模块的属性 [-1, +0, e]
	//---end

	//创建cHandle类,并设为此lib的一个属性
	lua_pushcfunction(L, create_handle_class);//[-0, +1, –]	
	lua_call(L, 0, 1);//[-(nargs+1), +nresults, e]
	auto HANDLE_INDEX = lua_gettop(L);

	//---begin
	lua_pushvalue(L, HANDLE_INDEX);//handle类下面还要作为别人的父类,所以要复制一个
	lua_setfield(L, MODULE_INDEX, "cHandle");//把类设为目标模块的属性 [-1, +0, e]
	//---end

	//创建cIdle类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_idle_class);//[-0, +1, –]
	lua_pushvalue(L, HANDLE_INDEX);//这个是作为父类参数	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cIdle");//把类设为目标模块的属性 [-1, +0, e]
	//---end
	
	//创建cPrepare类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_prepare_class);//[-0, +1, –]
	lua_pushvalue(L, HANDLE_INDEX);//这个是作为父类参数	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cPrepare");//把类设为目标模块的属性 [-1, +0, e]
	//---end

	//创建cTimer类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_timer_class);//[-0, +1, –]
	lua_pushvalue(L, HANDLE_INDEX);//这个是作为父类参数	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cTimer");//把类设为目标模块的属性 [-1, +0, e]
	//---end

	//创建cPoll类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_poll_class);//[-0, +1, –]
	lua_pushvalue(L, HANDLE_INDEX);//这个是作为父类参数	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cPoll");//把类设为目标模块的属性 [-1, +0, e]
	//---end

	//创建cAsync类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_async_class);//[-0, +1, –]
	lua_pushvalue(L, HANDLE_INDEX);//这个是作为父类参数	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cAsync");//把类设为目标模块的属性 [-1, +0, e]
	//---end

	lua_pushvalue(L, MODULE_INDEX);
	return 1;
}