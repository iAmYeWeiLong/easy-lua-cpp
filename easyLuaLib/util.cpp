#include "util.h"
//#include "uv.h"


#include <lua.hpp>
#include <string>

using namespace std;

static int openF(lua_State *L) {
	auto modName = lua_tostring(L, 1);//[-0, +0, m]
	auto modNameWithPostfix = modName + string(".lua");

	auto status = luaL_loadfile(L, modNameWithPostfix.c_str());//[-0, +1, m]
	if (status != 0) {
		auto s = lua_tostring(L, -1);//[-0, +0, m]
		luaL_error(L, s);
		//lua_pop(L, 1);//上面都error了,走不到这里了
	}

	lua_pushstring(L, modName);//这个是参数,模块名字,class.lua模块中可以用到的...[-0, +1, m]
	lua_call(L, 1, 0);//[-(nargs + 1), +nresults, e] class.lua最后一行并没有返回_ENV

	//这里有个约定:上面被loadfile后并call之后的模块要在package.loaded[modName]=mod
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");//
	lua_getfield(L, -1, modName);//
	return 1;
}

int craeteClass(lua_State *L) {
	auto baseCount = lua_gettop(L);
	if(0!= baseCount){//有父类
		for (auto i = 1; i <= baseCount; ++i) {
			if (!lua_istable(L, i)){//检查的是不是一个类
				luaL_error(L, "base class should be a table ");
			}
		}
	}
	auto const modName = "class";//试过了,暂时不支持模块之间用点分隔
	auto const funName = "create";
	luaL_requiref(L, modName, openF, 0);//[-0, +1, e]
	lua_getfield(L, -1, funName);//[-0, +1, e]
	if (!lua_isfunction(L, -1)) {
		luaL_error(L, " 函数%s找不到", funName);
	}
	//压上参数,即是n个父类
	for (auto i = 1; i <= baseCount; ++i) {
		lua_pushvalue(L, i);
	}
	lua_call(L, baseCount, 1);//创建类
	if (!lua_istable(L, -1)) {//检查返回的是不是一个类
		luaL_error(L, " class should be a table ");
	}
	return 1;
}

int ywlSetMetaTable(lua_State *L) {
	if (!lua_istable(L, 1) && !lua_isuserdata(L, 1) && !lua_isthread(L,1)) {
		luaL_argerror(L, 1, "实例参数必须是table或userdata");
	}
	if (!lua_istable(L, 2)) {
		luaL_argerror(L, 2, "元表参数必须是table");
	}
	lua_setmetatable(L, -2);//[-1, +0, -] 弹掉栈顶的metatable,并将栈顶metatable设为index所指向的value的metatable
	return 0;
}
//检查实例
int checkIsInstance(lua_State *L) {
	auto const INSTANCE = 1;
	auto const CLASS = 2;

	lua_getfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-0, +1, e]
	lua_getfield(L, -1, "isInstance");//[-0, +1, e]
	lua_pushvalue(L, INSTANCE);//实例
	lua_pushvalue(L, CLASS);//类
	lua_call(L, 2, 1);
	if (!lua_isboolean(L, -1)) {//[-0, +0, v]
		luaL_error(L, "isInstance的返回值不是bool型");
	}
	if (!lua_toboolean(L, -1)) {//[-0, +0, v]
		luaL_error(L, "此实例不属于此类产生的");
	}
	return 0;
}
void checkInstance(lua_State *L,int objIdx,int clsIdx) {
	//检查传过来的实例是否属于这个类
	//---begin
	lua_pushcfunction(L, checkIsInstance);
	lua_pushvalue(L, objIdx);
	lua_pushvalue(L, clsIdx);
	lua_call(L, 2, 0);
	//---end
}

//检查是不是class,不是则抛异常
int checkIsClass(lua_State *L) {
	auto const CLASS = 1;
	if (!lua_istable(L, CLASS)) {//[-0, +0, –]
		auto iType = lua_type(L, CLASS);
		auto sName = luaL_typename(L, iType);
		luaL_error(L, "不是一个类,连table都不是,类应该是table,不能是%s", sName);
	}
	lua_pushstring(L, "__isClass__");
	lua_rawget(L, CLASS); //[-1, +1, –] 避免触发元方法,否则对一个实例的访问得到的是类的属性
	auto ret = lua_toboolean(L, -1);//[-0, +0, –]
	luaL_argcheck(L, ret, CLASS, "不是一个类");//[-0, +0, v]
	return 0;	
}

//检查类a是不是类b的子类,不是则抛异常
int checkIsSubClass(lua_State *L) {
	auto const parameterCount = lua_gettop(L);
	if (parameterCount != 2) {
		luaL_error(L, "收到%d个参数,期望值是2个", parameterCount);
	}
	//检查2个参数是不是类
	for (auto i = 1; i <= parameterCount; ++i) {
		lua_pushcfunction(L, checkIsClass);
		lua_pushvalue(L, i);
		lua_call(L, 1, 0);
	}
	//调用脚本层的isSubClass判断传过来的类合不合法
	lua_getfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-0, +1, e]
	lua_getfield(L, -1, "isSubClass");//[-0, +1, e]
	lua_pushvalue(L, 1);//子类
	lua_pushvalue(L, 2);//父类
	lua_call(L, 2, 1);
	luaL_argcheck(L, lua_isboolean(L, -1), 1, "isSubClass的返回值不是bool型");//[-0, +0, v]
	luaL_argcheck(L, lua_toboolean(L, -1), 1, "给错类了,类方法传的类不正确");
	
	return 0;
}


//[-0, +1, e]
bool createOrNewTable(lua_State *L, const void * key) {
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, key) != LUA_TNIL)//[-0, +1, –]
		return false;  /* leave previous value on top, but return 0 */
	lua_pop(L, 1);//弹掉nil
	lua_createtable(L, 0, 2);//[-0, +1, e]
	lua_pushvalue(L, -1);//[-0, +1, e]
	lua_rawsetp(L, LUA_REGISTRYINDEX, key);//[-1, +0, e] 操作是栈顶的value
	return true;
}