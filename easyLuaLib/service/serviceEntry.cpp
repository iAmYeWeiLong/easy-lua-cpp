// dllmain.cpp : Defines the entry point for the DLL application.
#include "../stdafx.h"
#include "serviceUtil.h"
#include "../util.h"
#include "lua-seri.h"

#include "message.h"
#include "context.h"
#include "serviceInfo.h"
#include "waiter.h"

#include <lua.hpp>
#include <string>
//#include <iostream>
#include <thread>
#include <vector>
#include <memory>

using namespace std;

int create_thisVM_class(lua_State *L);//����
int main(int, char **, void * ,void *);

static void splitString(const std::string& s, std::vector<std::string>& v, const std::string& c){
	std::string::size_type pos1 = 0, pos2 = s.find(c);	
	while (std::string::npos != pos2){
		v.push_back(s.substr(pos1, pos2 - pos1));
		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if(pos1 != s.length()) {
		v.push_back(s.substr(pos1));
	}
}

static void threadFunc(int argc, char **argv, void * waiterKey, void * waiter) {
	for (int i = 0; i < argc; ++i) {
		//printf("threadFunc  -- %d ,%s \n", i, argv[i]);
	}
	main(argc, argv, waiterKey, waiter);
	auto wt = (Waiter*)waiter;
	wt->notify(0,0);//�������Ϊ�쳣�˳�,Ҳ��֪ͨ���ȴ���.����Ѿ����ù�notify��,�ظ�����Ҳ����������
}

static int startService(lua_State* L) {
	const auto CMD_LINE = 1;
	auto const sCmdLine = std::string(lua_tostring(L, CMD_LINE));
	std::vector<std::string> args = {"EXE_HOLDER "};//������lua.c�Ĵ���,�õ��Ǳ�׼���������﷨.����startService��ʹ���߲����ᴫ��exe�����������
	splitString(sCmdLine, args, " ");
	char ** argv = new char *[args.size()];
	int i = 0;
	for (auto s : args) {
		argv[i] = new char[s.length() + 1]{0};
		strcpy(argv[i], s.c_str());
		i++;
	}
	Waiter wt;
	void * waiter_key = main;
	std::thread thread(threadFunc,static_cast<int>(args.size()), argv, waiter_key, &wt);
	thread.detach();

	wt.wait();//����������ȴ�
	
	lua_pushinteger(L, wt.getVmId());//����vm��id
	lua_pushinteger(L, wt.getServiceId());//����service��id
	return 2;
}

static int ltostring(lua_State *L) {
	if (lua_isnoneornil(L, 1)) {
		return 0;
	}
	auto pBuff = (std::shared_ptr<uint8_t> *)lua_touserdata(L, 1);
	auto msg = (char *)(&**pBuff);
	auto sz = luaL_checkinteger(L, 2);
	lua_pushlstring(L, msg, sz);
	return 1;
}
static int lpackstring(lua_State *L) {
	luaseri_pack(L);
	auto pBuff = (std::shared_ptr<uint8_t> *)lua_touserdata(L, -2);

	auto str = (char *)&(**pBuff);
	auto sz = lua_tointeger(L, -1);
	lua_pushlstring(L, str, sz);	
	delete pBuff;//skynet_free(str);
	return 1;
}

static int ltrash(lua_State *L) {
	int t = lua_type(L, 1);
	switch (t) {
	case LUA_TSTRING: {
		break;
	}
	case LUA_TLIGHTUSERDATA: {
		auto pBuff = (std::shared_ptr<uint8_t> *)lua_touserdata(L, 1);
		//luaL_checkinteger(L, 2);//������û��ʲô��
		delete pBuff;//skynet_free(msg);
		break;
	}
	default:
		luaL_error(L, "skynet.trash invalid param %s", lua_typename(L, t));
	}
	return 0;
}

static luaL_Reg mod_func[] = {
	{ "startService", startService },
	{ "pack", luaseri_pack },
	{ "unpack", luaseri_unpack },
	{ "tostring", ltostring },
	{ "packstring", lpackstring },
	{ "trash" , ltrash },
	{ nullptr, nullptr }
};

//��ֹ���Ƹı�,������������luaopen_����
extern "C" _declspec(dllexport) int luaopen_easyLuaLib_service(lua_State* L) {
	luaL_checkversion(L);
	if (lua_gettop(L) != 3) {
		luaL_error(L, "�յ�%d������,����ֵ��3��", lua_gettop(L));
	}
	auto const MOD_NAME = 1, FILE_PATH = 2, SCRIPT_FUNCTIONS_INDEX = 3;
	auto modName = lua_tostring(L, MOD_NAME);//ģ������
	auto filePath = lua_tostring(L, FILE_PATH);//·��
			
	//--begin
	lua_pushvalue(L, SCRIPT_FUNCTIONS_INDEX);//���ƽű�������
	lua_setfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-1, +0, e]
	//---end
	
	luaL_newlibtable(L, mod_func);//[-0, +1, e]
	auto const MODULE_INDEX = lua_gettop(L);
	luaL_setfuncs(L, mod_func, 0);//[-nup, +0, e]
		
	//����cThisVM��,����Ϊ��lib��һ������
	//---begin
	lua_pushcfunction(L, create_thisVM_class);//[-0, +1, �C]	
	lua_call(L, 0, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, MODULE_INDEX, "cThisVM");//������ΪĿ��ģ������� [-1, +0, e]
	//---end
	
	lua_pushvalue(L, MODULE_INDEX);
	return 1;
}