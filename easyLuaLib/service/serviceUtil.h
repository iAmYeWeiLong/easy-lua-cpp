#pragma once

template<typename T>
class Singleton;

template<typename keyType, typename valueType>
class Register;
//---------------------------
class cServiceInfo;

#include <string>
typedef Singleton<Register<std::string, cServiceInfo>> ServiceMgr;
//---------------------------
class cVmContext;
typedef Singleton<Register<int, cVmContext>> ContextMgr;

//---------------------------
#define MSG_TYPE_SYS 0
#define MSG_TYPE_LUA 1
#define MSG_TYPE_TEXT 2
