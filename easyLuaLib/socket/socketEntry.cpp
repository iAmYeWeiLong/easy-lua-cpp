// dllmain.cpp : Defines the entry point for the DLL application.
#include "../stdafx.h"

#include "../util.h"
#include "socketmodule.h"
#include <lua.hpp>
#include <string>
//#include <iostream>

using namespace std;

static luaL_Reg mod_func[] = {
	//{ "uv_version", uv_version_ },
	{ nullptr, nullptr }
};

int create_socket_class(lua_State *L);//声明
void createErrorObj(lua_State *, const string&, const string& sErrorMsg = "", int iCode = 0);//声明
int os_init(lua_State*);

//禁止名称改编,函数名必须是luaopen_库名
extern "C" _declspec(dllexport) int luaopen_easyLuaLib_socket(lua_State* L) {
	luaL_checkversion(L);
	if (lua_gettop(L) != 3) {
		luaL_error(L, "收到%d个参数,期望值是3个", lua_gettop(L));
	}
	auto modName = lua_tostring(L, 1);//模块名字
	auto filePath = lua_tostring(L, 2);//路径
	//printf("luaopen_libuv4lua 模块名字 =%s\n", modName);
	//printf("luaopen_libuv4lua 路径 =%s\n", filePath);
		
	auto const SCRIPT_FUNCTIONS_INDEX = 3;//脚本函数表的位置
		
	//--begin
	lua_pushvalue(L, SCRIPT_FUNCTIONS_INDEX);//复制脚本传来的
	lua_setfield(L, LUA_REGISTRYINDEX, "scriptFunctions");//[-1, +0, e]
	//---end
	
	luaL_newlib(L, mod_func);//[-0, +1, e]
	auto MODULE_INDEX = lua_gettop(L);

	os_init(L);
	//===begin	
	//lua_pushinteger(L, uv_run_mode::UV_RUN_DEFAULT);//[-0, +1, –]
	//lua_setfield(L, -2, "UV_RUN_DEFAULT");//[-1, +0, e]
	//
	//lua_pushinteger(L, uv_run_mode::UV_RUN_NOWAIT);//[-0, +1, –]
	//lua_setfield(L, -2, "UV_RUN_NOWAIT");//[-1, +0, e]
	//
	//lua_pushinteger(L, uv_run_mode::UV_RUN_ONCE);//[-0, +1, –]
	//lua_setfield(L, -2, "UV_RUN_ONCE");//[-1, +0, e]	
	//===end

	//创建cLoop类,并设为此lib的一个属性
	//---begin
	lua_pushcfunction(L, create_socket_class);//[-0, +1, –]	
	lua_call(L, 0, 1);//[-(nargs+1), +nresults, e]
	lua_setfield(L, -2, "cSocket");//把类设为目标模块的属性 [-1, +0, e]
	//---end
	
	lua_pushvalue(L, MODULE_INDEX);
	return 1;
}



/* Python interface to gethostname(). */

/*ARGSUSED*/
//static PyObject *
//socket_gethostname(PyObject *self, PyObject *unused)
//{
//    char buf[1024];
//    int res;
//    //,.Py_BEGIN_ALLOW_THREADS
//    res = gethostname(buf, (int) sizeof buf - 1);
//    //,.Py_END_ALLOW_THREADS
//    if (res < 0)
//        return set_error();
//    buf[sizeof buf - 1] = '\0';
//    return PyString_FromString(buf);
//}

/*PyDoc_STRVAR(gethostname_doc,
"gethostname() -> string\n\
\n\
Return the current host name.");*/


/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
// static PyObject *
// socket_gethostbyname(PyObject *self, PyObject *args)
// {
//     char *name;
//     sock_addr_t addrbuf;

//     if (!PyArg_ParseTuple(args, "s:gethostbyname", &name))
//         return NULL;
//     if (setipaddr(L,name, SAS2SA(&addrbuf),  sizeof(addrbuf), AF_INET) < 0)
//         return NULL;
//     return makeipaddr(SAS2SA(&addrbuf), sizeof(struct sockaddr_in));
// }

/*PyDoc_STRVAR(gethostbyname_doc,
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.");*/


/* Convenience function common to gethostbyname_ex and gethostbyaddr */

//static PyObject *
//gethost_common(struct hostent *h, struct sockaddr *addr, int alen, int af)
//{
//    char **pch;
//    PyObject *rtn_tuple = (PyObject *)NULL;
//    PyObject *name_list = (PyObject *)NULL;
//    PyObject *addr_list = (PyObject *)NULL;
//    PyObject *tmp;
//
//    if (h == NULL) {
//        /* Let's get real error message to return */
//#ifndef RISCOS
//        set_herror(h_errno);
//#else
//        PyErr_SetString(socket_error, "host not found");
//#endif
//        return NULL;
//    }
//
//    if (h->h_addrtype != af) {
//        /* Let's get real error message to return */
//        PyErr_SetString(socket_error,
//                        (char *)strerror(EAFNOSUPPORT));
//
//        return NULL;
//    }
//
//    switch (af) {
//
//    case AF_INET:
//        if (alen < sizeof(struct sockaddr_in))
//            return NULL;
//        break;
//
//#ifdef ENABLE_IPV6
//    case AF_INET6:
//        if (alen < sizeof(struct sockaddr_in6))
//            return NULL;
//        break;
//#endif
//
//    }
//
//    if ((name_list = PyList_New(0)) == NULL)
//        goto err;
//
//    if ((addr_list = PyList_New(0)) == NULL)
//        goto err;
//
//    /* SF #1511317: h_aliases can be NULL */
//    if (h->h_aliases) {
//        for (pch = h->h_aliases; *pch != NULL; pch++) {
//            int status;
//            tmp = PyString_FromString(*pch);
//            if (tmp == NULL)
//                goto err;
//
//            status = PyList_Append(name_list, tmp);
//            Py_DECREF(tmp);
//
//            if (status)
//                goto err;
//        }
//    }
//
//    for (pch = h->h_addr_list; *pch != NULL; pch++) {
//        int status;
//
//        switch (af) {
//
//        case AF_INET:
//            {
//            struct sockaddr_in sin;
//            memset(&sin, 0, sizeof(sin));
//            sin.sin_family = af;
//#ifdef HAVE_SOCKADDR_SA_LEN
//            sin.sin_len = sizeof(sin);
//#endif
//            memcpy(&sin.sin_addr, *pch, sizeof(sin.sin_addr));
//            tmp = makeipaddr((struct sockaddr *)&sin, sizeof(sin));
//
//            if (pch == h->h_addr_list && alen >= sizeof(sin))
//                memcpy((char *) addr, &sin, sizeof(sin));
//            break;
//            }
//
//#ifdef ENABLE_IPV6
//        case AF_INET6:
//            {
//            struct sockaddr_in6 sin6;
//            memset(&sin6, 0, sizeof(sin6));
//            sin6.sin6_family = af;
//#ifdef HAVE_SOCKADDR_SA_LEN
//            sin6.sin6_len = sizeof(sin6);
//#endif
//            memcpy(&sin6.sin6_addr, *pch, sizeof(sin6.sin6_addr));
//            tmp = makeipaddr((struct sockaddr *)&sin6,
//                sizeof(sin6));
//
//            if (pch == h->h_addr_list && alen >= sizeof(sin6))
//                memcpy((char *) addr, &sin6, sizeof(sin6));
//            break;
//            }
//#endif
//
//        default:                /* can't happen */
//            PyErr_SetString(socket_error,
//                            "unsupported address family");
//            return NULL;
//        }
//
//        if (tmp == NULL)
//            goto err;
//
//        status = PyList_Append(addr_list, tmp);
//        Py_DECREF(tmp);
//
//        if (status)
//            goto err;
//    }
//
//    rtn_tuple = Py_BuildValue("sOO", h->h_name, name_list, addr_list);
//
// err:
//    Py_XDECREF(name_list);
//    Py_XDECREF(addr_list);
//    return rtn_tuple;
//}


/* Python interface to gethostbyname_ex(name). */

/*ARGSUSED*/
// static PyObject *
// socket_gethostbyname_ex(PyObject *self, PyObject *args)
// {
//     char *name;
//     struct hostent *h;
// #ifdef ENABLE_IPV6
//     struct sockaddr_storage addr;
// #else
//     struct sockaddr_in addr;
// #endif
//     struct sockaddr *sa;
//     PyObject *ret;
// #ifdef HAVE_GETHOSTBYNAME_R
//     struct hostent hp_allocated;
// #ifdef HAVE_GETHOSTBYNAME_R_3_ARG
//     struct hostent_data data;
// #else
//     char buf[16384];
//     int buf_len = (sizeof buf) - 1;
//     int errnop;
// #endif
// #ifdef HAVE_GETHOSTBYNAME_R_3_ARG
//     int result;
// #endif
// #endif /* HAVE_GETHOSTBYNAME_R */

//     if (!PyArg_ParseTuple(args, "s:gethostbyname_ex", &name))
//         return NULL;
//     if (setipaddr(L,name, (struct sockaddr *)&addr, sizeof(addr), AF_INET) < 0)
//         return NULL;
//     //,.Py_BEGIN_ALLOW_THREADS
// #ifdef HAVE_GETHOSTBYNAME_R
// #if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
//     gethostbyname_r(name, &hp_allocated, buf, buf_len,
//                              &h, &errnop);
// #elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
//     h = gethostbyname_r(name, &hp_allocated, buf, buf_len, &errnop);
// #else /* HAVE_GETHOSTBYNAME_R_3_ARG */
//     memset((void *) &data, '\0', sizeof(data));
//     result = gethostbyname_r(name, &hp_allocated, &data);
//     h = (result != 0) ? NULL : &hp_allocated;
// #endif
// #else /* not HAVE_GETHOSTBYNAME_R */
// #ifdef USE_GETHOSTBYNAME_LOCK
//     PyThread_acquire_lock(netdb_lock, 1);
// #endif
//     h = gethostbyname(name);
// #endif /* HAVE_GETHOSTBYNAME_R */
//     //,.Py_END_ALLOW_THREADS
//     /* Some C libraries would require addr.__ss_family instead of
//        addr.ss_family.
//        Therefore, we cast the sockaddr_storage into sockaddr to
//        access sa_family. */
//     sa = (struct sockaddr*)&addr;
//     ret = gethost_common(h, (struct sockaddr *)&addr, sizeof(addr),
//                          sa->sa_family);
// #ifdef USE_GETHOSTBYNAME_LOCK
//     PyThread_release_lock(netdb_lock);
// #endif
//     return ret;
// }

/*PyDoc_STRVAR(ghbn_ex_doc,
"gethostbyname_ex(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");*/


/* Python interface to gethostbyaddr(IP). */

/*ARGSUSED*/
// static PyObject *
// socket_gethostbyaddr(PyObject *self, PyObject *args)
// {
// #ifdef ENABLE_IPV6
//     struct sockaddr_storage addr;
// #else
//     struct sockaddr_in addr;
// #endif
//     struct sockaddr *sa = (struct sockaddr *)&addr;
//     char *ip_num;
//     struct hostent *h;
//     PyObject *ret;
// #ifdef HAVE_GETHOSTBYNAME_R
//     struct hostent hp_allocated;
// #ifdef HAVE_GETHOSTBYNAME_R_3_ARG
//     struct hostent_data data;
// #else
//     /* glibcs up to 2.10 assume that the buf argument to
//        gethostbyaddr_r is 8-byte aligned, which at least llvm-gcc
//        does not ensure. The attribute below instructs the compiler
//        to maintain this alignment. */
//     char buf[16384] Py_ALIGNED(8);
//     int buf_len = (sizeof buf) - 1;
//     int errnop;
// #endif
// #ifdef HAVE_GETHOSTBYNAME_R_3_ARG
//     int result;
// #endif
// #endif /* HAVE_GETHOSTBYNAME_R */
//     char *ap;
//     int al;
//     int af;

//     if (!PyArg_ParseTuple(args, "s:gethostbyaddr", &ip_num))
//         return NULL;
//     af = AF_UNSPEC;
//     if (setipaddr(L,ip_num, sa, sizeof(addr), af) < 0)
//         return NULL;
//     af = sa->sa_family;
//     ap = NULL;
//     switch (af) {
//     case AF_INET:
//         ap = (char *)&((struct sockaddr_in *)sa)->sin_addr;
//         al = sizeof(((struct sockaddr_in *)sa)->sin_addr);
//         break;
// #ifdef ENABLE_IPV6
//     case AF_INET6:
//         ap = (char *)&((struct sockaddr_in6 *)sa)->sin6_addr;
//         al = sizeof(((struct sockaddr_in6 *)sa)->sin6_addr);
//         break;
// #endif
//     default:
//         PyErr_SetString(socket_error, "unsupported address family");
//         return NULL;
//     }
//     //,.Py_BEGIN_ALLOW_THREADS
// #ifdef HAVE_GETHOSTBYNAME_R
// #if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
//     gethostbyaddr_r(ap, al, af,
//         &hp_allocated, buf, buf_len,
//         &h, &errnop);
// #elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
//     h = gethostbyaddr_r(ap, al, af,
//                         &hp_allocated, buf, buf_len, &errnop);
// #else /* HAVE_GETHOSTBYNAME_R_3_ARG */
//     memset((void *) &data, '\0', sizeof(data));
//     result = gethostbyaddr_r(ap, al, af, &hp_allocated, &data);
//     h = (result != 0) ? NULL : &hp_allocated;
// #endif
// #else /* not HAVE_GETHOSTBYNAME_R */
// #ifdef USE_GETHOSTBYNAME_LOCK
//     PyThread_acquire_lock(netdb_lock, 1);
// #endif
//     h = gethostbyaddr(ap, al, af);
// #endif /* HAVE_GETHOSTBYNAME_R */
//     //,.Py_END_ALLOW_THREADS
//     ret = gethost_common(h, (struct sockaddr *)&addr, sizeof(addr), af);
// #ifdef USE_GETHOSTBYNAME_LOCK
//     PyThread_release_lock(netdb_lock);
// #endif
//     return ret;
// }

/*PyDoc_STRVAR(gethostbyaddr_doc,
"gethostbyaddr(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");*/


/* Python interface to getservbyname(name).
This only returns the port number, since the other info is already
known or not useful (like the list of aliases). */

/*ARGSUSED*/
// static PyObject *
// socket_getservbyname(PyObject *self, PyObject *args)
// {
//     char *name, *proto=NULL;
//     struct servent *sp;
//     if (!PyArg_ParseTuple(args, "s|s:getservbyname", &name, &proto))
//         return NULL;
//     //,.Py_BEGIN_ALLOW_THREADS
//     sp = getservbyname(name, proto);
//     //,.Py_END_ALLOW_THREADS
//     if (sp == NULL) {
//         PyErr_SetString(socket_error, "service/proto not found");
//         return NULL;
//     }
//     return PyInt_FromLong((long) ntohs(sp->s_port));
// }

/*PyDoc_STRVAR(getservbyname_doc,
"getservbyname(servicename[, protocolname]) -> integer\n\
\n\
Return a port number from a service name and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");*/


/* Python interface to getservbyport(port).
This only returns the service name, since the other info is already
known or not useful (like the list of aliases). */

/*ARGSUSED*/
// static PyObject *
// socket_getservbyport(PyObject *self, PyObject *args)
// {
//     int port;
//     char *proto=NULL;
//     struct servent *sp;
//     if (!PyArg_ParseTuple(args, "i|s:getservbyport", &port, &proto))
//         return NULL;
//     if (port < 0 || port > 0xffff) {
//         PyErr_SetString(
//             PyExc_OverflowError,
//             "getservbyport: port must be 0-65535.");
//         return NULL;
//     }
//     //,.Py_BEGIN_ALLOW_THREADS
//     sp = getservbyport(htons((short)port), proto);
//     //,.Py_END_ALLOW_THREADS
//     if (sp == NULL) {
//         PyErr_SetString(socket_error, "port/proto not found");
//         return NULL;
//     }
//     return PyString_FromString(sp->s_name);
// }

/*PyDoc_STRVAR(getservbyport_doc,
"getservbyport(port[, protocolname]) -> string\n\
\n\
Return the service name from a port number and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");*/

/* Python interface to getprotobyname(name).
This only returns the protocol number, since the other info is
already known or not useful (like the list of aliases). */

/*ARGSUSED*/
// static PyObject *
// socket_getprotobyname(PyObject *self, PyObject *args)
// {
//     char *name;
//     struct protoent *sp;
// #ifdef __BEOS__
// /* Not available in BeOS yet. - [cjh] */
//     PyErr_SetString(socket_error, "getprotobyname not supported");
//     return NULL;
// #else
//     if (!PyArg_ParseTuple(args, "s:getprotobyname", &name))
//         return NULL;
//     //,.Py_BEGIN_ALLOW_THREADS
//     sp = getprotobyname(name);
//     //,.Py_END_ALLOW_THREADS
//     if (sp == NULL) {
//         PyErr_SetString(socket_error, "protocol not found");
//         return NULL;
//     }
//     return PyInt_FromLong((long) sp->p_proto);
// #endif
// }

/*PyDoc_STRVAR(getprotobyname_doc,
"getprotobyname(name) -> integer\n\
\n\
Return the protocol number for the named protocol.  (Rarely used.)");*/


#ifdef HAVE_SOCKETPAIR
/* Create a pair of sockets using the socketpair() function.
Arguments as for socket() except the default family is AF_UNIX if
defined on the platform; otherwise, the default is AF_INET. */

/*ARGSUSED*/
// static PyObject *
// socket_socketpair(PyObject *self, PyObject *args)
// {
//     PySocketSockObject *s0 = NULL, *s1 = NULL;
//     SOCKET_T sv[2];
//     int family, type = SOCK_STREAM, proto = 0;
//     PyObject *res = NULL;

// #if defined(AF_UNIX)
//     family = AF_UNIX;
// #else
//     family = AF_INET;
// #endif
//     if (!PyArg_ParseTuple(args, "|iii:socketpair",
//                           &family, &type, &proto))
//         return NULL;
//     /* Create a pair of socket fds */
//     if (socketpair(family, type, proto, sv) < 0)
//         return set_error();
//     s0 = new_sockobject(sv[0], family, type, proto);
//     if (s0 == NULL)
//         goto finally;
//     s1 = new_sockobject(sv[1], family, type, proto);
//     if (s1 == NULL)
//         goto finally;
//     res = PyTuple_Pack(2, s0, s1);

// finally:
//     if (res == NULL) {
//         if (s0 == NULL)
//             SOCKETCLOSE(sv[0]);
//         if (s1 == NULL)
//             SOCKETCLOSE(sv[1]);
//     }
//     Py_XDECREF(s0);
//     Py_XDECREF(s1);
//     return res;
// }

/*PyDoc_STRVAR(socketpair_doc,
"socketpair([family[, type[, proto]]]) -> (socket object, socket object)\n\
\n\
Create a pair of socket objects from the sockets returned by the platform\n\
socketpair() function.\n\
The arguments are the same as for socket() except the default family is\n\
AF_UNIX if defined on the platform; otherwise, the default is AF_INET.");*/

#endif /* HAVE_SOCKETPAIR */


#ifndef NO_DUP
/* Create a socket object from a numeric file description.
Useful e.g. if stdin is a socket.
Additional arguments as for socket(). */

/*ARGSUSED*/
// static PyObject *
// socket_fromfd(PyObject *self, PyObject *args)
// {
//     PySocketSockObject *s;
//     SOCKET_T fd;
//     int family, type, proto = 0;
//     if (!PyArg_ParseTuple(args, "iii|i:fromfd",
//                           &fd, &family, &type, &proto))
//         return NULL;
//     /* Dup the fd so it and the socket can be closed independently */
//     fd = dup(fd);
//     if (fd < 0)
//         return set_error();
//     s = new_sockobject(fd, family, type, proto);
//     return (PyObject *) s;
// }

/*PyDoc_STRVAR(fromfd_doc,
"fromfd(fd, family, type[, proto]) -> socket object\n\
\n\
Create a socket object from a duplicate of the given\n\
file descriptor.\n\
The remaining arguments are the same as for socket().");*/

#endif /* NO_DUP */

/*static PyObject *
socket_ntohs(PyObject *self, PyObject *args)
{
int x1, x2;

if (!PyArg_ParseTuple(args, "i:ntohs", &x1)) {
return NULL;
}
if (x1 < 0) {
PyErr_SetString(PyExc_OverflowError,
"can't convert negative number to unsigned long");
return NULL;
}
x2 = (unsigned int)ntohs((unsigned short)x1);
return PyInt_FromLong(x2);
}*/

/*PyDoc_STRVAR(ntohs_doc,
"ntohs(integer) -> integer\n\
\n\
Convert a 16-bit integer from network to host byte order.");*/


// static PyObject *
// socket_ntohl(PyObject *self, PyObject *arg)
// {
//     unsigned long x;

//     if (PyInt_Check(arg)) {
//         x = PyInt_AS_LONG(arg);
//         if (x == (unsigned long) -1 && PyErr_Occurred())
//             return NULL;
//         if ((long)x < 0) {
//             PyErr_SetString(PyExc_OverflowError,
//               "can't convert negative number to unsigned long");
//             return NULL;
//         }
//     }
//     else if (PyLong_Check(arg)) {
//         x = PyLong_AsUnsignedLong(arg);
//         if (x == (unsigned long) -1 && PyErr_Occurred())
//             return NULL;
// #if SIZEOF_LONG > 4
//         {
//             unsigned long y;
//             /* only want the trailing 32 bits */
//             y = x & 0xFFFFFFFFUL;
//             if (y ^ x)
//                 return PyErr_Format(PyExc_OverflowError,
//                             "long int larger than 32 bits");
//             x = y;
//         }
// #endif
//     }
//     else
//         return PyErr_Format(PyExc_TypeError,
//                             "expected int/long, %s found",
//                             Py_TYPE(arg)->tp_name);
//     if (x == (unsigned long) -1 && PyErr_Occurred())
//         return NULL;
//     return PyLong_FromUnsignedLong(ntohl(x));
// }

/*PyDoc_STRVAR(ntohl_doc,
"ntohl(integer) -> integer\n\
\n\
Convert a 32-bit integer from network to host byte order.");*/


// static PyObject *
// socket_htons(PyObject *self, PyObject *args)
// {
//     int x1, x2;

//     if (!PyArg_ParseTuple(args, "i:htons", &x1)) {
//         return NULL;
//     }
//     if (x1 < 0) {
//         PyErr_SetString(PyExc_OverflowError,
//             "can't convert negative number to unsigned long");
//         return NULL;
//     }
//     x2 = (unsigned int)htons((unsigned short)x1);
//     return PyInt_FromLong(x2);
// }

/*PyDoc_STRVAR(htons_doc,
"htons(integer) -> integer\n\
\n\
Convert a 16-bit integer from host to network byte order.");*/


// static PyObject *
// socket_htonl(PyObject *self, PyObject *arg)
// {
//     unsigned long x;

//     if (PyInt_Check(arg)) {
//         x = PyInt_AS_LONG(arg);
//         if (x == (unsigned long) -1 && PyErr_Occurred())
//             return NULL;
//         if ((long)x < 0) {
//             PyErr_SetString(PyExc_OverflowError,
//               "can't convert negative number to unsigned long");
//             return NULL;
//         }
//     }
//     else if (PyLong_Check(arg)) {
//         x = PyLong_AsUnsignedLong(arg);
//         if (x == (unsigned long) -1 && PyErr_Occurred())
//             return NULL;
// #if SIZEOF_LONG > 4
//         {
//             unsigned long y;
//             /* only want the trailing 32 bits */
//             y = x & 0xFFFFFFFFUL;
//             if (y ^ x)
//                 return PyErr_Format(PyExc_OverflowError,
//                             "long int larger than 32 bits");
//             x = y;
//         }
// #endif
//     }
//     else
//         return PyErr_Format(PyExc_TypeError,
//                             "expected int/long, %s found",
//                             Py_TYPE(arg)->tp_name);
//     return PyLong_FromUnsignedLong(htonl((unsigned long)x));
// }

/*PyDoc_STRVAR(htonl_doc,
"htonl(integer) -> integer\n\
\n\
Convert a 32-bit integer from host to network byte order.");*/

/* socket.inet_aton() and socket.inet_ntoa() functions. */

/*PyDoc_STRVAR(inet_aton_doc,
"inet_aton(string) -> packed 32-bit IP representation\n\
\n\
Convert an IP address in string format (123.45.67.89) to the 32-bit packed\n\
binary format used in low-level network functions.");*/

// static PyObject*
// socket_inet_aton(PyObject *self, PyObject *args)
// {
// #ifndef INADDR_NONE
// #define INADDR_NONE (-1)
// #endif
// #ifdef HAVE_INET_ATON
//     struct in_addr buf;
// #endif

// #if !defined(HAVE_INET_ATON) || defined(USE_INET_ATON_WEAKLINK)
// #if (SIZEOF_INT != 4)
// #error "Not sure if in_addr_t exists and int is not 32-bits."
// #endif
//     /* Have to use inet_addr() instead */
//     unsigned int packed_addr;
// #endif
//     char *ip_addr;

//     if (!PyArg_ParseTuple(args, "s:inet_aton", &ip_addr))
//         return NULL;


// #ifdef HAVE_INET_ATON

// #ifdef USE_INET_ATON_WEAKLINK
//     if (inet_aton != NULL) {
// #endif
//     if (inet_aton(ip_addr, &buf))
//         return PyString_FromStringAndSize((char *)(&buf),
//                                           sizeof(buf));

//     PyErr_SetString(socket_error,
//                     "illegal IP address string passed to inet_aton");
//     return NULL;

// #ifdef USE_INET_ATON_WEAKLINK
//    } else {
// #endif

// #endif

// #if !defined(HAVE_INET_ATON) || defined(USE_INET_ATON_WEAKLINK)

//     /* special-case this address as inet_addr might return INADDR_NONE
//      * for this */
//     if (strcmp(ip_addr, "255.255.255.255") == 0) {
//         packed_addr = 0xFFFFFFFF;
//     } else {

//         packed_addr = inet_addr(ip_addr);

//         if (packed_addr == INADDR_NONE) {               /* invalid address */
//             PyErr_SetString(socket_error,
//                 "illegal IP address string passed to inet_aton");
//             return NULL;
//         }
//     }
//     return PyString_FromStringAndSize((char *) &packed_addr,
//                                       sizeof(packed_addr));

// #ifdef USE_INET_ATON_WEAKLINK
//    }
// #endif

// #endif
// }

/*PyDoc_STRVAR(inet_ntoa_doc,
"inet_ntoa(packed_ip) -> ip_address_string\n\
\n\
Convert an IP address from 32-bit packed binary format to string format");*/

/*static PyObject*
socket_inet_ntoa(PyObject *self, PyObject *args)
{
char *packed_str;
int addr_len;
struct in_addr packed_addr;

if (!PyArg_ParseTuple(args, "s#:inet_ntoa", &packed_str, &addr_len)) {
return NULL;
}

if (addr_len != sizeof(packed_addr)) {
PyErr_SetString(socket_error,
"packed IP wrong length for inet_ntoa");
return NULL;
}

memcpy(&packed_addr, packed_str, addr_len);

return PyString_FromString(inet_ntoa(packed_addr));
}*/

#ifdef HAVE_INET_PTON

/*PyDoc_STRVAR(inet_pton_doc,
"inet_pton(af, ip) -> packed IP address string\n\
\n\
Convert an IP address from string format to a packed string suitable\n\
for use with low-level network functions.");*/
/*
static PyObject *
socket_inet_pton(PyObject *self, PyObject *args)
{
int af;
char* ip;
int retval;
#ifdef ENABLE_IPV6
char packed[MAX(sizeof(struct in_addr), sizeof(struct in6_addr))];
#else
char packed[sizeof(struct in_addr)];
#endif
if (!PyArg_ParseTuple(args, "is:inet_pton", &af, &ip)) {
return NULL;
}

#if !defined(ENABLE_IPV6) && defined(AF_INET6)
if(af == AF_INET6) {
PyErr_SetString(socket_error,
"can't use AF_INET6, IPv6 is disabled");
return NULL;
}
#endif

retval = inet_pton(af, ip, packed);
if (retval < 0) {
PyErr_SetFromErrno(socket_error);
return NULL;
} else if (retval == 0) {
PyErr_SetString(socket_error,
"illegal IP address string passed to inet_pton");
return NULL;
} else if (af == AF_INET) {
return PyString_FromStringAndSize(packed,
sizeof(struct in_addr));
#ifdef ENABLE_IPV6
} else if (af == AF_INET6) {
return PyString_FromStringAndSize(packed,
sizeof(struct in6_addr));
#endif
} else {
PyErr_SetString(socket_error, "unknown address family");
return NULL;
}
}*/

/*PyDoc_STRVAR(inet_ntop_doc,
"inet_ntop(af, packed_ip) -> string formatted IP address\n\
\n\
Convert a packed IP address of the given family to string format.");*/
/*
static PyObject *
socket_inet_ntop(PyObject *self, PyObject *args)
{
int af;
char* packed;
int len;
const char* retval;
#ifdef ENABLE_IPV6
char ip[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1];
#else
char ip[INET_ADDRSTRLEN + 1];
#endif

// Guarantee NUL-termination for PyString_FromString() below
memset((void *) &ip[0], '\0', sizeof(ip));

if (!PyArg_ParseTuple(args, "is#:inet_ntop", &af, &packed, &len)) {
return NULL;
}

if (af == AF_INET) {
if (len != sizeof(struct in_addr)) {
PyErr_SetString(PyExc_ValueError,
"invalid length of packed IP address string");
return NULL;
}
#ifdef ENABLE_IPV6
} else if (af == AF_INET6) {
if (len != sizeof(struct in6_addr)) {
PyErr_SetString(PyExc_ValueError,
"invalid length of packed IP address string");
return NULL;
}
#endif
} else {
PyErr_Format(PyExc_ValueError,
"unknown address family %d", af);
return NULL;
}

retval = inet_ntop(af, packed, ip, sizeof(ip));
if (!retval) {
PyErr_SetFromErrno(socket_error);
return NULL;
} else {
return PyString_FromString(retval);
}

// NOTREACHED
PyErr_SetString(PyExc_RuntimeError, "invalid handling of inet_ntop");
return NULL;
}*/

#endif /* HAVE_INET_PTON */

/* Python interface to getaddrinfo(host, port). */

/*ARGSUSED*/
/*static PyObject *
socket_getaddrinfo(PyObject *self, PyObject *args)
{
struct addrinfo hints, *res;
struct addrinfo *res0 = NULL;
PyObject *hobj = NULL;
PyObject *pobj = (PyObject *)NULL;
char pbuf[30];
char *hptr, *pptr;
int family, socktype, protocol, flags;
int error;
PyObject *all = (PyObject *)NULL;
PyObject *single = (PyObject *)NULL;
PyObject *idna = NULL;

family = socktype = protocol = flags = 0;
family = AF_UNSPEC;
if (!PyArg_ParseTuple(args, "OO|iiii:getaddrinfo",
&hobj, &pobj, &family, &socktype,
&protocol, &flags)) {
return NULL;
}
if (hobj == Py_None) {
hptr = NULL;
} else if (PyUnicode_Check(hobj)) {
idna = PyUnicode_AsEncodedString(hobj, "idna", NULL);
if (!idna)
return NULL;
hptr = PyString_AsString(idna);
} else if (PyString_Check(hobj)) {
hptr = PyString_AsString(hobj);
} else {
PyErr_SetString(PyExc_TypeError,
"getaddrinfo() argument 1 must be string or None");
return NULL;
}
if (PyInt_Check(pobj) || PyLong_Check(pobj)) {
long value = PyLong_AsLong(pobj);
if (value == -1 && PyErr_Occurred())
return NULL;
PyOS_snprintf(pbuf, sizeof(pbuf), "%ld", value);
pptr = pbuf;
} else if (PyString_Check(pobj)) {
pptr = PyString_AsString(pobj);
} else if (pobj == Py_None) {
pptr = (char *)NULL;
} else {
PyErr_SetString(socket_error,
"getaddrinfo() argument 2 must be integer or string");
goto err;
}
#if defined(__APPLE__) && defined(AI_NUMERICSERV)
if ((flags & AI_NUMERICSERV) && (pptr == NULL || (pptr[0] == '0' && pptr[1] == 0))) {
// On OSX upto at least OSX 10.8 getaddrinfo crashes
// if AI_NUMERICSERV is set and the servname is NULL or "0".
// This workaround avoids a segfault in libsystem.
//
pptr = "00";
}
#endif
memset(&hints, 0, sizeof(hints));
hints.ai_family = family;
hints.ai_socktype = socktype;
hints.ai_protocol = protocol;
hints.ai_flags = flags;
//,.Py_BEGIN_ALLOW_THREADS
ACQUIRE_GETADDRINFO_LOCK
error = getaddrinfo(hptr, pptr, &hints, &res0);
//,.Py_END_ALLOW_THREADS
RELEASE_GETADDRINFO_LOCK  // see comment in setipaddr()
if (error) {
set_gaierror(error);
goto err;
}

all = PyList_New(0);
if (all == NULL)
goto err;
for (res = res0; res; res = res->ai_next) {
PyObject *addr =
makesockaddr(-1, res->ai_addr, res->ai_addrlen, protocol);
if (addr == NULL)
goto err;
single = Py_BuildValue("iiisO", res->ai_family,
res->ai_socktype, res->ai_protocol,
res->ai_canonname ? res->ai_canonname : "",
addr);
Py_DECREF(addr);
if (single == NULL)
goto err;

if (PyList_Append(all, single))
goto err;
Py_XDECREF(single);
}
Py_XDECREF(idna);
if (res0)
freeaddrinfo(res0);
return all;
err:
Py_XDECREF(single);
Py_XDECREF(all);
Py_XDECREF(idna);
if (res0)
freeaddrinfo(res0);
return (PyObject *)NULL;
}*/

/*PyDoc_STRVAR(getaddrinfo_doc,
"getaddrinfo(host, port [, family, socktype, proto, flags])\n\
-> list of (family, socktype, proto, canonname, sockaddr)\n\
\n\
Resolve host and port into addrinfo struct.");*/

/* Python interface to getnameinfo(sa, flags). */

/*ARGSUSED*/
/*static PyObject *
socket_getnameinfo(PyObject *self, PyObject *args)
{
PyObject *sa = (PyObject *)NULL;
int flags;
char *hostp;
int port;
unsigned int flowinfo, scope_id;
char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
struct addrinfo hints, *res = NULL;
int error;
PyObject *ret = (PyObject *)NULL;

flags = flowinfo = scope_id = 0;
if (!PyArg_ParseTuple(args, "Oi:getnameinfo", &sa, &flags))
return NULL;
if (!PyTuple_Check(sa)) {
PyErr_SetString(PyExc_TypeError,
"getnameinfo() argument 1 must be a tuple");
return NULL;
}
if (!PyArg_ParseTuple(sa, "si|II",
&hostp, &port, &flowinfo, &scope_id))
return NULL;
if (flowinfo > 0xfffff) {
PyErr_SetString(PyExc_OverflowError,
"getsockaddrarg: flowinfo must be 0-1048575.");
return NULL;
}
PyOS_snprintf(pbuf, sizeof(pbuf), "%d", port);
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_DGRAM;     // make numeric port happy
//,.Py_BEGIN_ALLOW_THREADS
ACQUIRE_GETADDRINFO_LOCK
error = getaddrinfo(hostp, pbuf, &hints, &res);
//,.Py_END_ALLOW_THREADS
RELEASE_GETADDRINFO_LOCK  // see comment in setipaddr()
if (error) {
set_gaierror(error);
goto fail;
}
if (res->ai_next) {
PyErr_SetString(socket_error,
"sockaddr resolved to multiple addresses");
goto fail;
}
switch (res->ai_family) {
case AF_INET:
{
if (PyTuple_GET_SIZE(sa) != 2) {
PyErr_SetString(socket_error,
"IPv4 sockaddr must be 2 tuple");
goto fail;
}
break;
}
#ifdef ENABLE_IPV6
case AF_INET6:
{
struct sockaddr_in6 *sin6;
sin6 = (struct sockaddr_in6 *)res->ai_addr;
sin6->sin6_flowinfo = htonl(flowinfo);
sin6->sin6_scope_id = scope_id;
break;
}
#endif
}
error = getnameinfo(res->ai_addr, res->ai_addrlen,
hbuf, sizeof(hbuf), pbuf, sizeof(pbuf), flags);
if (error) {
set_gaierror(error);
goto fail;
}
ret = Py_BuildValue("ss", hbuf, pbuf);

fail:
if (res)
freeaddrinfo(res);
return ret;
}*/

/*PyDoc_STRVAR(getnameinfo_doc,
"getnameinfo(sockaddr, flags) --> (host, port)\n\
\n\
Get host and port for a sockaddr.");*/


/* Python API to getting and setting the default timeout value. */

// static PyObject *
// socket_getdefaulttimeout(PyObject *self)
// {
//     if (defaulttimeout < 0.0) {
//         Py_INCREF(Py_None);
//         return Py_None;
//     }
//     else
//         return PyFloat_FromDouble(defaulttimeout);
// }

/*PyDoc_STRVAR(getdefaulttimeout_doc,
"getdefaulttimeout() -> timeout\n\
\n\
Returns the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");*/

// static PyObject *
// socket_setdefaulttimeout(PyObject *self, PyObject *arg)
// {
//     double timeout;

//     if (arg == Py_None)
//         timeout = -1.0;
//     else {
//         timeout = PyFloat_AsDouble(arg);
//         if (timeout < 0.0) {
//             if (!PyErr_Occurred())
//                 PyErr_SetString(PyExc_ValueError,
//                                 "Timeout value out of range");
//             return NULL;
//         }
//     }

//     defaulttimeout = timeout;

//     Py_INCREF(Py_None);
//     return Py_None;
// }

/*PyDoc_STRVAR(setdefaulttimeout_doc,
"setdefaulttimeout(timeout)\n\
\n\
Set the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");*/


/* List of functions exported by this module. */
/*
static PyMethodDef socket_methods[] = {
//{"gethostbyname",           socket_gethostbyname,METH_VARARGS, gethostbyname_doc},
//{"gethostbyname_ex",        socket_gethostbyname_ex, METH_VARARGS, ghbn_ex_doc},
// {"gethostbyaddr",           socket_gethostbyaddr,METH_VARARGS, gethostbyaddr_doc},
// {"gethostname",             socket_gethostname,METH_NOARGS,  gethostname_doc},
// {"getservbyname",           socket_getservbyname,METH_VARARGS, getservbyname_doc},
//{"getservbyport",           socket_getservbyport,METH_VARARGS, getservbyport_doc},
//{"getprotobyname",          socket_getprotobyname,METH_VARARGS, getprotobyname_doc},
#ifndef NO_DUP
// {"fromfd",                  socket_fromfd, METH_VARARGS, fromfd_doc},
#endif
#ifdef HAVE_SOCKETPAIR
// {"socketpair",              socket_socketpair,METH_VARARGS, socketpair_doc},
#endif
//{"ntohs",                   socket_ntohs,METH_VARARGS, ntohs_doc},
// {"ntohl",                   socket_ntohl,METH_O, ntohl_doc},
// {"htons",                   socket_htons,METH_VARARGS, htons_doc},
// {"htonl",                   socket_htonl,METH_O, htonl_doc},
// {"inet_aton",               socket_inet_aton,METH_VARARGS, inet_aton_doc},
//{"inet_ntoa",               socket_inet_ntoa,METH_VARARGS, inet_ntoa_doc},
#ifdef HAVE_INET_PTON
//{"inet_pton",               socket_inet_pton,METH_VARARGS, inet_pton_doc},
//{"inet_ntop",               socket_inet_ntop,METH_VARARGS, inet_ntop_doc},
#endif
//{"getaddrinfo",             socket_getaddrinfo,METH_VARARGS, getaddrinfo_doc},
//{"getnameinfo",             socket_getnameinfo,METH_VARARGS, getnameinfo_doc},
//{"getdefaulttimeout",       (PyCFunction)socket_getdefaulttimeout,METH_NOARGS, getdefaulttimeout_doc},
// {"setdefaulttimeout",       socket_setdefaulttimeout,METH_O, setdefaulttimeout_doc},
{NULL,                      NULL}            // Sentinel
};*/


//#ifdef RISCOS
//	#define OS_INIT_DEFINED
//
//	static int
//	os_init(void)
//	{
//		_kernel_swi_regs r;
//
//		r.r[0] = 0;
//		_kernel_swi(0x43380, &r, &r);
//		taskwindow = r.r[0];
//
//		return 1;
//	}
//
//#endif /* RISCOS */


#ifdef MS_WINDOWS
	#define OS_INIT_DEFINED

	// Additional initialization and cleanup for Windows 

	static void
	os_cleanup(void)
	{
		WSACleanup();
	}

	static int
	os_init(lua_State* L)//void
	{
		WSADATA WSAData;
		int ret;
		char buf[100];
		ret = WSAStartup(0x0101, &WSAData);
		switch (ret) {
		case 0:     // No error 
			//Py_AtExit(os_cleanup); //lua没有提供相应的回调机制
			return 1; // Success 
		case WSASYSNOTREADY:			
			luaL_error(L, "WSAStartup failed: network not ready");//PyErr_SetString(PyExc_ImportError,"WSAStartup failed: network not ready");
			break;
		case WSAVERNOTSUPPORTED:
		case WSAEINVAL:			
			luaL_error(L, "WSAStartup failed: requested version not supported");//PyErr_SetString(PyExc_ImportError,"WSAStartup failed: requested version not supported");
			break;
		default:
			snprintf(buf, sizeof(buf),"WSAStartup failed: error code %d", ret);
			luaL_error(L, buf);//PyErr_SetString(PyExc_ImportError, buf);
			break;
		}
		return 0; // Failure 
	}

#endif // MS_WINDOWS 

//
//#ifdef PYOS_OS2
//	#define OS_INIT_DEFINED
//
//	/* Additional initialization for OS/2 */
//
//	static int
//	os_init(void)
//	{
//	#ifndef PYCC_GCC
//		char reason[64];
//		int rc = sock_init();
//
//		if (rc == 0) {
//			return 1; /* Success */
//		}
//
//		PyOS_snprintf(reason, sizeof(reason),
//					  "OS/2 TCP/IP Error# %d", sock_errno());
//		PyErr_SetString(PyExc_ImportError, reason);
//
//		return 0;  /* Failure */
//	#else
//		/* No need to initialize sockets with GCC/EMX */
//		return 1; /* Success */
//	#endif
//	}
//
//#endif /* PYOS_OS2 */


#ifndef OS_INIT_DEFINED
static int
os_init(void)
{
	return 1; /* Success */
}
#endif


/* C API table - always add new things to the end for binary
compatibility. */
//static
//PySocketModule_APIObject PySocketModuleAPI =
//{
//    &sock_type,
//    NULL
//};


/* Initialize the _socket module.

This module is actually called "_socket", and there's a wrapper
"socket.py" which implements some additional functionality.  On some
platforms (e.g. Windows and OS/2), socket.py also implements a
wrapper for the socket type that provides missing functionality such
as makefile(), dup() and fromfd().  The import of "_socket" may fail
with an ImportError exception if os-specific initialization fails.
On Windows, this does WINSOCK initialization.  When WINSOCK is
initialized successfully, a call to WSACleanup() is scheduled to be
made at exit time.
*/



/*PyDoc_STRVAR(socket_doc,
"Implementation module for socket operations.\n\
\n\
See the socket module for documentation.");*/
/*
PyMODINIT_FUNC
init_socket(void)
{
	PyObject *m, *has_ipv6;

	if (!os_init())
		return;

	Py_TYPE(&sock_type) = &PyType_Type;
	m = Py_InitModule3(PySocket_MODULE_NAME,
		socket_methods,
		socket_doc);
	if (m == NULL)
		return;

	socket_error = PyErr_NewException("socket.error",
		PyExc_IOError, NULL);
	if (socket_error == NULL)
		return;
	PySocketModuleAPI.error = socket_error;
	Py_INCREF(socket_error);
	PyModule_AddObject(m, "error", socket_error);
	socket_herror = PyErr_NewException("socket.herror",
		socket_error, NULL);
	if (socket_herror == NULL)
		return;
	Py_INCREF(socket_herror);
	PyModule_AddObject(m, "herror", socket_herror);
	socket_gaierror = PyErr_NewException("socket.gaierror", socket_error,
		NULL);
	if (socket_gaierror == NULL)
		return;
	Py_INCREF(socket_gaierror);
	PyModule_AddObject(m, "gaierror", socket_gaierror);
	socket_timeout = PyErr_NewException("socket.timeout",
		socket_error, NULL);
	if (socket_timeout == NULL)
		return;
	Py_INCREF(socket_timeout);
	PyModule_AddObject(m, "timeout", socket_timeout);
	Py_INCREF((PyObject *)&sock_type);
	if (PyModule_AddObject(m, "SocketType",
		(PyObject *)&sock_type) != 0)
		return;
	Py_INCREF((PyObject *)&sock_type);
	if (PyModule_AddObject(m, "socket",
		(PyObject *)&sock_type) != 0)
		return;

#ifdef ENABLE_IPV6
	has_ipv6 = Py_True;
#else
	has_ipv6 = Py_False;
#endif
	Py_INCREF(has_ipv6);
	PyModule_AddObject(m, "has_ipv6", has_ipv6);

	// Export C API
	if (PyModule_AddObject(m, PySocket_CAPI_NAME,
		PyCapsule_New(&PySocketModuleAPI, PySocket_CAPSULE_NAME, NULL)
		) != 0)
		return;

	// Address families (we only support AF_INET and AF_UNIX)
#ifdef AF_UNSPEC
	PyModule_AddIntConstant(m, "AF_UNSPEC", AF_UNSPEC);
#endif
	PyModule_AddIntConstant(m, "AF_INET", AF_INET);
#ifdef AF_INET6
	PyModule_AddIntConstant(m, "AF_INET6", AF_INET6);
#endif // AF_INET6 
#if defined(AF_UNIX)
	PyModule_AddIntConstant(m, "AF_UNIX", AF_UNIX);
#endif // AF_UNIX 
#ifdef AF_AX25
	// Amateur Radio AX.25 
	PyModule_AddIntConstant(m, "AF_AX25", AF_AX25);
#endif
#ifdef AF_IPX
	PyModule_AddIntConstant(m, "AF_IPX", AF_IPX); // Novell IPX 
#endif
#ifdef AF_APPLETALK
												  // Appletalk DDP 
	PyModule_AddIntConstant(m, "AF_APPLETALK", AF_APPLETALK);
#endif
#ifdef AF_NETROM
	// Amateur radio NetROM 
	PyModule_AddIntConstant(m, "AF_NETROM", AF_NETROM);
#endif
#ifdef AF_BRIDGE
	// Multiprotocol bridge 
	PyModule_AddIntConstant(m, "AF_BRIDGE", AF_BRIDGE);
#endif
#ifdef AF_ATMPVC
	// ATM PVCs 
	PyModule_AddIntConstant(m, "AF_ATMPVC", AF_ATMPVC);
#endif
#ifdef AF_AAL5
	// Reserved for Werner's ATM 
	PyModule_AddIntConstant(m, "AF_AAL5", AF_AAL5);
#endif
#ifdef AF_X25
	// Reserved for X.25 project 
	PyModule_AddIntConstant(m, "AF_X25", AF_X25);
#endif
#ifdef AF_INET6
	PyModule_AddIntConstant(m, "AF_INET6", AF_INET6); // IP version 6 
#endif
#ifdef AF_ROSE
													  // Amateur Radio X.25 PLP 
	PyModule_AddIntConstant(m, "AF_ROSE", AF_ROSE);
#endif
#ifdef AF_DECnet
	// Reserved for DECnet project 
	PyModule_AddIntConstant(m, "AF_DECnet", AF_DECnet);
#endif
#ifdef AF_NETBEUI
	// Reserved for 802.2LLC project 
	PyModule_AddIntConstant(m, "AF_NETBEUI", AF_NETBEUI);
#endif
#ifdef AF_SECURITY
	// Security callback pseudo AF 
	PyModule_AddIntConstant(m, "AF_SECURITY", AF_SECURITY);
#endif
#ifdef AF_KEY
	// PF_KEY key management API 
	PyModule_AddIntConstant(m, "AF_KEY", AF_KEY);
#endif
#ifdef AF_NETLINK
	//  
	PyModule_AddIntConstant(m, "AF_NETLINK", AF_NETLINK);
	PyModule_AddIntConstant(m, "NETLINK_ROUTE", NETLINK_ROUTE);
#ifdef NETLINK_SKIP
	PyModule_AddIntConstant(m, "NETLINK_SKIP", NETLINK_SKIP);
#endif
#ifdef NETLINK_W1
	PyModule_AddIntConstant(m, "NETLINK_W1", NETLINK_W1);
#endif
	PyModule_AddIntConstant(m, "NETLINK_USERSOCK", NETLINK_USERSOCK);
	PyModule_AddIntConstant(m, "NETLINK_FIREWALL", NETLINK_FIREWALL);
#ifdef NETLINK_TCPDIAG
	PyModule_AddIntConstant(m, "NETLINK_TCPDIAG", NETLINK_TCPDIAG);
#endif
#ifdef NETLINK_NFLOG
	PyModule_AddIntConstant(m, "NETLINK_NFLOG", NETLINK_NFLOG);
#endif
#ifdef NETLINK_XFRM
	PyModule_AddIntConstant(m, "NETLINK_XFRM", NETLINK_XFRM);
#endif
#ifdef NETLINK_ARPD
	PyModule_AddIntConstant(m, "NETLINK_ARPD", NETLINK_ARPD);
#endif
#ifdef NETLINK_ROUTE6
	PyModule_AddIntConstant(m, "NETLINK_ROUTE6", NETLINK_ROUTE6);
#endif
	PyModule_AddIntConstant(m, "NETLINK_IP6_FW", NETLINK_IP6_FW);
#ifdef NETLINK_DNRTMSG
	PyModule_AddIntConstant(m, "NETLINK_DNRTMSG", NETLINK_DNRTMSG);
#endif
#ifdef NETLINK_TAPBASE
	PyModule_AddIntConstant(m, "NETLINK_TAPBASE", NETLINK_TAPBASE);
#endif
#endif // AF_NETLINK 
#ifdef AF_ROUTE
	// Alias to emulate 4.4BSD 
	PyModule_AddIntConstant(m, "AF_ROUTE", AF_ROUTE);
#endif
#ifdef AF_ASH
	// Ash 
	PyModule_AddIntConstant(m, "AF_ASH", AF_ASH);
#endif
#ifdef AF_ECONET
	// Acorn Econet 
	PyModule_AddIntConstant(m, "AF_ECONET", AF_ECONET);
#endif
#ifdef AF_ATMSVC
	// ATM SVCs 
	PyModule_AddIntConstant(m, "AF_ATMSVC", AF_ATMSVC);
#endif
#ifdef AF_SNA
	// Linux SNA Project (nutters!) 
	PyModule_AddIntConstant(m, "AF_SNA", AF_SNA);
#endif
#ifdef AF_IRDA
	// IRDA sockets 
	PyModule_AddIntConstant(m, "AF_IRDA", AF_IRDA);
#endif
#ifdef AF_PPPOX
	// PPPoX sockets 
	PyModule_AddIntConstant(m, "AF_PPPOX", AF_PPPOX);
#endif
#ifdef AF_WANPIPE
	// Wanpipe API Sockets 
	PyModule_AddIntConstant(m, "AF_WANPIPE", AF_WANPIPE);
#endif
#ifdef AF_LLC
	// Linux LLC 
	PyModule_AddIntConstant(m, "AF_LLC", AF_LLC);
#endif

#ifdef USE_BLUETOOTH
	PyModule_AddIntConstant(m, "AF_BLUETOOTH", AF_BLUETOOTH);
	PyModule_AddIntConstant(m, "BTPROTO_L2CAP", BTPROTO_L2CAP);
	PyModule_AddIntConstant(m, "BTPROTO_HCI", BTPROTO_HCI);
	PyModule_AddIntConstant(m, "SOL_HCI", SOL_HCI);
#if !defined(__NetBSD__) && !defined(__DragonFly__)
	PyModule_AddIntConstant(m, "HCI_FILTER", HCI_FILTER);
#endif
#if !defined(__FreeBSD__)
#if !defined(__NetBSD__) && !defined(__DragonFly__)
	PyModule_AddIntConstant(m, "HCI_TIME_STAMP", HCI_TIME_STAMP);
#endif
	PyModule_AddIntConstant(m, "HCI_DATA_DIR", HCI_DATA_DIR);
	PyModule_AddIntConstant(m, "BTPROTO_SCO", BTPROTO_SCO);
#endif
	PyModule_AddIntConstant(m, "BTPROTO_RFCOMM", BTPROTO_RFCOMM);
	PyModule_AddStringConstant(m, "BDADDR_ANY", "00:00:00:00:00:00");
	PyModule_AddStringConstant(m, "BDADDR_LOCAL", "00:00:00:FF:FF:FF");
#endif

#ifdef AF_PACKET
	PyModule_AddIntMacro(m, AF_PACKET);
#endif
#ifdef PF_PACKET
	PyModule_AddIntMacro(m, PF_PACKET);
#endif
#ifdef PACKET_HOST
	PyModule_AddIntMacro(m, PACKET_HOST);
#endif
#ifdef PACKET_BROADCAST
	PyModule_AddIntMacro(m, PACKET_BROADCAST);
#endif
#ifdef PACKET_MULTICAST
	PyModule_AddIntMacro(m, PACKET_MULTICAST);
#endif
#ifdef PACKET_OTHERHOST
	PyModule_AddIntMacro(m, PACKET_OTHERHOST);
#endif
#ifdef PACKET_OUTGOING
	PyModule_AddIntMacro(m, PACKET_OUTGOING);
#endif
#ifdef PACKET_LOOPBACK
	PyModule_AddIntMacro(m, PACKET_LOOPBACK);
#endif
#ifdef PACKET_FASTROUTE
	PyModule_AddIntMacro(m, PACKET_FASTROUTE);
#endif

#ifdef HAVE_LINUX_TIPC_H
	PyModule_AddIntConstant(m, "AF_TIPC", AF_TIPC);

	// for addresses 
	PyModule_AddIntConstant(m, "TIPC_ADDR_NAMESEQ", TIPC_ADDR_NAMESEQ);
	PyModule_AddIntConstant(m, "TIPC_ADDR_NAME", TIPC_ADDR_NAME);
	PyModule_AddIntConstant(m, "TIPC_ADDR_ID", TIPC_ADDR_ID);

	PyModule_AddIntConstant(m, "TIPC_ZONE_SCOPE", TIPC_ZONE_SCOPE);
	PyModule_AddIntConstant(m, "TIPC_CLUSTER_SCOPE", TIPC_CLUSTER_SCOPE);
	PyModule_AddIntConstant(m, "TIPC_NODE_SCOPE", TIPC_NODE_SCOPE);

	// for setsockopt() 
	PyModule_AddIntConstant(m, "SOL_TIPC", SOL_TIPC);
	PyModule_AddIntConstant(m, "TIPC_IMPORTANCE", TIPC_IMPORTANCE);
	PyModule_AddIntConstant(m, "TIPC_SRC_DROPPABLE", TIPC_SRC_DROPPABLE);
	PyModule_AddIntConstant(m, "TIPC_DEST_DROPPABLE",
		TIPC_DEST_DROPPABLE);
	PyModule_AddIntConstant(m, "TIPC_CONN_TIMEOUT", TIPC_CONN_TIMEOUT);

	PyModule_AddIntConstant(m, "TIPC_LOW_IMPORTANCE",
		TIPC_LOW_IMPORTANCE);
	PyModule_AddIntConstant(m, "TIPC_MEDIUM_IMPORTANCE",
		TIPC_MEDIUM_IMPORTANCE);
	PyModule_AddIntConstant(m, "TIPC_HIGH_IMPORTANCE",
		TIPC_HIGH_IMPORTANCE);
	PyModule_AddIntConstant(m, "TIPC_CRITICAL_IMPORTANCE",
		TIPC_CRITICAL_IMPORTANCE);

	// for subscriptions 
	PyModule_AddIntConstant(m, "TIPC_SUB_PORTS", TIPC_SUB_PORTS);
	PyModule_AddIntConstant(m, "TIPC_SUB_SERVICE", TIPC_SUB_SERVICE);
#ifdef TIPC_SUB_CANCEL
	// doesn't seem to be available everywhere 
	PyModule_AddIntConstant(m, "TIPC_SUB_CANCEL", TIPC_SUB_CANCEL);
#endif
	PyModule_AddIntConstant(m, "TIPC_WAIT_FOREVER", TIPC_WAIT_FOREVER);
	PyModule_AddIntConstant(m, "TIPC_PUBLISHED", TIPC_PUBLISHED);
	PyModule_AddIntConstant(m, "TIPC_WITHDRAWN", TIPC_WITHDRAWN);
	PyModule_AddIntConstant(m, "TIPC_SUBSCR_TIMEOUT", TIPC_SUBSCR_TIMEOUT);
	PyModule_AddIntConstant(m, "TIPC_CFG_SRV", TIPC_CFG_SRV);
	PyModule_AddIntConstant(m, "TIPC_TOP_SRV", TIPC_TOP_SRV);
#endif

	// Socket types 
	PyModule_AddIntConstant(m, "SOCK_STREAM", SOCK_STREAM);
	PyModule_AddIntConstant(m, "SOCK_DGRAM", SOCK_DGRAM);
#ifndef __BEOS__
	// We have incomplete socket support. 
	PyModule_AddIntConstant(m, "SOCK_RAW", SOCK_RAW);
	PyModule_AddIntConstant(m, "SOCK_SEQPACKET", SOCK_SEQPACKET);
#if defined(SOCK_RDM)
	PyModule_AddIntConstant(m, "SOCK_RDM", SOCK_RDM);
#endif
#endif

#ifdef  SO_DEBUG
	PyModule_AddIntConstant(m, "SO_DEBUG", SO_DEBUG);
#endif
#ifdef  SO_ACCEPTCONN
	PyModule_AddIntConstant(m, "SO_ACCEPTCONN", SO_ACCEPTCONN);
#endif
#ifdef  SO_REUSEADDR
	PyModule_AddIntConstant(m, "SO_REUSEADDR", SO_REUSEADDR);
#endif
#ifdef SO_EXCLUSIVEADDRUSE
	PyModule_AddIntConstant(m, "SO_EXCLUSIVEADDRUSE", SO_EXCLUSIVEADDRUSE);
#endif

#ifdef  SO_KEEPALIVE
	PyModule_AddIntConstant(m, "SO_KEEPALIVE", SO_KEEPALIVE);
#endif
#ifdef  SO_DONTROUTE
	PyModule_AddIntConstant(m, "SO_DONTROUTE", SO_DONTROUTE);
#endif
#ifdef  SO_BROADCAST
	PyModule_AddIntConstant(m, "SO_BROADCAST", SO_BROADCAST);
#endif
#ifdef  SO_USELOOPBACK
	PyModule_AddIntConstant(m, "SO_USELOOPBACK", SO_USELOOPBACK);
#endif
#ifdef  SO_LINGER
	PyModule_AddIntConstant(m, "SO_LINGER", SO_LINGER);
#endif
#ifdef  SO_OOBINLINE
	PyModule_AddIntConstant(m, "SO_OOBINLINE", SO_OOBINLINE);
#endif
#ifdef  SO_REUSEPORT
	PyModule_AddIntConstant(m, "SO_REUSEPORT", SO_REUSEPORT);
#endif
#ifdef  SO_SNDBUF
	PyModule_AddIntConstant(m, "SO_SNDBUF", SO_SNDBUF);
#endif
#ifdef  SO_RCVBUF
	PyModule_AddIntConstant(m, "SO_RCVBUF", SO_RCVBUF);
#endif
#ifdef  SO_SNDLOWAT
	PyModule_AddIntConstant(m, "SO_SNDLOWAT", SO_SNDLOWAT);
#endif
#ifdef  SO_RCVLOWAT
	PyModule_AddIntConstant(m, "SO_RCVLOWAT", SO_RCVLOWAT);
#endif
#ifdef  SO_SNDTIMEO
	PyModule_AddIntConstant(m, "SO_SNDTIMEO", SO_SNDTIMEO);
#endif
#ifdef  SO_RCVTIMEO
	PyModule_AddIntConstant(m, "SO_RCVTIMEO", SO_RCVTIMEO);
#endif
#ifdef  SO_ERROR
	PyModule_AddIntConstant(m, "SO_ERROR", SO_ERROR);
#endif
#ifdef  SO_TYPE
	PyModule_AddIntConstant(m, "SO_TYPE", SO_TYPE);
#endif
#ifdef SO_SETFIB
	PyModule_AddIntConstant(m, "SO_SETFIB", SO_SETFIB);
#endif

	// Maximum number of connections for "listen" 
#ifdef  SOMAXCONN
	PyModule_AddIntConstant(m, "SOMAXCONN", SOMAXCONN);
#else
	PyModule_AddIntConstant(m, "SOMAXCONN", 5); // Common value 
#endif

												// Flags for send, recv 
#ifdef  MSG_OOB
	PyModule_AddIntConstant(m, "MSG_OOB", MSG_OOB);
#endif
#ifdef  MSG_PEEK
	PyModule_AddIntConstant(m, "MSG_PEEK", MSG_PEEK);
#endif
#ifdef  MSG_DONTROUTE
	PyModule_AddIntConstant(m, "MSG_DONTROUTE", MSG_DONTROUTE);
#endif
#ifdef  MSG_DONTWAIT
	PyModule_AddIntConstant(m, "MSG_DONTWAIT", MSG_DONTWAIT);
#endif
#ifdef  MSG_EOR
	PyModule_AddIntConstant(m, "MSG_EOR", MSG_EOR);
#endif
#ifdef  MSG_TRUNC
	PyModule_AddIntConstant(m, "MSG_TRUNC", MSG_TRUNC);
#endif
#ifdef  MSG_CTRUNC
	PyModule_AddIntConstant(m, "MSG_CTRUNC", MSG_CTRUNC);
#endif
#ifdef  MSG_WAITALL
	PyModule_AddIntConstant(m, "MSG_WAITALL", MSG_WAITALL);
#endif
#ifdef  MSG_BTAG
	PyModule_AddIntConstant(m, "MSG_BTAG", MSG_BTAG);
#endif
#ifdef  MSG_ETAG
	PyModule_AddIntConstant(m, "MSG_ETAG", MSG_ETAG);
#endif

	// Protocol level and numbers, usable for [gs]etsockopt 
#ifdef  SOL_SOCKET
	PyModule_AddIntConstant(m, "SOL_SOCKET", SOL_SOCKET);
#endif
#ifdef  SOL_IP
	PyModule_AddIntConstant(m, "SOL_IP", SOL_IP);
#else
	PyModule_AddIntConstant(m, "SOL_IP", 0);
#endif
#ifdef  SOL_IPX
	PyModule_AddIntConstant(m, "SOL_IPX", SOL_IPX);
#endif
#ifdef  SOL_AX25
	PyModule_AddIntConstant(m, "SOL_AX25", SOL_AX25);
#endif
#ifdef  SOL_ATALK
	PyModule_AddIntConstant(m, "SOL_ATALK", SOL_ATALK);
#endif
#ifdef  SOL_NETROM
	PyModule_AddIntConstant(m, "SOL_NETROM", SOL_NETROM);
#endif
#ifdef  SOL_ROSE
	PyModule_AddIntConstant(m, "SOL_ROSE", SOL_ROSE);
#endif
#ifdef  SOL_TCP
	PyModule_AddIntConstant(m, "SOL_TCP", SOL_TCP);
#else
	PyModule_AddIntConstant(m, "SOL_TCP", 6);
#endif
#ifdef  SOL_UDP
	PyModule_AddIntConstant(m, "SOL_UDP", SOL_UDP);
#else
	PyModule_AddIntConstant(m, "SOL_UDP", 17);
#endif
#ifdef  IPPROTO_IP
	PyModule_AddIntConstant(m, "IPPROTO_IP", IPPROTO_IP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_IP", 0);
#endif
#ifdef  IPPROTO_HOPOPTS
	PyModule_AddIntConstant(m, "IPPROTO_HOPOPTS", IPPROTO_HOPOPTS);
#endif
#ifdef  IPPROTO_ICMP
	PyModule_AddIntConstant(m, "IPPROTO_ICMP", IPPROTO_ICMP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_ICMP", 1);
#endif
#ifdef  IPPROTO_IGMP
	PyModule_AddIntConstant(m, "IPPROTO_IGMP", IPPROTO_IGMP);
#endif
#ifdef  IPPROTO_GGP
	PyModule_AddIntConstant(m, "IPPROTO_GGP", IPPROTO_GGP);
#endif
#ifdef  IPPROTO_IPV4
	PyModule_AddIntConstant(m, "IPPROTO_IPV4", IPPROTO_IPV4);
#endif
#ifdef  IPPROTO_IPV6
	PyModule_AddIntConstant(m, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif
#ifdef  IPPROTO_IPIP
	PyModule_AddIntConstant(m, "IPPROTO_IPIP", IPPROTO_IPIP);
#endif
#ifdef  IPPROTO_TCP
	PyModule_AddIntConstant(m, "IPPROTO_TCP", IPPROTO_TCP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_TCP", 6);
#endif
#ifdef  IPPROTO_EGP
	PyModule_AddIntConstant(m, "IPPROTO_EGP", IPPROTO_EGP);
#endif
#ifdef  IPPROTO_PUP
	PyModule_AddIntConstant(m, "IPPROTO_PUP", IPPROTO_PUP);
#endif
#ifdef  IPPROTO_UDP
	PyModule_AddIntConstant(m, "IPPROTO_UDP", IPPROTO_UDP);
#else
	PyModule_AddIntConstant(m, "IPPROTO_UDP", 17);
#endif
#ifdef  IPPROTO_IDP
	PyModule_AddIntConstant(m, "IPPROTO_IDP", IPPROTO_IDP);
#endif
#ifdef  IPPROTO_HELLO
	PyModule_AddIntConstant(m, "IPPROTO_HELLO", IPPROTO_HELLO);
#endif
#ifdef  IPPROTO_ND
	PyModule_AddIntConstant(m, "IPPROTO_ND", IPPROTO_ND);
#endif
#ifdef  IPPROTO_TP
	PyModule_AddIntConstant(m, "IPPROTO_TP", IPPROTO_TP);
#endif
#ifdef  IPPROTO_IPV6
	PyModule_AddIntConstant(m, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif
#ifdef  IPPROTO_ROUTING
	PyModule_AddIntConstant(m, "IPPROTO_ROUTING", IPPROTO_ROUTING);
#endif
#ifdef  IPPROTO_FRAGMENT
	PyModule_AddIntConstant(m, "IPPROTO_FRAGMENT", IPPROTO_FRAGMENT);
#endif
#ifdef  IPPROTO_RSVP
	PyModule_AddIntConstant(m, "IPPROTO_RSVP", IPPROTO_RSVP);
#endif
#ifdef  IPPROTO_GRE
	PyModule_AddIntConstant(m, "IPPROTO_GRE", IPPROTO_GRE);
#endif
#ifdef  IPPROTO_ESP
	PyModule_AddIntConstant(m, "IPPROTO_ESP", IPPROTO_ESP);
#endif
#ifdef  IPPROTO_AH
	PyModule_AddIntConstant(m, "IPPROTO_AH", IPPROTO_AH);
#endif
#ifdef  IPPROTO_MOBILE
	PyModule_AddIntConstant(m, "IPPROTO_MOBILE", IPPROTO_MOBILE);
#endif
#ifdef  IPPROTO_ICMPV6
	PyModule_AddIntConstant(m, "IPPROTO_ICMPV6", IPPROTO_ICMPV6);
#endif
#ifdef  IPPROTO_NONE
	PyModule_AddIntConstant(m, "IPPROTO_NONE", IPPROTO_NONE);
#endif
#ifdef  IPPROTO_DSTOPTS
	PyModule_AddIntConstant(m, "IPPROTO_DSTOPTS", IPPROTO_DSTOPTS);
#endif
#ifdef  IPPROTO_XTP
	PyModule_AddIntConstant(m, "IPPROTO_XTP", IPPROTO_XTP);
#endif
#ifdef  IPPROTO_EON
	PyModule_AddIntConstant(m, "IPPROTO_EON", IPPROTO_EON);
#endif
#ifdef  IPPROTO_PIM
	PyModule_AddIntConstant(m, "IPPROTO_PIM", IPPROTO_PIM);
#endif
#ifdef  IPPROTO_IPCOMP
	PyModule_AddIntConstant(m, "IPPROTO_IPCOMP", IPPROTO_IPCOMP);
#endif
#ifdef  IPPROTO_VRRP
	PyModule_AddIntConstant(m, "IPPROTO_VRRP", IPPROTO_VRRP);
#endif
#ifdef  IPPROTO_BIP
	PyModule_AddIntConstant(m, "IPPROTO_BIP", IPPROTO_BIP);
#endif
	
#ifdef  IPPROTO_RAW
	PyModule_AddIntConstant(m, "IPPROTO_RAW", IPPROTO_RAW);
#else
	PyModule_AddIntConstant(m, "IPPROTO_RAW", 255);
#endif
#ifdef  IPPROTO_MAX
	PyModule_AddIntConstant(m, "IPPROTO_MAX", IPPROTO_MAX);
#endif

	// Some port configuration 
#ifdef  IPPORT_RESERVED
	PyModule_AddIntConstant(m, "IPPORT_RESERVED", IPPORT_RESERVED);
#else
	PyModule_AddIntConstant(m, "IPPORT_RESERVED", 1024);
#endif
#ifdef  IPPORT_USERRESERVED
	PyModule_AddIntConstant(m, "IPPORT_USERRESERVED", IPPORT_USERRESERVED);
#else
	PyModule_AddIntConstant(m, "IPPORT_USERRESERVED", 5000);
#endif

	// Some reserved IP v.4 addresses 
#ifdef  INADDR_ANY
	PyModule_AddIntConstant(m, "INADDR_ANY", INADDR_ANY);
#else
	PyModule_AddIntConstant(m, "INADDR_ANY", 0x00000000);
#endif
#ifdef  INADDR_BROADCAST
	PyModule_AddIntConstant(m, "INADDR_BROADCAST", INADDR_BROADCAST);
#else
	PyModule_AddIntConstant(m, "INADDR_BROADCAST", 0xffffffff);
#endif
#ifdef  INADDR_LOOPBACK
	PyModule_AddIntConstant(m, "INADDR_LOOPBACK", INADDR_LOOPBACK);
#else
	PyModule_AddIntConstant(m, "INADDR_LOOPBACK", 0x7F000001);
#endif
#ifdef  INADDR_UNSPEC_GROUP
	PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", INADDR_UNSPEC_GROUP);
#else
	PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", 0xe0000000);
#endif
#ifdef  INADDR_ALLHOSTS_GROUP
	PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP",
		INADDR_ALLHOSTS_GROUP);
#else
	PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
#endif
#ifdef  INADDR_MAX_LOCAL_GROUP
	PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP",
		INADDR_MAX_LOCAL_GROUP);
#else
	PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
#endif
#ifdef  INADDR_NONE
	PyModule_AddIntConstant(m, "INADDR_NONE", INADDR_NONE);
#else
	PyModule_AddIntConstant(m, "INADDR_NONE", 0xffffffff);
#endif

	// IPv4 [gs]etsockopt options 
#ifdef  IP_OPTIONS
	PyModule_AddIntConstant(m, "IP_OPTIONS", IP_OPTIONS);
#endif
#ifdef  IP_HDRINCL
	PyModule_AddIntConstant(m, "IP_HDRINCL", IP_HDRINCL);
#endif
#ifdef  IP_TOS
	PyModule_AddIntConstant(m, "IP_TOS", IP_TOS);
#endif
#ifdef  IP_TTL
	PyModule_AddIntConstant(m, "IP_TTL", IP_TTL);
#endif
#ifdef  IP_RECVOPTS
	PyModule_AddIntConstant(m, "IP_RECVOPTS", IP_RECVOPTS);
#endif
#ifdef  IP_RECVRETOPTS
	PyModule_AddIntConstant(m, "IP_RECVRETOPTS", IP_RECVRETOPTS);
#endif
#ifdef  IP_RECVDSTADDR
	PyModule_AddIntConstant(m, "IP_RECVDSTADDR", IP_RECVDSTADDR);
#endif
#ifdef  IP_RETOPTS
	PyModule_AddIntConstant(m, "IP_RETOPTS", IP_RETOPTS);
#endif
#ifdef  IP_MULTICAST_IF
	PyModule_AddIntConstant(m, "IP_MULTICAST_IF", IP_MULTICAST_IF);
#endif
#ifdef  IP_MULTICAST_TTL
	PyModule_AddIntConstant(m, "IP_MULTICAST_TTL", IP_MULTICAST_TTL);
#endif
#ifdef  IP_MULTICAST_LOOP
	PyModule_AddIntConstant(m, "IP_MULTICAST_LOOP", IP_MULTICAST_LOOP);
#endif
#ifdef  IP_ADD_MEMBERSHIP
	PyModule_AddIntConstant(m, "IP_ADD_MEMBERSHIP", IP_ADD_MEMBERSHIP);
#endif
#ifdef  IP_DROP_MEMBERSHIP
	PyModule_AddIntConstant(m, "IP_DROP_MEMBERSHIP", IP_DROP_MEMBERSHIP);
#endif
#ifdef  IP_DEFAULT_MULTICAST_TTL
	PyModule_AddIntConstant(m, "IP_DEFAULT_MULTICAST_TTL",
		IP_DEFAULT_MULTICAST_TTL);
#endif
#ifdef  IP_DEFAULT_MULTICAST_LOOP
	PyModule_AddIntConstant(m, "IP_DEFAULT_MULTICAST_LOOP",
		IP_DEFAULT_MULTICAST_LOOP);
#endif
#ifdef  IP_MAX_MEMBERSHIPS
	PyModule_AddIntConstant(m, "IP_MAX_MEMBERSHIPS", IP_MAX_MEMBERSHIPS);
#endif

	// IPv6 [gs]etsockopt options, defined in RFC2553 
#ifdef  IPV6_JOIN_GROUP
	PyModule_AddIntConstant(m, "IPV6_JOIN_GROUP", IPV6_JOIN_GROUP);
#endif
#ifdef  IPV6_LEAVE_GROUP
	PyModule_AddIntConstant(m, "IPV6_LEAVE_GROUP", IPV6_LEAVE_GROUP);
#endif
#ifdef  IPV6_MULTICAST_HOPS
	PyModule_AddIntConstant(m, "IPV6_MULTICAST_HOPS", IPV6_MULTICAST_HOPS);
#endif
#ifdef  IPV6_MULTICAST_IF
	PyModule_AddIntConstant(m, "IPV6_MULTICAST_IF", IPV6_MULTICAST_IF);
#endif
#ifdef  IPV6_MULTICAST_LOOP
	PyModule_AddIntConstant(m, "IPV6_MULTICAST_LOOP", IPV6_MULTICAST_LOOP);
#endif
#ifdef  IPV6_UNICAST_HOPS
	PyModule_AddIntConstant(m, "IPV6_UNICAST_HOPS", IPV6_UNICAST_HOPS);
#endif
	// Additional IPV6 socket options, defined in RFC 3493 
#ifdef IPV6_V6ONLY
	PyModule_AddIntConstant(m, "IPV6_V6ONLY", IPV6_V6ONLY);
#endif
	// Advanced IPV6 socket options, from RFC 3542 
#ifdef IPV6_CHECKSUM
	PyModule_AddIntConstant(m, "IPV6_CHECKSUM", IPV6_CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
	PyModule_AddIntConstant(m, "IPV6_DONTFRAG", IPV6_DONTFRAG);
#endif
#ifdef IPV6_DSTOPTS
	PyModule_AddIntConstant(m, "IPV6_DSTOPTS", IPV6_DSTOPTS);
#endif
#ifdef IPV6_HOPLIMIT
	PyModule_AddIntConstant(m, "IPV6_HOPLIMIT", IPV6_HOPLIMIT);
#endif
#ifdef IPV6_HOPOPTS
	PyModule_AddIntConstant(m, "IPV6_HOPOPTS", IPV6_HOPOPTS);
#endif
#ifdef IPV6_NEXTHOP
	PyModule_AddIntConstant(m, "IPV6_NEXTHOP", IPV6_NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
	PyModule_AddIntConstant(m, "IPV6_PATHMTU", IPV6_PATHMTU);
#endif
#ifdef IPV6_PKTINFO
	PyModule_AddIntConstant(m, "IPV6_PKTINFO", IPV6_PKTINFO);
#endif
#ifdef IPV6_RECVDSTOPTS
	PyModule_AddIntConstant(m, "IPV6_RECVDSTOPTS", IPV6_RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
	PyModule_AddIntConstant(m, "IPV6_RECVHOPLIMIT", IPV6_RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
	PyModule_AddIntConstant(m, "IPV6_RECVHOPOPTS", IPV6_RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPKTINFO
	PyModule_AddIntConstant(m, "IPV6_RECVPKTINFO", IPV6_RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
	PyModule_AddIntConstant(m, "IPV6_RECVRTHDR", IPV6_RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
	PyModule_AddIntConstant(m, "IPV6_RECVTCLASS", IPV6_RECVTCLASS);
#endif
#ifdef IPV6_RTHDR
	PyModule_AddIntConstant(m, "IPV6_RTHDR", IPV6_RTHDR);
#endif
#ifdef IPV6_RTHDRDSTOPTS
	PyModule_AddIntConstant(m, "IPV6_RTHDRDSTOPTS", IPV6_RTHDRDSTOPTS);
#endif
#ifdef IPV6_RTHDR_TYPE_0
	PyModule_AddIntConstant(m, "IPV6_RTHDR_TYPE_0", IPV6_RTHDR_TYPE_0);
#endif
#ifdef IPV6_RECVPATHMTU
	PyModule_AddIntConstant(m, "IPV6_RECVPATHMTU", IPV6_RECVPATHMTU);
#endif
#ifdef IPV6_TCLASS
	PyModule_AddIntConstant(m, "IPV6_TCLASS", IPV6_TCLASS);
#endif
#ifdef IPV6_USE_MIN_MTU
	PyModule_AddIntConstant(m, "IPV6_USE_MIN_MTU", IPV6_USE_MIN_MTU);
#endif

	// TCP options 
#ifdef  TCP_NODELAY
	PyModule_AddIntConstant(m, "TCP_NODELAY", TCP_NODELAY);
#endif
#ifdef  TCP_MAXSEG
	PyModule_AddIntConstant(m, "TCP_MAXSEG", TCP_MAXSEG);
#endif
#ifdef  TCP_CORK
	PyModule_AddIntConstant(m, "TCP_CORK", TCP_CORK);
#endif
#ifdef  TCP_KEEPIDLE
	PyModule_AddIntConstant(m, "TCP_KEEPIDLE", TCP_KEEPIDLE);
#endif
#ifdef  TCP_KEEPINTVL
	PyModule_AddIntConstant(m, "TCP_KEEPINTVL", TCP_KEEPINTVL);
#endif
#ifdef  TCP_KEEPCNT
	PyModule_AddIntConstant(m, "TCP_KEEPCNT", TCP_KEEPCNT);
#endif
#ifdef  TCP_SYNCNT
	PyModule_AddIntConstant(m, "TCP_SYNCNT", TCP_SYNCNT);
#endif
#ifdef  TCP_LINGER2
	PyModule_AddIntConstant(m, "TCP_LINGER2", TCP_LINGER2);
#endif
#ifdef  TCP_DEFER_ACCEPT
	PyModule_AddIntConstant(m, "TCP_DEFER_ACCEPT", TCP_DEFER_ACCEPT);
#endif
#ifdef  TCP_WINDOW_CLAMP
	PyModule_AddIntConstant(m, "TCP_WINDOW_CLAMP", TCP_WINDOW_CLAMP);
#endif
#ifdef  TCP_INFO
	PyModule_AddIntConstant(m, "TCP_INFO", TCP_INFO);
#endif
#ifdef  TCP_QUICKACK
	PyModule_AddIntConstant(m, "TCP_QUICKACK", TCP_QUICKACK);
#endif


	// IPX options 
#ifdef  IPX_TYPE
	PyModule_AddIntConstant(m, "IPX_TYPE", IPX_TYPE);
#endif

	// get{addr,name}info parameters 
#ifdef EAI_ADDRFAMILY
	PyModule_AddIntConstant(m, "EAI_ADDRFAMILY", EAI_ADDRFAMILY);
#endif
#ifdef EAI_AGAIN
	PyModule_AddIntConstant(m, "EAI_AGAIN", EAI_AGAIN);
#endif
#ifdef EAI_BADFLAGS
	PyModule_AddIntConstant(m, "EAI_BADFLAGS", EAI_BADFLAGS);
#endif
#ifdef EAI_FAIL
	PyModule_AddIntConstant(m, "EAI_FAIL", EAI_FAIL);
#endif
#ifdef EAI_FAMILY
	PyModule_AddIntConstant(m, "EAI_FAMILY", EAI_FAMILY);
#endif
#ifdef EAI_MEMORY
	PyModule_AddIntConstant(m, "EAI_MEMORY", EAI_MEMORY);
#endif
#ifdef EAI_NODATA
	PyModule_AddIntConstant(m, "EAI_NODATA", EAI_NODATA);
#endif
#ifdef EAI_NONAME
	PyModule_AddIntConstant(m, "EAI_NONAME", EAI_NONAME);
#endif
#ifdef EAI_OVERFLOW
	PyModule_AddIntConstant(m, "EAI_OVERFLOW", EAI_OVERFLOW);
#endif
#ifdef EAI_SERVICE
	PyModule_AddIntConstant(m, "EAI_SERVICE", EAI_SERVICE);
#endif
#ifdef EAI_SOCKTYPE
	PyModule_AddIntConstant(m, "EAI_SOCKTYPE", EAI_SOCKTYPE);
#endif
#ifdef EAI_SYSTEM
	PyModule_AddIntConstant(m, "EAI_SYSTEM", EAI_SYSTEM);
#endif
#ifdef EAI_BADHINTS
	PyModule_AddIntConstant(m, "EAI_BADHINTS", EAI_BADHINTS);
#endif
#ifdef EAI_PROTOCOL
	PyModule_AddIntConstant(m, "EAI_PROTOCOL", EAI_PROTOCOL);
#endif
#ifdef EAI_MAX
	PyModule_AddIntConstant(m, "EAI_MAX", EAI_MAX);
#endif
#ifdef AI_PASSIVE
	PyModule_AddIntConstant(m, "AI_PASSIVE", AI_PASSIVE);
#endif
#ifdef AI_CANONNAME
	PyModule_AddIntConstant(m, "AI_CANONNAME", AI_CANONNAME);
#endif
#ifdef AI_NUMERICHOST
	PyModule_AddIntConstant(m, "AI_NUMERICHOST", AI_NUMERICHOST);
#endif
#ifdef AI_NUMERICSERV
	PyModule_AddIntConstant(m, "AI_NUMERICSERV", AI_NUMERICSERV);
#endif
#ifdef AI_MASK
	PyModule_AddIntConstant(m, "AI_MASK", AI_MASK);
#endif
#ifdef AI_ALL
	PyModule_AddIntConstant(m, "AI_ALL", AI_ALL);
#endif
#ifdef AI_V4MAPPED_CFG
	PyModule_AddIntConstant(m, "AI_V4MAPPED_CFG", AI_V4MAPPED_CFG);
#endif
#ifdef AI_ADDRCONFIG
	PyModule_AddIntConstant(m, "AI_ADDRCONFIG", AI_ADDRCONFIG);
#endif
#ifdef AI_V4MAPPED
	PyModule_AddIntConstant(m, "AI_V4MAPPED", AI_V4MAPPED);
#endif
#ifdef AI_DEFAULT
	PyModule_AddIntConstant(m, "AI_DEFAULT", AI_DEFAULT);
#endif
#ifdef NI_MAXHOST
	PyModule_AddIntConstant(m, "NI_MAXHOST", NI_MAXHOST);
#endif
#ifdef NI_MAXSERV
	PyModule_AddIntConstant(m, "NI_MAXSERV", NI_MAXSERV);
#endif
#ifdef NI_NOFQDN
	PyModule_AddIntConstant(m, "NI_NOFQDN", NI_NOFQDN);
#endif
#ifdef NI_NUMERICHOST
	PyModule_AddIntConstant(m, "NI_NUMERICHOST", NI_NUMERICHOST);
#endif
#ifdef NI_NAMEREQD
	PyModule_AddIntConstant(m, "NI_NAMEREQD", NI_NAMEREQD);
#endif
#ifdef NI_NUMERICSERV
	PyModule_AddIntConstant(m, "NI_NUMERICSERV", NI_NUMERICSERV);
#endif
#ifdef NI_DGRAM
	PyModule_AddIntConstant(m, "NI_DGRAM", NI_DGRAM);
#endif

	// shutdown() parameters 
#ifdef SHUT_RD
	PyModule_AddIntConstant(m, "SHUT_RD", SHUT_RD);
#elif defined(SD_RECEIVE)
	PyModule_AddIntConstant(m, "SHUT_RD", SD_RECEIVE);
#else
	PyModule_AddIntConstant(m, "SHUT_RD", 0);
#endif
#ifdef SHUT_WR
	PyModule_AddIntConstant(m, "SHUT_WR", SHUT_WR);
#elif defined(SD_SEND)
	PyModule_AddIntConstant(m, "SHUT_WR", SD_SEND);
#else
	PyModule_AddIntConstant(m, "SHUT_WR", 1);
#endif
#ifdef SHUT_RDWR
	PyModule_AddIntConstant(m, "SHUT_RDWR", SHUT_RDWR);
#elif defined(SD_BOTH)
	PyModule_AddIntConstant(m, "SHUT_RDWR", SD_BOTH);
#else
	PyModule_AddIntConstant(m, "SHUT_RDWR", 2);
#endif

#ifdef SIO_RCVALL
	{
		DWORD codes[] = { SIO_RCVALL, SIO_KEEPALIVE_VALS };
		const char *names[] = { "SIO_RCVALL", "SIO_KEEPALIVE_VALS" };
		int i;
		for (i = 0; i<sizeof(codes) / sizeof(*codes); ++i) {
			PyObject *tmp;
			tmp = PyLong_FromUnsignedLong(codes[i]);
			if (tmp == NULL)
				return;
			PyModule_AddObject(m, names[i], tmp);
		}
	}
	PyModule_AddIntConstant(m, "RCVALL_OFF", RCVALL_OFF);
	PyModule_AddIntConstant(m, "RCVALL_ON", RCVALL_ON);
	PyModule_AddIntConstant(m, "RCVALL_SOCKETLEVELONLY", RCVALL_SOCKETLEVELONLY);
#ifdef RCVALL_IPLEVEL
	PyModule_AddIntConstant(m, "RCVALL_IPLEVEL", RCVALL_IPLEVEL);
#endif
#ifdef RCVALL_MAX
	PyModule_AddIntConstant(m, "RCVALL_MAX", RCVALL_MAX);
#endif
#endif // _MSTCPIP_ 

	// Initialize gethostbyname lock 
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
	netdb_lock = PyThread_allocate_lock();
#endif
}
*/



//
//#ifndef HAVE_INET_PTON
//	#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_LONGHORN)
//
//		/* Simplistic emulation code for inet_pton that only works for IPv4 */
//		/* These are not exposed because they do not set errno properly */
//
//		int
//		inet_pton(int af, const char *src, void *dst)
//		{
//			if (af == AF_INET) {
//		#if (SIZEOF_INT != 4)
//			#error "Not sure if in_addr_t exists and int is not 32-bits."
//		#endif
//				unsigned int packed_addr;
//				packed_addr = inet_addr(src);
//				if (packed_addr == INADDR_NONE)
//					return 0;
//				memcpy(dst, &packed_addr, 4);
//				return 1;
//			}
//			/* Should set errno to EAFNOSUPPORT */
//			return -1;
//		}
//
//		const char *
//		inet_ntop(int af, const void *src, char *dst, socklen_t size)
//		{
//			if (af == AF_INET) {
//				struct in_addr packed_addr;
//				if (size < 16)
//					/* Should set errno to ENOSPC. */
//					return NULL;
//				memcpy(&packed_addr, src, sizeof(packed_addr));
//				return strncpy(dst, inet_ntoa(packed_addr), size);
//			}
//			/* Should set errno to EAFNOSUPPORT */
//			return NULL;
//		}
//
//	#endif
//#endif