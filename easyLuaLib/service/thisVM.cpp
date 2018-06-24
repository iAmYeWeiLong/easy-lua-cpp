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

static const char KEY_MEMBER_VAR_MAP;//成员变量表的key
static const char KEY_MEMBER_VAR_METATABLE;//成员变量表的元表的key
static const char KEY_HAS_CREATE;//创建标志的key

static int __new__(lua_State* L) {
	auto const ARG_CLS = 1, SERVICE_NAME = 2, ASYNC_WATCHER = 3, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]	
	//判断传过来的类合不合法
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);//[-0, +1, –]
	lua_pushvalue(L, THIS_CLASS);//[-0, +1, –]
	lua_call(L, 2, 0);
	//---end
	auto const serviceName = lua_tostring(L, SERVICE_NAME);
	if (strlen(serviceName) == 0) {
		luaL_error(L, "服务名字长度不能为0");
	}
	//一个vm只能创建一个实例	
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, &KEY_HAS_CREATE) != LUA_TNIL) {//[-0, +1, –]
		luaL_error(L, "cThisVM是单例,不能重复创建实例");
	}

	auto async = (std::shared_ptr<uv_async_t>**)lua_touserdata(L, ASYNC_WATCHER);
	luaL_argcheck(L, async, ASYNC_WATCHER, "invalid async");

	auto & serviceMgr = ServiceMgr::instance();	
	auto serviceInfo = serviceMgr.fetchOrCreate(serviceName, serviceName);
	
	//创建元表或取得元表.相当于t = setmetatable({},{__mode='k'})
	if (createOrNewTable(L, &KEY_MEMBER_VAR_METATABLE )) {//[-0, +1, e]
		lua_pushstring(L, "k");//[-0, +1, e]
		lua_setfield(L,-2, "__mode");//[-1, +0, e]
	}

	//创建关联表或取得关联表
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, &KEY_MEMBER_VAR_MAP) == LUA_TNIL) {//[-0, +1, –]
		lua_createtable(L, 0, 1);//[-0, +1, e]
		lua_rawgetp(L, LUA_REGISTRYINDEX, &KEY_MEMBER_VAR_METATABLE);//[-0, +1, –]
		lua_setmetatable(L, -2);//[-1, +0, -]
		lua_pushvalue(L, -1);//[-0, +1, –]
		lua_rawsetp(L, LUA_REGISTRYINDEX, &KEY_MEMBER_VAR_MAP);//[-1, +0, e] 操作是栈顶的value
	}
	auto const MEMBER_VAR_MAP = lua_gettop(L);

	//生命期是通过shared_ptr管理,其他地方用weak_ptr引用
	auto size = sizeof(std::shared_ptr<cVmContext>**);
	auto self = (std::shared_ptr<cVmContext>**)lua_newuserdata(L, size);//[-0, +1, e] 作为模块函数的upvalue
	auto const SELF = lua_gettop(L);

	
	//单例的,设个标志,用来检查是否重复创建
	lua_pushboolean(L, 1);//[-0, +1, e]
	lua_rawsetp(L, LUA_REGISTRYINDEX, &KEY_HAS_CREATE);//[-1, +0, e]

	*self = new std::shared_ptr<cVmContext>(new cVmContext(serviceInfo, **async));
		
	lua_pushvalue(L, SELF);//key [-0, +1, –]
	lua_pushvalue(L, ASYNC_WATCHER);//value [-0, +1, –]
	lua_settable(L, MEMBER_VAR_MAP);//[-2, +0, e]

	serviceInfo->checkIn(**self);
	auto & contextMgr = ContextMgr::instance();	
	contextMgr.checkIn(**self);	
		
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, main) != LUA_TNIL) {//[-0, +1, –].只有使用startService函数启动的服务才有这个变量
		auto wt = (Waiter*)lua_touserdata(L, -1);
		wt->notify((**self)->handle(), (**self)->serviceHandle());
		lua_pushnil(L);//[-0, +1, –]
		lua_rawsetp(L, LUA_REGISTRYINDEX, main);//[-1, +0, e] 
	}
	if (serviceInfo->getVmCount() == 1) {//本服务启动的第一个vm,通知其他service的各个vm	
	}

	lua_pushvalue(L, ARG_CLS);//[-0, +1, –] 把参数中传过来的类复制到栈顶
	lua_setmetatable(L, SELF);//[-1, +0, -]
	lua_pushvalue(L, SELF);//[-0, +1, –]
	return 1;
}

static int __gc(lua_State* L) {
	auto const SELF = 1,THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);
	luaL_argcheck(L, self, SELF, "invalid thisVM");

	checkInstance(L, SELF, THIS_CLASS);//检查传过来的thisVM实例
	//printf("cThisVM.__gc....\n");
	delete (*self);//有可能释放真正的vm context,取决于有没有引用计数
	return 0;
}

/*
暂时没有__init__的需求,对象初始化已经在类的构造函数中完成了
static int __init__(lua_State* L) {	
	return 0;
}
*/

static int sendByVmId(lua_State* L) {
	auto const SELF = 1, TARGET_VM_ID = 2, MSG = 3, SIZE = 4, THIS_CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto self = (std::shared_ptr<cVmContext>**)lua_touserdata(L, SELF);	
	checkInstance(L, SELF, THIS_CLASS);//检查传过来的thisVM实例

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

	checkInstance(L, SELF, THIS_CLASS);//检查传过来的thisVM实例
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
		//跳过自己
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
	//检查传过来的thisVM实例
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
		
	checkInstance(L, SELF, THIS_CLASS);//检查传过来的thisVM实例
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
		auto pBuffer = new std::shared_ptr<uint8_t>(msg.data);//拷贝构造一个

		lua_pushlightuserdata(L, pBuffer);//脚本用完要自行调用trash
		lua_pushinteger(L, sz);
		return 6;
	}
}
static const struct luaL_Reg thisVM_m[] = {
	{ "__new__", __new__ },
	{ "__gc", __gc },
	//{ "__init__", __init__ },//暂时没有这需求
		
	{ "getInternalMsgNonBlock" , getInternalMsgNonBlock },
	{ "sendByVmId" , sendByVmId },
	//{ "sendByServiceId" , sendByServiceId },
	{ "sendByServiceName", sendByServiceName },
	{ "iAmReady", iAmReady },
	{ nullptr, nullptr }
};

int create_thisVM_class(lua_State *L) {
	lua_pushcfunction(L, craeteClass);
	lua_call(L, 0, 1);//创建类
	
	//检查创建出来的是不是类
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);//[-0, +1, –]
	lua_call(L, 1, 0);
	//--end
	
	lua_pushvalue(L, -1);//[-0, +1, –]复制一个类作为各个成员函数的upvalue
	luaL_setfuncs(L, thisVM_m, 1);//把类设为全部成员函数的upvalue.[-nup, +0, e]
	return 1;
}