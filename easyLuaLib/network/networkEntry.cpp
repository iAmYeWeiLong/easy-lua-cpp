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

//��ֹ���Ƹı�,������������luaopen_����
extern "C" _declspec(dllexport) int luaopen_easyLuaLib_network(lua_State* L) {
	luaL_checkversion(L);
	if (lua_gettop(L) != 3) {
		luaL_error(L, "�յ�%d������,����ֵ��3��", lua_gettop(L));
	}
	auto modName = lua_tostring(L, 1);//ģ������
	auto filePath = lua_tostring(L, 2);//·��
	//printf("luaopen_libuv4lua ģ������ =%s\n", modName);
	//printf("luaopen_libuv4lua ·�� =%s\n", filePath);
		
	auto const SCRIPT_FUNCTIONS_INDEX = 3;//�ű��������λ��
		
	//--begin
	lua_pushvalue(L, SCRIPT_FUNCTIONS_INDEX);//���ƽű�������
	lua_setfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-1, +0, e]
	//---end

	luaL_newlib(L, mod_func);//[-0, +1, e]	
	auto MODULE_INDEX = lua_gettop(L);
	//===begin
	//---begin
	lua_pushinteger(L, uv_run_mode::UV_RUN_DEFAULT);//[-0, +1, �C]
	lua_setfield(L, MODULE_INDEX, "UV_RUN_DEFAULT");//[-1, +0, e]
	//---end
	//---begin
	lua_pushinteger(L, uv_run_mode::UV_RUN_NOWAIT);//[-0, +1, �C]
	lua_setfield(L, MODULE_INDEX, "UV_RUN_NOWAIT");//[-1, +0, e]
	//---end
	//---begin
	lua_pushinteger(L, uv_run_mode::UV_RUN_ONCE);//[-0, +1, �C]
	lua_setfield(L, MODULE_INDEX, "UV_RUN_ONCE");//[-1, +0, e]
	//---end
	//===end

	//����cLoop��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_loop_class);//[-0, +1, �C]	
	lua_call(L, 0, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cLoop");//������ΪĿ��ģ������� [-1, +0, e]
	//---end

	//����cHandle��,����Ϊ��lib��һ������
	lua_pushcfunction(L, create_handle_class);//[-0, +1, �C]	
	lua_call(L, 0, 1);//[-(nargs+1), +nresults, e]
	auto HANDLE_INDEX = lua_gettop(L);

	//---begin
	lua_pushvalue(L, HANDLE_INDEX);//handle�����滹Ҫ��Ϊ���˵ĸ���,����Ҫ����һ��
	lua_setfield(L, MODULE_INDEX, "cHandle");//������ΪĿ��ģ������� [-1, +0, e]
	//---end

	//����cIdle��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_idle_class);//[-0, +1, �C]
	lua_pushvalue(L, HANDLE_INDEX);//�������Ϊ�������	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cIdle");//������ΪĿ��ģ������� [-1, +0, e]
	//---end
	
	//����cPrepare��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_prepare_class);//[-0, +1, �C]
	lua_pushvalue(L, HANDLE_INDEX);//�������Ϊ�������	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cPrepare");//������ΪĿ��ģ������� [-1, +0, e]
	//---end

	//����cTimer��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_timer_class);//[-0, +1, �C]
	lua_pushvalue(L, HANDLE_INDEX);//�������Ϊ�������	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cTimer");//������ΪĿ��ģ������� [-1, +0, e]
	//---end

	//����cPoll��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_poll_class);//[-0, +1, �C]
	lua_pushvalue(L, HANDLE_INDEX);//�������Ϊ�������	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cPoll");//������ΪĿ��ģ������� [-1, +0, e]
	//---end

	//����cAsync��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_async_class);//[-0, +1, �C]
	lua_pushvalue(L, HANDLE_INDEX);//�������Ϊ�������	
	lua_call(L, 1, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cAsync");//������ΪĿ��ģ������� [-1, +0, e]
	//---end

	lua_pushvalue(L, MODULE_INDEX);
	return 1;
}