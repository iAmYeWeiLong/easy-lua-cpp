#include "loop.h"
#include "../util.h"

#include <lua.hpp>
#include <uv.h>
#include <string>
using namespace std;

static int __new__(lua_State* L) {	
	auto const ARG_CLS = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	//�жϴ���������ϲ��Ϸ�
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);
	lua_pushvalue(L, THIS_CLASS);
	lua_call(L, 2, 0);
	//---end

	auto loop = (uv_loop_t**)lua_newuserdata(L, sizeof(uv_loop_t*));//[-0, +1, m]	
	*loop = new uv_loop_t;//uv_loop_new()��uv_loop_delete()���Ѿ�DEPRECATED	
	(*loop)->data = nullptr;

	lua_pushvalue(L, ARG_CLS);//��ʵ�ʴ��������ิ�Ƶ�ջ��
	lua_setmetatable(L, -2);//[-1, +0, -]
	return 1;
}

static int __init__(lua_State* L) {
	auto const SELF = 1, COROUTINE = 2,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto coroutine = lua_tothread(L, COROUTINE);
	if (nullptr == coroutine) {
		coroutine = L;
	}
	lua_createtable(coroutine, 1024, 0);//[-0, +1, e] ��Ϣ����,�������Ԫ���ǻص�����
	auto msg_queue_handle = luaL_ref(coroutine, LUA_REGISTRYINDEX);//[-1, +0, e] ��������ϵͳ
	
	auto ret = uv_loop_init(*self);
	(*self)->data = new cLoopInfo(msg_queue_handle, coroutine);

	if (ret != 0) {
		luaL_error(L, "cLoop��__init__ʧ��");
	}	
	return 0;
}

//�����ص����¼�ѭ��,����static,handle.cppҪ����
int run(lua_State* L) {
	auto const SELF = 1, MODE = 2,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
		
	auto self = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);
	luaL_checktype(L, MODE, LUA_TNUMBER);

	auto loopInfo = (cLoopInfo*)(*self)->data;
	if (L != loopInfo->coroutine) {
		luaL_error(L, "������loop�������Э�̽���__init__��ʼ��");
	}
	auto mode = lua_tointeger(L, MODE);
	auto beginStackSize = lua_gettop(L);
	int alive;

	try {
		//printf("uv_run_ begin top=%d......\n", lua_gettop(L));
		//����ű��з�����yield,uv_run�ᱻyield���ִ��
		alive = uv_run(*self, static_cast<uv_run_mode>(mode));//uv_run_mode::UV_RUN_DEFAULT
		//printf("uv_run_ end top=%d......\n", lua_gettop(L));
	}
	catch (...) {
		printf("uv_run_ end by catch ,top=%d.........\n", lua_gettop(L));
		throw;
	}
	if (lua_gettop(L) != beginStackSize) {
		luaL_error(L, "�¼��ص������в�����ջ�ϼ�����");
	}
	//�ص����������������,�п����ǿձ�
	lua_rawgeti(L, LUA_REGISTRYINDEX, loopInfo->msg_queue_handle);//[-0, +1, �C]
	lua_pushboolean(L, alive);
	return 2;
}

/*
int uv_run_2(lua_State* L);
//��������
static int continuation(lua_State *L, int status, lua_KContext ctx) {
	return uv_run_2(L);
}

//�ű���ں���
static int uv_run_2(lua_State* L) {
	auto const THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1;
	auto const MODE = 2;
	lua_pushvalue(L, THIS_CLASS);//����������upvalue
	lua_pushcclosure(L, uv_run_, 1);//��������
	lua_pushvalue(L, SELF);//����1
	lua_pushvalue(L, MODE);//����2
	try {
		//printf("uv_run_ begin.........\n");
		lua_callk(L, 2, 2, 0, continuation);//���uv_run_��yield�����,����continuationȥ
		//printf("uv_run_ end.........\n");
	}
	catch (...) {
		//printf("uv_run_ end  by catch .........\n");
		throw;
	}
	return 2;//�Ѿ�֪��uv_run_�᷵��2��ֵ����
}
*/

static int stop(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);

	uv_stop(*self);
	return 0;
}

static int alive(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
		
	auto self = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_loop_alive(*self);
	lua_pushboolean(L, ret);
	return 1;
}

static int close(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
		
	auto self = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_loop_close(*self);
	lua_pushinteger(L, ret);
	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);
	
	uv_stop(*self);
	auto ret = uv_loop_close(*self);

	auto loopInfo=(cLoopInfo*)(*self)->data;
	auto msg_queue_handle= loopInfo->msg_queue_handle;

	if (msg_queue_handle != LUA_NOREF) {//����˵,���ж�Ҳ����������
		luaL_unref(L, LUA_REGISTRYINDEX, msg_queue_handle); //[-0, +0, �C]
	}

	delete (*self);//uv_loop_delete(*loop)��uv_loop_new()����DEPRECATED��
	lua_pushinteger(L, ret);//����Ԫ����,�Ҷ���֪�������յ�����ֵ
	return 1;
}

static int now(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
		
	auto loop = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, loop, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto ret=uv_now(*loop);
	lua_pushinteger(L, ret);
	return 1;
}

static int __tostring(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto loop = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, loop, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);
	lua_pushfstring(L, "cLoop object: %p",loop);//[-0, +1, m]
	return 1;
}

void walk_cb(uv_handle_t* handle, void* arg) {
	//printf("walk_callback %d\n",handle->type);
}
static int walk(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto loop = (uv_loop_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, loop, SELF, "invalid loop");

	//��鴫������loopʵ��
	checkInstance(L, SELF, THIS_CLASS);

	uv_walk(*loop, walk_cb,nullptr);
	return 0;
}

static const struct luaL_Reg loop_m[] = {
	//{ "defaultLoop", uv_default_loop_ },//���io loop ,����¶�˽ӿ�
	{ "__new__", __new__ },
	{ "__init__", __init__ },
	{ "__gc", __gc },
	{ "run", run },
	{ "stop", stop },
	{ "close", close },
	{ "now", now },
	{ "alive", alive },
	
	//{ "__tostring", __tostring },
	{ "walk", walk },

	{ nullptr, nullptr }
};

int create_loop_class(lua_State *L) {
	lua_pushcfunction(L, craeteClass);
	lua_call(L, 0, 1);//������

	//��鴴�������ǲ�����
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);
	//---end

	lua_pushvalue(L, -1);//����һ������Ϊ������Ա������upvalue	
	luaL_setfuncs(L, loop_m, 1);//������Ϊȫ����Ա������upvalue.[-nup, +0, e]
	return 1;
}
