#include "handle.h"
#include "loop.h"
#include "../util.h"

#include <lua.hpp>
#include <uv.h>

static int __new__(lua_State* L) {
	luaL_error(L, "cHandle�ǳ�����,���ܴ���ʵ��");
	return 0;
}

static int isActive(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//��鴫������handleʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_is_active(*handle);
	lua_pushboolean(L, ret);
	return 1;
}
static int ref(lua_State* L) {	
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//��鴫������handleʵ��
	checkInstance(L, SELF, THIS_CLASS);

	uv_ref(*handle);
	return 0;
}


static int unRef(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//��鴫������handleʵ��
	checkInstance(L, SELF, THIS_CLASS);

	uv_unref(*handle);
	return 0;
}

static int hasRef(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//��鴫������handleʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto ret = uv_has_ref(*handle);
	lua_pushboolean(L, ret);
	return 1;
}

static int __tostring(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]

	auto handle = (uv_handle_t**)lua_touserdata(L, SELF);
	luaL_argcheck(L, handle, SELF, "invalid handle");

	//��鴫������handleʵ��
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
	//����¶uv_close���ű�,uv_close���ͷ���Դ������.����lua��__gc�ͷ���Դ����.
	//���ǽű�����stop,Ȼ��֤�����ж���,__gc��Ȼ�ᱻ����.
};

int create_handle_class(lua_State *L) {	
	lua_pushcfunction(L, craeteClass);
	lua_call(L, 0, 1);//������
	
	//��鴴�������ǲ�����
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);
	//---end
	
	lua_pushvalue(L, -1);//����һ������Ϊ������Ա������upvalue
	luaL_setfuncs(L, handle_m, 1);//������Ϊȫ����Ա������upvalue.[-nup, +0, e]
	return 1;
}
#ifdef YIELD_ACROSS_C_BOUNDARY
int uv_run_(lua_State* L);//����,������loop.cpp
//��������
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

//ͨ�õĻص�,���������� timer,async,prepare,check,idle,
//poll.cppҲ��������
void common_handle_cb(uv_handle_t * handle) {
	//����һ����ջ�Ͼ���2��ֵ,�Ǵӽű�����uv_run����.
	//��֤�˺�����Ҫ��ջ��ѹvalue,�����stack overflow

	auto data = (cHandleInfo*)handle->data;//��Σ�յĸо�
	auto callback_handle = data->callback_handle;
	if (LUA_NOREF == callback_handle) { //�ű����⴫��һ��nil���ص�����,�������Ϊ
		return;
	}
	auto loopInfo = (cLoopInfo*)handle->loop->data;
	auto coroutine = loopInfo->coroutine;

	//printf("common_handle_cb lua_gettop begin==%d\n", lua_gettop(coroutine));
#ifdef YIELD_ACROSS_C_BOUNDARY
	//ֻ����callk��pcallk,������call��pcall,���򱨴�attempt to yield across a C-call boundary
	
	lua_rawgeti(coroutine, LUA_REGISTRYINDEX, callback_handle);//value,[-0, +1, �C] //ȡ���ص�����	
	lua_callk(coroutine, 0, 0, (lua_KContext)0, continuation);//�п��ܻ�yield��uv_runѭ��;[-(nargs + 1), +nresults, e]
	//continuation(coroutine,  LUA_OK, (lua_KContext)0);//��Ϊû�к���������,����ע�͵�

	//lua_pcallk(coroutine, 0, 0, 0, (lua_KContext)0, continuation);//��ǿ������uv_runѭ��;//[-(nargs + 1), +(nresults|1), �C]
#else
	auto top = lua_gettop(coroutine);
	auto msg_queue_handle = loopInfo->msg_queue_handle;//��Ϣ���еľ��
	lua_rawgeti(coroutine, LUA_REGISTRYINDEX, msg_queue_handle);//[-0, +1, �C]
	size_t len = lua_rawlen(coroutine, -1);//[-0, +0, �C]
	lua_pushinteger(coroutine, len + 1);//key,[-0, +1, �C]
	lua_rawgeti(coroutine, LUA_REGISTRYINDEX, callback_handle);//value,[-0, +1, �C] //�ص�����
	lua_settable(coroutine, -3);//[-2, +0, e]
	//lua_pop(coroutine, 1);//�������,������ĸ�����һ��
	//��������˳���,���뱣֤ջ��û������
	auto add = lua_gettop(coroutine) - top;
	lua_pop(coroutine, add);
#endif
	//printf("common_handle_cb lua_gettop end==%d\n", lua_gettop(coroutine));
}


void handle_close_cb(uv_handle_t * handle) {
	auto handleInfo = (cHandleInfo*)handle->data;
	auto callback_handle = handleInfo->callback_handle;
	if (callback_handle != LUA_NOREF) {//����˵,���ж�Ҳ����������	
		auto loopInfo = (cLoopInfo*)handle->loop->data;
		luaL_unref(loopInfo->coroutine, LUA_REGISTRYINDEX, callback_handle); //[-0, +0, �C]
		handleInfo->callback_handle = LUA_NOREF;
	}
	delete handleInfo;
}