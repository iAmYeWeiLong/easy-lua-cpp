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
		//lua_pop(L, 1);//���涼error��,�߲���������
	}

	lua_pushstring(L, modName);//����ǲ���,ģ������,class.luaģ���п����õ���...[-0, +1, m]
	lua_call(L, 1, 0);//[-(nargs + 1), +nresults, e] class.lua���һ�в�û�з���_ENV

	//�����и�Լ��:���汻loadfile��call֮���ģ��Ҫ��package.loaded[modName]=mod
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");//
	lua_getfield(L, -1, modName);//
	return 1;
}

int craeteClass(lua_State *L) {
	auto baseCount = lua_gettop(L);
	if(0!= baseCount){//�и���
		for (auto i = 1; i <= baseCount; ++i) {
			if (!lua_istable(L, i)){//�����ǲ���һ����
				luaL_error(L, "base class should be a table ");
			}
		}
	}
	auto const modName = "class";//�Թ���,��ʱ��֧��ģ��֮���õ�ָ�
	auto const funName = "create";
	luaL_requiref(L, modName, openF, 0);//[-0, +1, e]
	lua_getfield(L, -1, funName);//[-0, +1, e]
	if (!lua_isfunction(L, -1)) {
		luaL_error(L, " ����%s�Ҳ���", funName);
	}
	//ѹ�ϲ���,����n������
	for (auto i = 1; i <= baseCount; ++i) {
		lua_pushvalue(L, i);
	}
	lua_call(L, baseCount, 1);//������
	if (!lua_istable(L, -1)) {//��鷵�ص��ǲ���һ����
		luaL_error(L, " class should be a table ");
	}
	return 1;
}

int ywlSetMetaTable(lua_State *L) {
	if (!lua_istable(L, 1) && !lua_isuserdata(L, 1) && !lua_isthread(L,1)) {
		luaL_argerror(L, 1, "ʵ������������table��userdata");
	}
	if (!lua_istable(L, 2)) {
		luaL_argerror(L, 2, "Ԫ�����������table");
	}
	lua_setmetatable(L, -2);//[-1, +0, -] ����ջ����metatable,����ջ��metatable��Ϊindex��ָ���value��metatable
	return 0;
}
//���ʵ��
int checkIsInstance(lua_State *L) {
	auto const INSTANCE = 1;
	auto const CLASS = 2;

	lua_getfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-0, +1, e]
	lua_getfield(L, -1, "isInstance");//[-0, +1, e]
	lua_pushvalue(L, INSTANCE);//ʵ��
	lua_pushvalue(L, CLASS);//��
	lua_call(L, 2, 1);
	if (!lua_isboolean(L, -1)) {//[-0, +0, v]
		luaL_error(L, "isInstance�ķ���ֵ����bool��");
	}
	if (!lua_toboolean(L, -1)) {//[-0, +0, v]
		luaL_error(L, "��ʵ�������ڴ��������");
	}
	return 0;
}
void checkInstance(lua_State *L,int objIdx,int clsIdx) {
	//��鴫������ʵ���Ƿ����������
	//---begin
	lua_pushcfunction(L, checkIsInstance);
	lua_pushvalue(L, objIdx);
	lua_pushvalue(L, clsIdx);
	lua_call(L, 2, 0);
	//---end
}

//����ǲ���class,���������쳣
int checkIsClass(lua_State *L) {
	auto const CLASS = 1;
	if (!lua_istable(L, CLASS)) {//[-0, +0, �C]
		auto iType = lua_type(L, CLASS);
		auto sName = luaL_typename(L, iType);
		luaL_error(L, "����һ����,��table������,��Ӧ����table,������%s", sName);
	}
	lua_pushstring(L, "__isClass__");
	lua_rawget(L, CLASS); //[-1, +1, �C] ���ⴥ��Ԫ����,�����һ��ʵ���ķ��ʵõ������������
	auto ret = lua_toboolean(L, -1);//[-0, +0, �C]
	luaL_argcheck(L, ret, CLASS, "����һ����");//[-0, +0, v]
	return 0;	
}

//�����a�ǲ�����b������,���������쳣
int checkIsSubClass(lua_State *L) {
	auto const parameterCount = lua_gettop(L);
	if (parameterCount != 2) {
		luaL_error(L, "�յ�%d������,����ֵ��2��", parameterCount);
	}
	//���2�������ǲ�����
	for (auto i = 1; i <= parameterCount; ++i) {
		lua_pushcfunction(L, checkIsClass);
		lua_pushvalue(L, i);
		lua_call(L, 1, 0);
	}
	//���ýű����isSubClass�жϴ���������ϲ��Ϸ�
	lua_getfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-0, +1, e]
	lua_getfield(L, -1, "isSubClass");//[-0, +1, e]
	lua_pushvalue(L, 1);//����
	lua_pushvalue(L, 2);//����
	lua_call(L, 2, 1);
	luaL_argcheck(L, lua_isboolean(L, -1), 1, "isSubClass�ķ���ֵ����bool��");//[-0, +0, v]
	luaL_argcheck(L, lua_toboolean(L, -1), 1, "��������,�෽�������಻��ȷ");
	
	return 0;
}


//[-0, +1, e]
bool createOrNewTable(lua_State *L, const void * key) {
	if (lua_rawgetp(L, LUA_REGISTRYINDEX, key) != LUA_TNIL)//[-0, +1, �C]
		return false;  /* leave previous value on top, but return 0 */
	lua_pop(L, 1);//����nil
	lua_createtable(L, 0, 2);//[-0, +1, e]
	lua_pushvalue(L, -1);//[-0, +1, e]
	lua_rawsetp(L, LUA_REGISTRYINDEX, key);//[-1, +0, e] ������ջ����value
	return true;
}