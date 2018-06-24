#include <lua.hpp>
#include <string>

using namespace std;
//压栈错误对象
void createErrorObj(lua_State *L, const string& sExceptTypeName, const string& sErrorMsg = "", int iCode = 0) {
	lua_getfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-0, +1, e]
	lua_getfield(L, -1, sExceptTypeName.c_str());//[-0, +1, e]
	if (luaL_getmetafield(L, -1, "__call") == LUA_TNIL)//[-0, +(0|1), e]  /* no metafield? */
		luaL_error(L, "%s没有元表或元表没有__call域", sExceptTypeName.c_str());
	lua_pushvalue(L, -2); //参数1,是cls
	lua_pushstring(L, sErrorMsg.c_str());//参数2
	lua_pushinteger(L, iCode);//参数3
	lua_call(L, 3, 1);
}
//----------------------------------------------------------------------------------------
#ifndef __STDC__
	#ifndef MS_WINDOWS
		extern char *strerror(int);
	#endif
#endif

#ifdef MS_WINDOWS
	#include "windows.h"
	#include "winbase.h"
#endif

#include <ctype.h>

/* Windows specific error code handling */

void PyErr_SetExcFromWindowsErrWithFilenameObject(lua_State* L,const string & exc, int ierr, int fileNameObjIdx){//	
	int len;
	char *s;
	char *s_buf = NULL; /* Free via LocalFree */
	char s_small_buf[28]; /* Room for "Windows Error 0xFFFFFFFF" */
	
	DWORD err = (DWORD)ierr;
	if (err == 0) err = GetLastError();
	len = FormatMessage(
		/* Error API error */
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,           /* no message source */
		err,
		MAKELANGID(LANG_NEUTRAL,
			SUBLANG_DEFAULT), /* Default language */
		(LPTSTR)&s_buf,
		0,              /* size not used */
		NULL);          /* no args */
	if (len == 0) {
		/* Only seen this in out of mem situations */
		sprintf(s_small_buf, "Windows Error 0x%X", err);
		s = s_small_buf;
		s_buf = NULL;
	}
	else {
		s = s_buf;
		/* remove trailing cr/lf and dots */
		while (len > 0 && (s[len - 1] <= ' ' || s[len - 1] == '.'))
			s[--len] = '\0';
	}
	/*
	if (fileNameObjIdx != 0) {
		//v = Py_BuildValue("(isO)", err, s, filenameObject);
		lua_createtable(L, 3, 0);

		lua_pushinteger(L, 1);//index1
		lua_pushinteger(L, err);//value1
		lua_settable(L, -3);//[-2, +0, e]

		lua_pushinteger(L, 2);//index2
		lua_pushstring(L, s);//value2
		lua_settable(L, -3);//[-2, +0, e]

		lua_pushinteger(L, 3);//index3
		lua_pushvalue(L, fileNameObjIdx);//value3
		lua_settable(L, -3);//[-2, +0, e]
	}
	else {
		//v = Py_BuildValue("(is)", err, s);
		lua_createtable(L, 2, 0);

		lua_pushinteger(L, 1);//index1
		lua_pushinteger(L, err);//value1
		lua_settable(L, -3);//[-2, +0, e]

		lua_pushinteger(L, 2);//index2
		lua_pushstring(L, s);//value2
		lua_settable(L, -3);//[-2, +0, e]
	}*/
	//if (v != NULL) {
		//PyErr_SetObject(exc, v);
		createErrorObj(L, exc, s, err);		
	//}
	LocalFree(s_buf);	
}

//压栈错误对象
void PyErr_SetExcFromWindowsErrWithFilename(lua_State* L, const string & exc, int ierr,	const char *filename){
	//PyObject *name = filename ? PyString_FromString(filename) : NULL;
	auto name = 0;
	PyErr_SetExcFromWindowsErrWithFilenameObject(L,exc, ierr, name);	
}

//压栈错误对象
void PyErr_SetExcFromWindowsErr(lua_State* L, const string & exc, int ierr)
{
	PyErr_SetExcFromWindowsErrWithFilename(L, exc, ierr, NULL);
}


//-------------------------------------

void PyErr_SetFromErrnoWithFilenameObject(lua_State* L, const string & exc){//, PyObject *filenameObject
	
	char *s;
	int i = errno;
#ifdef PLAN9
	char errbuf[ERRMAX];
#endif
#ifdef MS_WINDOWS
	char *s_buf = NULL;
	char s_small_buf[28]; /* Room for "Windows Error 0xFFFFFFFF" */
#endif
#ifdef EINTR
	//todo
	//if (i == EINTR && PyErr_CheckSignals())
	//	return NULL;
#endif
#ifdef PLAN9
	rerrstr(errbuf, sizeof errbuf);
	s = errbuf;
#else
	if (i == 0)
		s = "Error"; /* Sometimes errno didn't get set */
	else
#ifndef MS_WINDOWS
		s = strerror(i);
#else
	{
		/* Note that the Win32 errors do not lineup with the
		errno error.  So if the error is in the MSVC error
		table, we use it, otherwise we assume it really _is_
		a Win32 error code
		*/
		if (i > 0 && i < _sys_nerr) {
			s = _sys_errlist[i];
		}
		else {
			int len = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,                   /* no message source */
				i,
				MAKELANGID(LANG_NEUTRAL,
					SUBLANG_DEFAULT),
				/* Default language */
				(LPTSTR)&s_buf,
				0,                      /* size not used */
				NULL);                  /* no args */
			if (len == 0) {
				/* Only ever seen this in out-of-mem
				situations */
				sprintf(s_small_buf, "Windows Error 0x%X", i);
				s = s_small_buf;
				s_buf = NULL;
			}
			else {
				s = s_buf;
				/* remove trailing cr/lf and dots */
				while (len > 0 && (s[len - 1] <= ' ' || s[len - 1] == '.'))
					s[--len] = '\0';
			}
		}
	}
#endif /* Unix/Windows */
#endif /* PLAN 9*/
	//if (filenameObject != NULL)
	//	v = Py_BuildValue("(isO)", i, s, filenameObject);
	//else
	//	v = Py_BuildValue("(is)", i, s);
	//if (v != NULL) {
	//	PyErr_SetObject(exc, v);	
	//}
	createErrorObj(L, exc, s, i);
#ifdef MS_WINDOWS
	LocalFree(s_buf);
#endif
	
}

void PyErr_SetFromErrno(lua_State* L,const string &exc){
	PyErr_SetFromErrnoWithFilenameObject(L,exc);
}
