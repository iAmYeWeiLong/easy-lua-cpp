#include "message.h"
#include "context.h"
#include "serviceInfo.h"
#include "waiter.h"
#include "serviceUtil.h"
#include "lua-seri.h"

#include "../util.h"
#include "../register.h"
#include "../singleton.h"

#include <lua.hpp>

#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1) * 8)

int main(int, char **, void *, void *);

static const char KEY_MEMBER_VAR_MAP;//��Ա�������key
static const char KEY_MEMBER_VAR_METATABLE;//��Ա�������Ԫ���key
static const char KEY_HAS_CREATE;//������־��key

static int __new__(lua_State* L) {
	auto const ARG_CLS = 1, SERVICE_NAME = 2, ASYNC_WATCHER = 3, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]	
	//�жϴ���������ϲ��Ϸ�
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);//[-0, +1, �C]
	lua_pushvalue(L, THIS_CLASS);//[-0, +1, �C]
	lua_call(L, 2, 0);
	//---end
	auto const serviceName = lua_tostring(L, SERVICE_NAME);
	if (strlen(serviceName) == 0) {
		luaL_error(L, "�������ֳ��Ȳ���Ϊ0");
	}
	//һ��vmֻ�ܴ���һ��ʵ��	
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, &KEY_HAS_CREATE) != LUA_TNIL) {//[-0, +1, �C]
		luaL_error(L, "cThisVM�ǵ���,�����ظ�����ʵ��");
	}

	auto async = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, ASYNC_WATCHER);
	luaL_argcheck(L, async, ASYNC_WATCHER, "invalid async");

	auto & serviceMgr = ServiceMgr::instance();	
	auto serviceInfo = serviceMgr.fetchOrCreate(serviceName, serviceName);
	
	//����Ԫ���ȡ��Ԫ��.�൱��t = setmetatable({},{__mode='k'})
	if (createOrNewTable(L, &KEY_MEMBER_VAR_METATABLE )) {//[-0, +1, e]
		lua_pushstring(L, "k");//[-0, +1, e]
		lua_setfield(L,-2, "__mode");//[-1, +0, e]
	}

	//�����������ȡ�ù�����
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, &KEY_MEMBER_VAR_MAP) == LUA_TNIL) {//[-0, +1, �C]
		lua_createtable(L, 0, 1);//[-0, +1, e]
		lua_rawgetp(L, LUA_REGISTRYINDEX, &KEY_MEMBER_VAR_METATABLE);//[-0, +1, �C]
		lua_setmetatable(L, -2);//[-1, +0, -]
		lua_pushvalue(L, -1);//[-0, +1, �C]
		lua_rawsetp(L, LUA_REGISTRYINDEX, &KEY_MEMBER_VAR_MAP);//[-1, +0, e] ������ջ����value
	}
	auto const MEMBER_VAR_MAP = lua_gettop(L);

	//��������ͨ��shared_ptr����,�����ط���weak_ptr����
	auto size = sizeof(std::shared_ptr<cVmContext>**);
	auto self = (std::shared_ptr<cVmContext>**)lua_newuserdata(L, size);//[-0, +1, e] ��Ϊģ�麯����upvalue
	auto const SELF = lua_gettop(L);

	
	//������,�����־,��������Ƿ��ظ�����
	lua_pushboolean(L, 1);//[-0, +1, e]
	lua_rawsetp(L, LUA_REGISTRYINDEX, &KEY_HAS_CREATE);//[-1, +0, e]

	*self = new std::shared_ptr<cVmContext>(new cVmContext(serviceInfo, **async));
		
	lua_pushvalue(L, SELF);//key [-0, +1, �C]
	lua_pushvalue(L, ASYNC_WATCHER);//value [-0, +1, �C]
	lua_settable(L, MEMBER_VAR_MAP);//[-2, +0, e]

	serviceInfo->checkIn(**self);
	auto & contextMgr = ContextMgr::instance();	
	contextMgr.checkIn(**self);	
		
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, main) != LUA_TNIL) {//[-0, +1, �C].ֻ��ʹ��startService���������ķ�������������
		auto wt = (Waiter*)lua_touserdata(L, -1);
		wt->notify((**self)->handle(), (**self)->serviceHandle());
		lua_pushnil(L);//[-0, +1, �C]
		lua_rawsetp(L, LUA_REGISTRYINDEX, main);//[-1, +0, e] 
	}
	if (serviceInfo->getVmCount() == 1) {//�����������ĵ�һ��vm,֪ͨ����service�ĸ���vm	
	}

	lua_pushvalue(L, ARG_CLS);//[-0, +1, �C] �Ѳ����д��������ิ�Ƶ�ջ��
	lua_setmetatable(L, SELF);//[-1, +0, -]
	lua_pushvalue(L, SELF);//[-0, +1, �C]
	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid thisVM");

	checkInstance(L, SELF, THIS_CLASS);//��鴫������thisVMʵ��
	//printf("cThisVM.__gc....\n");
	delete (*self);//�п����ͷ�������vm context,ȡ������û�����ü���
	return 0;
}

/*
��ʱû��__init__������,�����ʼ���Ѿ�����Ĺ��캯���������
static int __init__(lua_State* L) {	
	return 0;
}
*/

static int sendByVmId(lua_State* L) {
	auto const SELF = 1, TARGET_VM_ID = 2, MSG = 3, SIZE = 4, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);	
	checkInstance(L, SELF, THIS_CLASS);//��鴫������thisVMʵ��

	auto const targetVmId = lua_tointeger(L, TARGET_VM_ID);	

	auto charMsg = (std::shared_ptr<uint8_t> *)lua_touserdata(L, MSG);
	auto size = luaL_checkinteger(L, SIZE);

	Message msg((**self)->service->serviceName(), (**self)->handle(), (**self)->newSession(), *charMsg, size | ((size_t)MSG_TYPE_LUA << MESSAGE_TYPE_SHIFT));

	auto & contextMgr = ContextMgr::instance();
	auto targetVmContext = contextMgr.fetch(static_cast<int>(targetVmId));
	if (targetVmContext) {
		targetVmContext->pushMsg(msg);
		lua_pushboolean(L, 1);	
	}else{
		lua_pushboolean(L, 0);
	}
	return 1;
}


static int iAmReady(lua_State* L) {
	auto const SELF = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid thisVM");

	checkInstance(L, SELF, THIS_CLASS);//��鴫������thisVMʵ��
	auto & contextMgr = ContextMgr::instance();

	lua_pushcfunction(L, luaseri_pack);
	lua_pushstring(L, "REPORT");
	lua_pushinteger(L, (**self)->handle());
	lua_pushstring(L, (**self)->service->serviceName().c_str());
	lua_call(L, 3, 2);
	auto len = lua_tointeger(L, -1);
	auto pBuff = (std::shared_ptr<uint8_t> *)lua_touserdata(L, -2);

	for (auto paire : contextMgr.getItemsMap()) {
		auto obj = (paire.second).lock();
		//�����Լ�
		if ((**self)->service->serviceName() == obj->service->serviceName()) {
			continue;
		}
		Message msg((**self)->service->serviceName(), (**self)->handle(), 0, *pBuff,len | ((size_t)MSG_TYPE_TEXT << MESSAGE_TYPE_SHIFT));
		obj->pushMsg(msg);
	}
	delete pBuff;
	//auto targetVmContext = contextMgr.fetch(static_cast<int>((**self)->handle()));
	//auto & serviceMgr = ServiceMgr::instance();
	return 0;
}

static int sendByServiceName(lua_State* L) {
	auto const SELF = 1, SERVICE_NAME = 2, MSG = 3, SIZE = 4, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);
	//��鴫������thisVMʵ��
	checkInstance(L, SELF, THIS_CLASS);

	auto const serviceName = lua_tostring(L, SERVICE_NAME);
	auto charMsg = (std::shared_ptr<uint8_t> *)lua_touserdata(L, MSG);
	auto size = luaL_checkinteger(L, SIZE);

	Message msg((**self)->service->serviceName(), (**self)->handle(), (**self)->newSession(), *charMsg, size|((size_t)MSG_TYPE_LUA << MESSAGE_TYPE_SHIFT));

	auto & serviceMgr = ServiceMgr::instance();
	auto service = serviceMgr.fetch(serviceName);
	if (service) {
		service->pushMsg2FreeVm(msg);
		lua_pushboolean(L, 1);
	}
	else {
		lua_pushboolean(L, 0);
	}
	return 1;
}
static int getInternalMsgNonBlock(lua_State* L) {
	auto const SELF = 1, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid thisVM");
		
	checkInstance(L, SELF, THIS_CLASS);//��鴫������thisVMʵ��
	auto msg = (**self)->popMsg();
	if (msg.invalid()) {
		return 0;
	}else{
		int type = msg.sz >> MESSAGE_TYPE_SHIFT;
		size_t sz = msg.sz & MESSAGE_TYPE_MASK;
		lua_pushstring(L, msg.sourceServiceName.c_str());
		lua_pushinteger(L, msg.sourceVm);
		lua_pushinteger(L, msg.session);
		lua_pushinteger(L, type);
		//lua_pushlstring(L,(char*)msg.data, sz);
		auto pBuffer = new std::shared_ptr<uint8_t>(msg.data);//��������һ��

		lua_pushlightuserdata(L, pBuffer);//�ű�����Ҫ���е���trash
		lua_pushinteger(L, sz);
		return 6;
	}
}
static const struct luaL_Reg thisVM_m[] = {
	{ "__new__", __new__ },
	{ "__gc", __gc },
	//{ "__init__", __init__ },//��ʱû��������
		
	{ "getInternalMsgNonBlock" , getInternalMsgNonBlock },
	{ "sendByVmId" , sendByVmId },
	//{ "sendByServiceId" , sendByServiceId },
	{ "sendByServiceName", sendByServiceName },
	{ "iAmReady", iAmReady },
	{ nullptr, nullptr }
};

int create_thisVM_class(lua_State *L) {
	lua_pushcfunction(L, craeteClass);
	lua_call(L, 0, 1);//������
	
	//��鴴���������ǲ�����
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);//[-0, +1, �C]
	lua_call(L, 1, 0);
	//--end
	
	lua_pushvalue(L, -1);//[-0, +1, �C]����һ������Ϊ������Ա������upvalue
	luaL_setfuncs(L, thisVM_m, 1);//������Ϊȫ����Ա������upvalue.[-nup, +0, e]
	return 1;
}