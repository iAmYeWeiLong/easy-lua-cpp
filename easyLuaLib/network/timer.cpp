#include "watcher.h"
#include "../util.h"
#include "handle.h"

#include <lua.hpp>
#include <uv.h>

static void deleter(uv_timer_t * handle) {
	//uv_close�������stop����
	uv_close((uv_handle_t*)handle, handle_close_cb);
}

static int __new__(lua_State* L) {
	auto const THIS_CLASS = lua_upvalueindex(1), ARG_CLS = 1;//[-0, +0, -]
	//�жϴ���������ϲ��Ϸ�
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);
	lua_pushvalue(L, THIS_CLASS);
	lua_call(L, 2, 0);
	//---end

	auto size = sizeof(std::shared_ptr<uv_timer_t>**);
	auto self = (std::shared_ptr<uv_timer_t>**)lua_newuserdata(L, size);//[-0, +1, m]
	*self = new std::shared_ptr<uv_timer_t>(new uv_timer_t, deleter);
	(**self)->data = new cHandleInfo();

	lua_pushvalue(L, ARG_CLS);//�Ѳ����д��������ิ�Ƶ�ջ��
	lua_setmetatable(L, -2);//[-1, +0, -]

	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto self = (std::shared_ptr<uv_timer_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid timer");

	//��鴫������timerʵ��
	checkInstance(L, SELF, THIS_CLASS);

	delete (*self);
	return 0;
}
static int __init__(lua_State* L) {
	auto const SELF = 1, LOOP_INSTANCE = 2,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_timer_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid timer");

	auto loop = (uv_loop_t**)lua_touserdata(L, LOOP_INSTANCE);
	luaL_argcheck(L, loop, LOOP_INSTANCE, "invalid loop");
	
	//��鴫������timerʵ��
	checkInstance(L, SELF, THIS_CLASS);

	//loopʵ������������,�ű�ʹ��ʱ�Լ�С�ĵ�

	auto ret = uv_timer_init(*loop, &***self);
	if (ret != 0) {
		luaL_error(L, "cTimer��__init__ʧ��");
	}
	return 0;
}

static int start(lua_State* L) {
	//if (lua_gettop(L) != 4) {
	//	luaL_error(L, "�յ�%d������,����ֵ��4��", lua_gettop(L));
	//}
	auto const SELF = 1, CALL_BACK = 2, TIMEOUT = 3, REPEAT = 4, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
		
	auto self = (std::shared_ptr<uv_timer_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid timer");

	//��鴫������timerʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto b = lua_isfunction(L, CALL_BACK) || lua_isnil(L, CALL_BACK);
	luaL_argcheck(L, b, CALL_BACK, "invalid function"); //������һ��nil���ص�����
	
	auto const timeout =(LUA_INTEGER)(lua_tonumber(L, TIMEOUT)*1000);
	auto const repeat =(LUA_INTEGER)(lua_tonumber(L, REPEAT) * 1000);

	auto info = (cHandleInfo*)(**self)->data;
	
	//��ֹ�ű��ظ���start
	//auto stopRet = uv_timer_stop(*self); //uv_timer_start�����Զ���stop
	if (info->callback_handle != LUA_NOREF) {//����˵,���ж�Ҳ����������
		luaL_unref(L, LUA_REGISTRYINDEX, info->callback_handle); //[-0, +0, �C]
		info->callback_handle = LUA_NOREF;
	}
	auto ret = uv_timer_start(&***self, uv_timer_cb(common_handle_cb),timeout,repeat);
	//�Ѳ����������Ļص������Ž�"����ϵͳ"(�ű�start��һ���ǵ�stop,��Ȼ�ű��ص�����һֱ��ǿ����,����ص������ֱհ��˶���,������һ���ӳ�������)
	
	if (lua_isfunction(L, CALL_BACK)) {
		lua_pushvalue(L, CALL_BACK);
		auto callback_handle = luaL_ref(L, LUA_REGISTRYINDEX);//[-1, +0, e]
		info->setInfo(callback_handle);
	}	
	lua_pushboolean(L, !ret);//0��Ȼ��ʾ�ɹ�
	return 1;
}
static int stop(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	
	auto self = (std::shared_ptr<uv_timer_t>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid timer");

	//��鴫������timerʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_timer_stop(&***self);

	auto info = (cHandleInfo*)(**self)->data;
	if (info->callback_handle != LUA_NOREF) {//����˵,���ж�Ҳ����������
		luaL_unref(L, LUA_REGISTRYINDEX, info->callback_handle); //[-0, +0, �C]
		info->callback_handle = LUA_NOREF;
	}
	lua_pushboolean(L, !ret);//0��Ȼ��ʾ�ɹ�
	return 1;
}

static const struct luaL_Reg timer_m[] = {
	{ "__new__", __new__ },
	{ "__gc", __gc },
	{ "__init__", __init__ },
	{ "start", start },
	{ "stop", stop },
	{ nullptr, nullptr }
};

int create_timer_class(lua_State *L) {
	auto const BASE_CLS = 1;
	//printf("create_timer_class lua_gettop==%d ...\n", lua_gettop(L));
	
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
	luaL_setfuncs(L, timer_m, 2);//������Ϊȫ����Ա������upvalue.[-nup, +0, e]
	return 1;
}