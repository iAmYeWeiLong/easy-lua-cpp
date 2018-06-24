/* Socket module */

/*

This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 and AF_UNIX address families are supported in a
  portable manner, though AF_PACKET, AF_NETLINK and AF_TIPC are supported
  under Linux.
- No read/write operations (use sendall/recv or makefile instead).
- Additional restrictions apply on some non-Unix platforms (compensated
  for by socket.py).

Module interface:

- socket.error: exception raised for socket specific errors
- socket.gaierror: exception raised for getaddrinfo/getnameinfo errors,
    a subclass of socket.error
- socket.herror: exception raised for gethostby* errors,
    a subclass of socket.error
- socket.fromfd(fd, family, type[, proto]) --> new socket object (created
    from an existing file descriptor)
- socket.gethostbyname(hostname) --> host IP address (string: 'dd.dd.dd.dd')
- socket.gethostbyaddr(IP address) --> (hostname, [alias, ...], [IP addr, ...])
- socket.gethostname() --> host name (string: 'spam' or 'spam.domain.com')
- socket.getprotobyname(protocolname) --> protocol number
- socket.getservbyname(servicename[, protocolname]) --> port number
- socket.getservbyport(portnumber[, protocolname]) --> service name
- socket.socket([family[, type [, proto]]]) --> new socket object
- socket.socketpair([family[, type [, proto]]]) --> (socket, socket)
- socket.ntohs(16 bit value) --> new int object
- socket.ntohl(32 bit value) --> new int object
- socket.htons(16 bit value) --> new int object
- socket.htonl(32 bit value) --> new int object
- socket.getaddrinfo(host, port [, family, socktype, proto, flags])
    --> List of (family, socktype, proto, canonname, sockaddr)
- socket.getnameinfo(sockaddr, flags) --> (host, port)
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.has_ipv6: boolean value indicating if IPv6 is supported
- socket.inet_aton(IP address) -> 32-bit packed IP representation
- socket.inet_ntoa(packed IP) -> IP address string
- socket.getdefaulttimeout() -> None | float
- socket.setdefaulttimeout(None | float)
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
- an AF_PACKET socket address is a tuple containing a string
  specifying the ethernet interface and an integer specifying
  the Ethernet protocol number to be received. For example:
  ("eth0",0x1234).  Optional 3rd,4th,5th elements in the tuple
  specify packet-type and ha-type/addr.
- an AF_TIPC socket address is expressed as
 (addr_type, v1, v2, v3 [, scope]); where addr_type can be one of:
    TIPC_ADDR_NAMESEQ, TIPC_ADDR_NAME, and TIPC_ADDR_ID;
  and scope can be one of:
    TIPC_ZONE_SCOPE, TIPC_CLUSTER_SCOPE, and TIPC_NODE_SCOPE.
  The meaning of v1, v2 and v3 depends on the value of addr_type:
    if addr_type is TIPC_ADDR_NAME:
        v1 is the server type
        v2 is the port identifier
        v3 is ignored
    if addr_type is TIPC_ADDR_NAMESEQ:
        v1 is the server type
        v2 is the lower port number
        v3 is the upper port number
    if addr_type is TIPC_ADDR_ID:
        v1 is the node
        v2 is the ref
        v3 is ignored


Local naming conventions:

- names starting with sock_ are socket object methods
- names starting with socket_ are module-level functions
- names starting with PySocket are exported through socketmodule.h

*/

#ifdef __APPLE__
	#include <AvailabilityMacros.h>
	/* for getaddrinfo thread safety test on old versions of OS X */
	#ifndef MAC_OS_X_VERSION_10_5
		#define MAC_OS_X_VERSION_10_5 1050
	#endif
	  /*
	   * inet_aton is not available on OSX 10.3, yet we want to use a binary
	   * that was build on 10.4 or later to work on that release, weak linking
	   * comes to the rescue.
	   */
	#pragma weak inet_aton
#endif

//#include "Python.h"
//#include "structmember.h"
//#include "timefuncs.h"
#include "../util.h"
#include <time.h> //time(nullptr)
#include <lua.hpp>
#include <string>
#include <cstring> //memcpy
using namespace std;
void createErrorObj(lua_State *, const string&, const string& sErrorMsg ="", int iCode=0);//声明
void PyErr_SetExcFromWindowsErr(lua_State* ,const string &, int);//声明
void PyErr_SetFromErrno(lua_State* L, const string &exc);
int __new__(lua_State* L);
//--从fileobject.h抠来的----------------------------------

/* A routine to check if a file descriptor can be select()-ed. */
#ifdef HAVE_SELECT
	#define _PyIsSelectable_fd(FD) (((FD) >= 0) && ((FD) < FD_SETSIZE))
#else
	#define _PyIsSelectable_fd(FD) (1)
#endif /* HAVE_SELECT */
//------------------------------------


#ifndef INVALID_SOCKET /* MS defines this */
	#define INVALID_SOCKET (-1)
#endif

#undef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))

/* Socket object documentation 
PyDoc_STRVAR(sock_doc,
"socket([family[, type[, proto]]]) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it defaults to AF_INET.  The type argument specifies\n\
whether this is a stream (SOCK_STREAM, this is the default)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.  Keyword arguments are accepted.\n\
\n\
A socket object represents one endpoint of a network connection.\n\
\n\
Methods of socket objects (keyword arguments not allowed):\n\
\n\
accept() -- accept a connection, returning new socket and client address\n\
bind(addr) -- bind the socket to a local address\n\
close() -- close the socket\n\
connect(addr) -- connect the socket to a remote address\n\
connect_ex(addr) -- connect, return an error code instead of an exception\n\
dup() -- return a new socket object identical to the current one [*]\n\
fileno() -- return underlying file descriptor\n\
getpeername() -- return remote address [*]\n\
getsockname() -- return local address\n\
getsockopt(level, optname[, buflen]) -- get socket options\n\
gettimeout() -- return timeout or None\n\
listen(n) -- start listening for incoming connections\n\
makefile([mode, [bufsize]]) -- return a file object for the socket [*]\n\
recv(buflen[, flags]) -- receive data\n\
recv_into(buffer[, nbytes[, flags]]) -- receive data (into a buffer)\n\
recvfrom(buflen[, flags]) -- receive data and sender\'s address\n\
recvfrom_into(buffer[, nbytes, [, flags])\n\
  -- receive data and sender\'s address (into a buffer)\n\
sendall(data[, flags]) -- send all data\n\
send(data[, flags]) -- send data, may not send all of it\n\
sendto(data[, flags], addr) -- send data to a given address\n\
setblocking(0 | 1) -- set or clear the blocking I/O flag\n\
setsockopt(level, optname, value) -- set socket options\n\
settimeout(None | float) -- set or clear the timeout\n\
shutdown(how) -- shut down traffic in one or both directions\n\
\n\
 [*] not available on all platforms!");*/

/* XXX This is a terrible mess of platform-dependent preprocessor hacks.
   I hope some day someone can clean this up please... */

/* Hacks for gethostbyname_r().  On some non-Linux platforms, the configure
   script doesn't get this right, so we hardcode some platform checks below.
   On the other hand, not all Linux versions agree, so there the settings
   computed by the configure script are needed! */

#ifndef linux
	#undef HAVE_GETHOSTBYNAME_R_3_ARG
	#undef HAVE_GETHOSTBYNAME_R_5_ARG
	#undef HAVE_GETHOSTBYNAME_R_6_ARG
#endif

#ifndef WITH_THREAD
	#undef HAVE_GETHOSTBYNAME_R
#endif

#ifdef HAVE_GETHOSTBYNAME_R
	#if defined(_AIX) && !defined(_LINUX_SOURCE_COMPAT) || defined(__osf__)
		#define HAVE_GETHOSTBYNAME_R_3_ARG
	#elif defined(__sun) || defined(__sgi)
		#define HAVE_GETHOSTBYNAME_R_5_ARG
	#elif defined(linux)
	/* Rely on the configure script */
	#elif defined(_LINUX_SOURCE_COMPAT) /* Linux compatibility on AIX */
		#define HAVE_GETHOSTBYNAME_R_6_ARG
	#else
		#undef HAVE_GETHOSTBYNAME_R
	#endif
#endif

#if !defined(HAVE_GETHOSTBYNAME_R) && defined(WITH_THREAD) && \
    !defined(MS_WINDOWS)
	#define USE_GETHOSTBYNAME_LOCK
#endif

/* To use __FreeBSD_version, __OpenBSD__, and __NetBSD_Version__ */
#ifdef HAVE_SYS_PARAM_H
	#include <sys/param.h>
#endif
/* On systems on which getaddrinfo() is believed to not be thread-safe,
   (this includes the getaddrinfo emulation) protect access with a lock.

   getaddrinfo is thread-safe on Mac OS X 10.5 and later. Originally it was
   a mix of code including an unsafe implementation from an old BSD's
   libresolv. In 10.5 Apple reimplemented it as a safe IPC call to the
   mDNSResponder process. 10.5 is the first be UNIX '03 certified, which
   includes the requirement that getaddrinfo be thread-safe. See issue #25924.

   It's thread-safe in OpenBSD starting with 5.4, released Nov 2013:
   http://www.openbsd.org/plus54.html

   It's thread-safe in NetBSD starting with 4.0, released Dec 2007:

http://cvsweb.netbsd.org/bsdweb.cgi/src/lib/libc/net/getaddrinfo.c.diff?r1=1.82&r2=1.83
*/
#if defined(WITH_THREAD) && ( \
    (defined(__APPLE__) && \
        MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5) || \
    (defined(__FreeBSD__) && __FreeBSD_version+0 < 503000) || \
    (defined(__OpenBSD__) && OpenBSD+0 < 201311) || \
    (defined(__NetBSD__) && __NetBSD_Version__+0 < 400000000) || \
    defined(__VMS) || !defined(HAVE_GETADDRINFO))
	#define USE_GETADDRINFO_LOCK
#endif

#ifdef USE_GETADDRINFO_LOCK
	#define ACQUIRE_GETADDRINFO_LOCK PyThread_acquire_lock(netdb_lock, 1);
	#define RELEASE_GETADDRINFO_LOCK PyThread_release_lock(netdb_lock);
#else
	#define ACQUIRE_GETADDRINFO_LOCK
	#define RELEASE_GETADDRINFO_LOCK
#endif

#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
	#include "pythread.h"
#endif

#if defined(PYCC_VACPP)
	#include <types.h>
	#include <io.h>
	#include <sys/ioctl.h>
	#include <utils.h>
	#include <ctype.h>
#endif

#if defined(__VMS)
	#include <ioctl.h>
#endif

#if defined(PYOS_OS2)
	#define  INCL_DOS
	#define  INCL_DOSERRORS
	#define  INCL_NOPMAPI
	#include <os2.h>
#endif

#if defined(__sgi) && _COMPILER_VERSION>700 && !_SGIAPI
/* make sure that the reentrant (gethostbyaddr_r etc)
   functions are declared correctly if compiling with
   MIPSPro 7.x in ANSI C mode (default) */

/* XXX Using _SGIAPI is the wrong thing,
   but I don't know what the right thing is. */
	#undef _SGIAPI /* to avoid warning */
	#define _SGIAPI 1

	#undef _XOPEN_SOURCE
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>

	#ifdef _SS_ALIGNSIZE
		#define HAVE_GETADDRINFO 1
		#define HAVE_GETNAMEINFO 1
	#endif

	#define HAVE_INET_PTON
	#include <netdb.h>
#endif

/* Irix 6.5 fails to define this variable at all. This is needed
   for both GCC and SGI's compiler. I'd say that the SGI headers
   are just busted. Same thing for Solaris. */
#if (defined(__sgi) || defined(sun)) && !defined(INET_ADDRSTRLEN)
	#define INET_ADDRSTRLEN 16
#endif

/* Generic includes */
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif

/* Generic socket object definitions and includes */
#define PySocket_BUILDING_SOCKET
#include "socketmodule.h"

/* Addressing includes */

#ifndef MS_WINDOWS

	/* Non-MS WINDOWS includes */
	#include <netdb.h>

	/* Headers needed for inet_ntoa() and inet_addr() */
	#ifdef __BEOS__
		#include <net/netdb.h>
	#elif defined(PYOS_OS2) && defined(PYCC_VACPP)
		#include <netdb.h>
		typedef size_t socklen_t;
	#else
		#include <arpa/inet.h>
	#endif

	#ifndef RISCOS
		#include <fcntl.h>
	#else
		#include <sys/ioctl.h>
		#include <socklib.h>
		#define NO_DUP
		int h_errno; /* not used */
		#define INET_ADDRSTRLEN 16
	#endif
#else

	/* MS_WINDOWS includes */
	#ifdef HAVE_FCNTL_H
		#include <fcntl.h>
	#endif

#endif

#include <stddef.h>

#ifndef offsetof
	#define offsetof(type, member) ((size_t)(&((type *)0)->member))
#endif

#ifndef O_NONBLOCK
	#define O_NONBLOCK O_NDELAY
#endif

/* include Python's addrinfo.h unless it causes trouble */
#if defined(__sgi) && _COMPILER_VERSION>700 && defined(_SS_ALIGNSIZE)
  /* Do not include addinfo.h on some newer IRIX versions.
   * _SS_ALIGNSIZE is defined in sys/socket.h by 6.5.21,
   * for example, but not by 6.5.10.
   */
#elif defined(_MSC_VER) && _MSC_VER>1201
  /* Do not include addrinfo.h for MSVC7 or greater. 'addrinfo' and
   * EAI_* constants are defined in (the already included) ws2tcpip.h.
   */
#else
	#include "addrinfo.h"
#endif

//#ifndef HAVE_INET_PTON
//	#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_LONGHORN)
//		int inet_pton(int af, const char *src, void *dst);
//		const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
//	#endif
//#endif

#ifdef __APPLE__
	/* On OS X, getaddrinfo returns no error indication of lookup
	   failure, so we must use the emulation instead of the libinfo
	   implementation. Unfortunately, performing an autoconf test
	   for this bug would require DNS access for the machine performing
	   the configuration, which is not acceptable. Therefore, we
	   determine the bug just by checking for __APPLE__. If this bug
	   gets ever fixed, perhaps checking for sys/version.h would be
	   appropriate, which is 10/0 on the system with the bug. */
	#ifndef HAVE_GETNAMEINFO
		/* This bug seems to be fixed in Jaguar. Ths easiest way I could
		   Find to check for Jaguar is that it has getnameinfo(), which
		   older releases don't have */
		#undef HAVE_GETADDRINFO
	#endif

	#ifdef HAVE_INET_ATON
		#define USE_INET_ATON_WEAKLINK
	#endif
#endif

/* I know this is a bad practice, but it is the easiest... */
#if !defined(HAVE_GETADDRINFO)
	/* avoid clashes with the C library definition of the symbol. */
	#define getaddrinfo fake_getaddrinfo
	#define gai_strerror fake_gai_strerror
	#define freeaddrinfo fake_freeaddrinfo
	#include "getaddrinfo.c"
#endif

#if !defined(HAVE_GETNAMEINFO)
	#define getnameinfo fake_getnameinfo
	#include "getnameinfo.c"
#endif

#if defined(MS_WINDOWS) || defined(__BEOS__)
	/* BeOS suffers from the same socket dichotomy as Win32... - [cjh] */
	/* seem to be a few differences in the API */
	#define SOCKETCLOSE closesocket
	#define NO_DUP /* Actually it exists on NT 3.5, but what the heck... */
#endif

#ifdef MS_WIN32
	#define EAFNOSUPPORT WSAEAFNOSUPPORT
	#define snprintf _snprintf
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
	#define SOCKETCLOSE soclose
	#define NO_DUP /* Sockets are Not Actual File Handles under OS/2 */
#endif

#ifndef SOCKETCLOSE
	#define SOCKETCLOSE close
#endif

#if (defined(HAVE_BLUETOOTH_H) || defined(HAVE_BLUETOOTH_BLUETOOTH_H)) && !defined(__NetBSD__) && !defined(__DragonFly__)
	#define USE_BLUETOOTH 1
	#if defined(__FreeBSD__)
		#define BTPROTO_L2CAP BLUETOOTH_PROTO_L2CAP
		#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
		#define BTPROTO_HCI BLUETOOTH_PROTO_HCI
		#define SOL_HCI SOL_HCI_RAW
		#define HCI_FILTER SO_HCI_RAW_FILTER
		#define sockaddr_l2 sockaddr_l2cap
		#define sockaddr_rc sockaddr_rfcomm
		#define hci_dev hci_node
		#define _BT_L2_MEMB(sa, memb) ((sa)->l2cap_##memb)
		#define _BT_RC_MEMB(sa, memb) ((sa)->rfcomm_##memb)
		#define _BT_HCI_MEMB(sa, memb) ((sa)->hci_##memb)
	#elif defined(__NetBSD__) || defined(__DragonFly__)
		#define sockaddr_l2 sockaddr_bt
		#define sockaddr_rc sockaddr_bt
		#define sockaddr_hci sockaddr_bt
		#define sockaddr_sco sockaddr_bt
		#define SOL_HCI BTPROTO_HCI
		#define HCI_DATA_DIR SO_HCI_DIRECTION
		#define _BT_L2_MEMB(sa, memb) ((sa)->bt_##memb)
		#define _BT_RC_MEMB(sa, memb) ((sa)->bt_##memb)
		#define _BT_HCI_MEMB(sa, memb) ((sa)->bt_##memb)
		#define _BT_SCO_MEMB(sa, memb) ((sa)->bt_##memb)
	#else
		#define _BT_L2_MEMB(sa, memb) ((sa)->l2_##memb)
		#define _BT_RC_MEMB(sa, memb) ((sa)->rc_##memb)
		#define _BT_HCI_MEMB(sa, memb) ((sa)->hci_##memb)
		#define _BT_SCO_MEMB(sa, memb) ((sa)->sco_##memb)
	#endif
#endif

#ifdef __VMS
	/* TCP/IP Services for VMS uses a maximum send/recv buffer length */
	#define SEGMENT_SIZE (32 * 1024 -1)
#endif

#define SAS2SA(x)       ((struct sockaddr *)(x))

/*
 * Constants for getnameinfo()
 */
#if !defined(NI_MAXHOST)
	#define NI_MAXHOST 1025
#endif
#if !defined(NI_MAXSERV)
	#define NI_MAXSERV 32
#endif

/* XXX There's a problem here: *static* functions are not supposed to have
   a Py prefix (or use CapitalizedWords).  Later... */

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */
//static PyObject *socket_error;
//static PyObject *socket_herror;
//static PyObject *socket_gaierror;
//static PyObject *socket_timeout;

#ifdef RISCOS
	/* Global variable which is !=0 if Python is running in a RISC OS taskwindow */
	static int taskwindow;
#endif

/* A forward reference to the socket type object.
   The sock_type variable contains pointers to various functions,
   some of which call new_sockobject(), which uses sock_type, so
   there has to be a circular reference. */
//static PyTypeObject sock_type;

#if defined(HAVE_POLL_H)
	#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
	#include <sys/poll.h>
#endif

#ifdef HAVE_POLL
	/* Instead of select(), we'll use poll() since poll() works on any fd. */
	#define IS_SELECTABLE(s) 1
	/* Can we call select() with this socket without a buffer overrun? */
#else
	/* If there's no timeout left, we don't have to call select, so it's a safe,
	 * little white lie. */
	#define IS_SELECTABLE(s) (_PyIsSelectable_fd((s)->sock_fd) || (s)->sock_timeout <= 0.0)
#endif

static void select_error(lua_State * L){
	createErrorObj(L,"cSocketError", "unable to select on socket");
}

#ifdef MS_WINDOWS
	#ifndef WSAEAGAIN
		#define WSAEAGAIN WSAEWOULDBLOCK
	#endif
	#define CHECK_ERRNO(expected) \
    (WSAGetLastError() == WSA ##expected)
#else
	#define CHECK_ERRNO(expected) \
    (errno == expected)
#endif

/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static bool set_error(lua_State* L){
#ifdef MS_WINDOWS
    int err_no = WSAGetLastError();
    /* PyErr_SetExcFromWindowsErr() invokes FormatMessage() which
       recognizes the error codes used by both GetLastError() and
       WSAGetLastError */
	if (err_no) {
		PyErr_SetExcFromWindowsErr(L, "cSocketError", err_no);
		return false;
	}

#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
    if (sock_errno() != NO_ERROR) {
        APIRET rc;
        ULONG  msglen;
        char outbuf[100];
        int myerrorcode = sock_errno();

        /* Retrieve socket-related error message from MPTN.MSG file */
        rc = DosGetMessage(NULL, 0, outbuf, sizeof(outbuf),
                           myerrorcode - SOCBASEERR + 26,
                           "mptn.msg",
                           &msglen);
        if (rc == NO_ERROR) {
            PyObject *v;

            /* OS/2 doesn't guarantee a terminator */
            outbuf[msglen] = '\0';
            if (strlen(outbuf) > 0) {
                /* If non-empty msg, trim CRLF */
                char *lastc = &outbuf[ strlen(outbuf)-1 ];
                while (lastc > outbuf &&
                       isspace(Py_CHARMASK(*lastc))) {
                    /* Trim trailing whitespace (CRLF) */
                    *lastc-- = '\0';
                }
            }
            v = Py_BuildValue("(is)", myerrorcode, outbuf);
            if (v != NULL) {
                PyErr_SetObject(socket_error, v);
                Py_DECREF(v);
            }
            return NULL;
        }
    }
#endif

#if defined(RISCOS)
    if (_inet_error.errnum != NULL) {
        PyObject *v;
        v = Py_BuildValue("(is)", errno, _inet_err());
        if (v != NULL) {
            PyErr_SetObject(socket_error, v);
            Py_DECREF(v);
        }
        return NULL;
    }
#endif
    PyErr_SetFromErrno(L,"cSocketError");
	return false;
}

//
//static PyObject *
//set_herror(int h_error)
//{
//    PyObject *v;
//
//#ifdef HAVE_HSTRERROR
//    v = Py_BuildValue("(is)", h_error, (char *)hstrerror(h_error));
//#else
//    v = Py_BuildValue("(is)", h_error, "host not found");
//#endif
//    if (v != NULL) {
//        PyErr_SetObject(socket_herror, v);
//        Py_DECREF(v);
//    }
//
//    return NULL;
//}

//在栈顶放上错误对象(目前只是一个字串对象)
static void set_gaierror(lua_State* L, int error) {
	//PyObject *v;
#ifdef EAI_SYSTEM
	/* EAI_SYSTEM is not available on Windows XP. */
	if (error == EAI_SYSTEM) {
		set_error(L);
		return;
	}
#endif

#ifdef HAVE_GAI_STRERROR
	createErrorObj(L, "cSocketGaiError", gai_strerror(error),error);
#else
	createErrorObj(L, "cSocketGaiError", "getaddrinfo failed", error);
#endif
}

//#ifdef __VMS
//	/* Function to send in segments */
//	static int
//	sendsegmented(int sock_fd, char *buf, int len, int flags)
//	{
//		int n = 0;
//		int remaining = len;
//
//		while (remaining > 0) {
//			unsigned int segment;
//
//			segment = (remaining >= SEGMENT_SIZE ? SEGMENT_SIZE : remaining);
//			n = send(sock_fd, buf, segment, flags);
//			if (n < 0) {
//				return n;
//			}
//			remaining -= segment;
//			buf += segment;
//		} /* end while */
//
//		return len;
//	}
//#endif

/* Function to perform the setting of socket blocking mode
   internally. block = (1 | 0). */
static int
internal_setblocking(PySocketSockObject *s, int block)
{
#ifndef RISCOS
	#ifndef MS_WINDOWS
		int delay_flag;
	#endif
#endif

#ifdef __BEOS__
    block = !block;
    setsockopt(s->sock_fd, SOL_SOCKET, SO_NONBLOCK,
               (void *)(&block), sizeof(int));
#else
	#ifndef RISCOS
		#ifndef MS_WINDOWS
			#if defined(PYOS_OS2) && !defined(PYCC_GCC)
				block = !block;
				ioctl(s->sock_fd, FIONBIO, (caddr_t)&block, sizeof(block));
			#elif defined(__VMS)
				block = !block;
				ioctl(s->sock_fd, FIONBIO, (unsigned int *)&block);
			#else  /* !PYOS_OS2 && !__VMS */
				delay_flag = fcntl(s->sock_fd, F_GETFL, 0);
				if (block)
					delay_flag &= (~O_NONBLOCK);
				else
					delay_flag |= O_NONBLOCK;
				fcntl(s->sock_fd, F_SETFL, delay_flag);
			#endif /* !PYOS_OS2 */
		#else /* MS_WINDOWS */
			block = !block;
			ioctlsocket(s->sock_fd, FIONBIO, (u_long*)&block);
		#endif /* MS_WINDOWS */
	#else /* RISCOS */
		block = !block;
		socketioctl(s->sock_fd, FIONBIO, (u_long*)&block);
	#endif /* RISCOS */
#endif /* __BEOS__ */
    /* Since these don't return anything */
    return 1;
}

/* Do a select()/poll() on the socket, if necessary (sock_timeout > 0).
   The argument writing indicates the direction.
   This does not raise an exception; we'll let our caller do that
   after they've reacquired the interpreter lock.
   Returns 1 on timeout, -1 on error, 0 otherwise. */
static int
internal_select_ex(PySocketSockObject *s, int writing, double interval)
{
    int n;

    /* Nothing to do unless we're in timeout mode (not non-blocking) */
    if (s->sock_timeout <= 0.0)
        return 0;

    /* Guard against closed socket */
    if (s->sock_fd < 0)
        return 0;

    /* Handling this condition here simplifies the select loops */
    if (interval < 0.0)
        return 1;

    /* Prefer poll, if available, since you can poll() any fd
     * which can't be done with select(). */
#ifdef HAVE_POLL
    {
        struct pollfd pollfd;
        int timeout;

        pollfd.fd = s->sock_fd;
        pollfd.events = writing ? POLLOUT : POLLIN;

        /* s->sock_timeout is in seconds, timeout in ms */
        timeout = (int)(interval * 1000 + 0.5);
        n = poll(&pollfd, 1, timeout);
    }
#else
    {
        /* Construct the arguments to select */
        fd_set fds;
        struct timeval tv;
        tv.tv_sec = (int)interval;
        tv.tv_usec = (int)((interval - tv.tv_sec) * 1e6);
        FD_ZERO(&fds);
        FD_SET(s->sock_fd, &fds);

        /* See if the socket is ready */
        if (writing)
            n = select(static_cast<int>(s->sock_fd+1), NULL, &fds, NULL, &tv);
        else
            n = select(static_cast<int>(s->sock_fd+1), &fds, NULL, NULL, &tv);
    }
#endif

    if (n < 0)
        return -1;
    if (n == 0)
        return 1;
    return 0;
}

static int
internal_select(PySocketSockObject *s, int writing)
{
    return internal_select_ex(s, writing, s->sock_timeout);
}

/*
   Two macros for automatic retry of select() in case of false positives
   (for example, select() could indicate a socket is ready for reading
    but the data then discarded by the OS because of a wrong checksum).
   Here is an example of use:

    BEGIN_SELECT_LOOP(s)
    //,.Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout)
        outlen = recv(s->sock_fd, cbuf, len, flags);
    //,.Py_END_ALLOW_THREADS
    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return -1;
    }
    END_SELECT_LOOP(s)
*/
#define BEGIN_SELECT_LOOP(s) \
    { \
        double deadline = 0, interval = s->sock_timeout; \
        int has_timeout = s->sock_timeout > 0.0; \
        if (has_timeout) { \
            deadline = time(nullptr) + s->sock_timeout; \
        } \
        while (1) { \
            errno = 0;

#define END_SELECT_LOOP(s) \
            if (!has_timeout || \
                (!CHECK_ERRNO(EWOULDBLOCK) && !CHECK_ERRNO(EAGAIN))) \
                break; \
            interval = deadline - time(nullptr); \
        } \
    }

/* Initialize a new socket object. */

static double defaulttimeout = -1.0; /* Default timeout for new sockets */


//__init__也调用到这里
//PyMODINIT_FUNC
void init_sockobject(PySocketSockObject *s, SOCKET_T fd, int family, int type, int proto)
{
#ifdef RISCOS
    int block = 1;
#endif
    s->sock_fd = fd;
    s->sock_family = family;
    s->sock_type = type;
    s->sock_proto = proto;
    s->sock_timeout = defaulttimeout;

    s->errorhandler = &set_error;

    if (defaulttimeout >= 0.0)
        internal_setblocking(s, 0);

#ifdef RISCOS
    if (taskwindow)
        socketioctl(s->sock_fd, 0x80046679, (u_long*)&block);
#endif
}


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

//内部接口,accept和dup会调用到这里
//userdata压栈,失败则error对象压栈
static bool new_sockobject(lua_State* L,int iSelfIndex,SOCKET_T fd, int family, int type, int proto){
	auto const CLASS = lua_upvalueindex(1);
	if (LUA_TNIL == luaL_getmetafield(L, CLASS, "__call")) {//[-0, +(0|1), e]
		lua_pushstring(L, "类没有元表或元表没有__call方法");
		return false;
	}
	lua_pushvalue(L, CLASS);//参数1,cls
	lua_pushinteger(L, family);
	lua_pushinteger(L, type);
	lua_pushinteger(L, proto);
	lua_pushinteger(L, fd);
	lua_call(L, 5, 1);
	return true;
	
}


/* Lock to allow python interpreter to continue, but only allow one
   thread to be in gethostbyname or getaddrinfo */
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
	static PyThread_type_lock netdb_lock;
#endif


/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (IPv4 should be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int setipaddr(lua_State * L,const char *name, struct sockaddr *addr_ret, size_t addr_ret_size, int af){
    struct addrinfo hints, *res;
    int error;
    int d1, d2, d3, d4;
    char ch;

    memset((void *) addr_ret, '\0', sizeof(*addr_ret));
    if (name[0] == '\0') {
        int siz;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = af;
        hints.ai_socktype = SOCK_DGRAM;         /*dummy*/
        hints.ai_flags = AI_PASSIVE;
        //,.Py_BEGIN_ALLOW_THREADS
        ACQUIRE_GETADDRINFO_LOCK
        error = getaddrinfo(NULL, "0", &hints, &res);
        //,.Py_END_ALLOW_THREADS
        /* We assume that those thread-unsafe getaddrinfo() versions
           *are* safe regarding their return value, ie. that a
           subsequent call to getaddrinfo() does not destroy the
           outcome of the first call. */
        RELEASE_GETADDRINFO_LOCK
        if (error) {
            set_gaierror(L,error);			
            return -1;
        }
        switch (res->ai_family) {
        case AF_INET:
            siz = 4;
            break;
#ifdef ENABLE_IPV6
        case AF_INET6:
            siz = 16;
            break;
#endif
        default:
            freeaddrinfo(res);            
			createErrorObj(L, "cSocketError", "unsupported address family");
            return -1;
        }
        if (res->ai_next) {
            freeaddrinfo(res);            
			createErrorObj(L, "cSocketError", "wildcard resolved to multiple address");
            return -1;
        }
        if (res->ai_addrlen < addr_ret_size)
            addr_ret_size = res->ai_addrlen;
        memcpy(addr_ret, res->ai_addr, addr_ret_size);
        freeaddrinfo(res);
        return siz;
    }
    if (name[0] == '<' && strcmp(name, "<broadcast>") == 0) {
        struct sockaddr_in *sin;
        if (af != AF_INET && af != AF_UNSPEC) {            
			createErrorObj(L, "cSocketError", "address family mismatched");
            return -1;
        }
        sin = (struct sockaddr_in *)addr_ret;
        memset((void *) sin, '\0', sizeof(*sin));
        sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
        sin->sin_len = sizeof(*sin);
#endif
        sin->sin_addr.s_addr = INADDR_BROADCAST;
        return sizeof(sin->sin_addr);
    }
    if (sscanf(name, "%d.%d.%d.%d%c", &d1, &d2, &d3, &d4, &ch) == 4 &&
        0 <= d1 && d1 <= 255 && 0 <= d2 && d2 <= 255 &&
        0 <= d3 && d3 <= 255 && 0 <= d4 && d4 <= 255) {
        struct sockaddr_in *sin;
        sin = (struct sockaddr_in *)addr_ret;
        sin->sin_addr.s_addr = htonl(
            ((long) d1 << 24) | ((long) d2 << 16) |
            ((long) d3 << 8) | ((long) d4 << 0));
        sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
        sin->sin_len = sizeof(*sin);
#endif
        return 4;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    //,.Py_BEGIN_ALLOW_THREADS
    ACQUIRE_GETADDRINFO_LOCK
    error = getaddrinfo(name, NULL, &hints, &res);
#if defined(__digital__) && defined(__unix__)
    if (error == EAI_NONAME && af == AF_UNSPEC) {
        /* On Tru64 V5.1, numeric-to-addr conversion fails
           if no address family is given. Assume IPv4 for now.*/
        hints.ai_family = AF_INET;
        error = getaddrinfo(name, NULL, &hints, &res);
    }
#endif
    //,.Py_END_ALLOW_THREADS
    RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
    if (error) {
        set_gaierror(L,error);
        return -1;
    }
    if (res->ai_addrlen < addr_ret_size)
        addr_ret_size = res->ai_addrlen;
    memcpy((char *) addr_ret, res->ai_addr, addr_ret_size);
    freeaddrinfo(res);
    switch (addr_ret->sa_family) {
    case AF_INET:
        return 4;
#ifdef ENABLE_IPV6
    case AF_INET6:
        return 16;
#endif
    default:
		createErrorObj(L, "cSocketError", "unknown address family");
        return -1;
    }
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

   //可能压栈错误对象
static bool makeipaddr(lua_State* L,struct sockaddr *addr, int addrlen)
{
    char buf[NI_MAXHOST];
    auto error = getnameinfo(addr, addrlen, buf, sizeof(buf), NULL, 0,NI_NUMERICHOST);
    if (error) {
        set_gaierror(L,error);
        return false;
    }
	//PyString_FromString(buf);
	lua_pushstring(L, buf);//压栈结果
	return true;
}


//#ifdef USE_BLUETOOTH
///* Convert a string representation of a Bluetooth address into a numeric
//   address.  Returns the length (6), or raises an exception and returns -1 if
//   an error occurred. */
//
//static int
//setbdaddr(char *name, bdaddr_t *bdaddr)
//{
//    unsigned int b0, b1, b2, b3, b4, b5;
//    char ch;
//    int n;
//
//    n = sscanf(name, "%X:%X:%X:%X:%X:%X%c",
//               &b5, &b4, &b3, &b2, &b1, &b0, &ch);
//    if (n == 6 && (b0 | b1 | b2 | b3 | b4 | b5) < 256) {
//        bdaddr->b[0] = b0;
//        bdaddr->b[1] = b1;
//        bdaddr->b[2] = b2;
//        bdaddr->b[3] = b3;
//        bdaddr->b[4] = b4;
//        bdaddr->b[5] = b5;
//        return 6;
//    } else {
//        PyErr_SetString(socket_error, "bad bluetooth address");
//        return -1;
//    }
//}
//
///* Create a string representation of the Bluetooth address.  This is always a
//   string of the form 'XX:XX:XX:XX:XX:XX' where XX is a two digit hexadecimal
//   value (zero padded if necessary). */
//
//static PyObject *
//makebdaddr(bdaddr_t *bdaddr)
//{
//    char buf[(6 * 2) + 5 + 1];
//
//    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
//        bdaddr->b[5], bdaddr->b[4], bdaddr->b[3],
//        bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);
//    return PyString_FromString(buf);
//}
//#endif


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

/*ARGSUSED*/
//会压栈,成功压正确的结果,失败压的是错误对象
static bool makesockaddr(lua_State* L,int sockfd, struct sockaddr *addr, int addrlen, int proto)
{
    if (addrlen == 0) {
        /* No address -- may be recvfrom() from known socket */
		lua_pushstring(L, "no address");//随便压点东西吧
		return true;// Py_None;
    }

#ifdef __BEOS__
    /* XXX: BeOS version of accept() doesn't set family correctly */
    addr->sa_family = AF_INET;
#endif

    switch (addr->sa_family) {

    case AF_INET:
    {
        struct sockaddr_in *a;
        auto ok = makeipaddr(L,addr, sizeof(*a));        
		if (!ok) {
			return false;
		}
		const auto ADDR_OBJ_INDEX = lua_gettop(L);
        a = (struct sockaddr_in *)addr;
		lua_createtable(L, 2, 0);
		const auto TABLE_INDEX = lua_gettop(L);

		lua_pushinteger(L, 1);//index1
		lua_pushvalue(L, ADDR_OBJ_INDEX);//value1,即是addrobj
		lua_settable(L, TABLE_INDEX);//[-2, +0, e]

		lua_pushinteger(L, 2);//index2
		lua_pushinteger(L, ntohs(a->sin_port));//value2
		lua_settable(L, TABLE_INDEX);//[-2, +0, e]

		lua_remove(L, ADDR_OBJ_INDEX);//即是addrobj,栈上不能有过多的
		return true;
    }

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        struct sockaddr_un *a = (struct sockaddr_un *) addr;
#ifdef linux
        if (a->sun_path[0] == 0) {  /* Linux abstract namespace */
            addrlen -= offsetof(struct sockaddr_un, sun_path);
            return PyString_FromStringAndSize(a->sun_path,
                                              addrlen);
        }
        else
#endif /* linux */
        {
            /* regular NULL-terminated string */
            return PyString_FromString(a->sun_path);
        }
    }
#endif /* AF_UNIX */

#if defined(AF_NETLINK)
       case AF_NETLINK:
       {
           struct sockaddr_nl *a = (struct sockaddr_nl *) addr;
           return Py_BuildValue("II", a->nl_pid, a->nl_groups);
       }
#endif /* AF_NETLINK */

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        struct sockaddr_in6 *a;
        auto ok = makeipaddr(L,addr, sizeof(*a));        
        if (ok) {
            a = (struct sockaddr_in6 *)addr;			
            //ret = Py_BuildValue("OiII",addrobj,ntohs(a->sin6_port),ntohl(a->sin6_flowinfo),a->sin6_scope_id);           
			lua_createtable(L, 4, 0);

			lua_pushinteger(L, 1);//index1
			lua_pushvalue(L, -3);//value1,即是addrobj
			lua_settable(L, -3);//[-2, +0, e]

			lua_pushinteger(L, 2);//index2
			lua_pushinteger(L, ntohs(a->sin6_port));//value2
			lua_settable(L, -3);//[-2, +0, e]

			lua_pushinteger(L, 3);//index3
			lua_pushinteger(L, ntohl(a->sin6_flowinfo));//value3
			lua_settable(L, -3);//[-2, +0, e]

			lua_pushinteger(L, 4);//index4
			lua_pushinteger(L, a->sin6_scope_id);//value4
			lua_settable(L, -3);//[-2, +0, e]
        }
        return ok;
    }
#endif

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
        switch (proto) {

        case BTPROTO_L2CAP:
        {
            struct sockaddr_l2 *a = (struct sockaddr_l2 *) addr;
            PyObject *addrobj = makebdaddr(&_BT_L2_MEMB(a, bdaddr));
            PyObject *ret = NULL;
            if (addrobj) {
                ret = Py_BuildValue("Oi",
                                    addrobj,
                                    _BT_L2_MEMB(a, psm));
                Py_DECREF(addrobj);
            }
            return ret;
        }

        case BTPROTO_RFCOMM:
        {
            struct sockaddr_rc *a = (struct sockaddr_rc *) addr;
            PyObject *addrobj = makebdaddr(&_BT_RC_MEMB(a, bdaddr));
            PyObject *ret = NULL;
            if (addrobj) {
                ret = Py_BuildValue("Oi",
                                    addrobj,
                                    _BT_RC_MEMB(a, channel));
                Py_DECREF(addrobj);
            }
            return ret;
        }

        case BTPROTO_HCI:
        {
            struct sockaddr_hci *a = (struct sockaddr_hci *) addr;
#if defined(__NetBSD__) || defined(__DragonFly__)
            return makebdaddr(&_BT_HCI_MEMB(a, bdaddr));
#else
            PyObject *ret = NULL;
            ret = Py_BuildValue("i", _BT_HCI_MEMB(a, dev));
            return ret;
#endif
        }

#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
        {
            struct sockaddr_sco *a = (struct sockaddr_sco *) addr;
            return makebdaddr(&_BT_SCO_MEMB(a, bdaddr));
        }
#endif

        default:
            PyErr_SetString(PyExc_ValueError,
                            "Unknown Bluetooth protocol");
            return NULL;
        }
#endif

#if defined(HAVE_NETPACKET_PACKET_H) && defined(SIOCGIFNAME)
    case AF_PACKET:
    {
        struct sockaddr_ll *a = (struct sockaddr_ll *)addr;
        char *ifname = "";
        struct ifreq ifr;
        /* need to look up interface name give index */
        if (a->sll_ifindex) {
            ifr.ifr_ifindex = a->sll_ifindex;
            if (ioctl(sockfd, SIOCGIFNAME, &ifr) == 0)
                ifname = ifr.ifr_name;
        }
        return Py_BuildValue("shbhs#",
                             ifname,
                             ntohs(a->sll_protocol),
                             a->sll_pkttype,
                             a->sll_hatype,
                             a->sll_addr,
                             a->sll_halen);
    }
#endif

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        struct sockaddr_tipc *a = (struct sockaddr_tipc *) addr;
        if (a->addrtype == TIPC_ADDR_NAMESEQ) {
            return Py_BuildValue("IIIII",
                            a->addrtype,
                            a->addr.nameseq.type,
                            a->addr.nameseq.lower,
                            a->addr.nameseq.upper,
                            a->scope);
        } else if (a->addrtype == TIPC_ADDR_NAME) {
            return Py_BuildValue("IIIII",
                            a->addrtype,
                            a->addr.name.name.type,
                            a->addr.name.name.instance,
                            a->addr.name.name.instance,
                            a->scope);
        } else if (a->addrtype == TIPC_ADDR_ID) {
            return Py_BuildValue("IIIII",
                            a->addrtype,
                            a->addr.id.node,
                            a->addr.id.ref,
                            0,
                            a->scope);
        } else {
            PyErr_SetString(PyExc_ValueError,
                            "Invalid address type");
            return NULL;
        }
    }
#endif
    /* More cases here... */
    default:
        /* If we don't know the address family, don't raise an
           exception -- return it as a tuple. */		
        //return Py_BuildValue("is#",addr->sa_family,addr->sa_data,sizeof(addr->sa_data));
		lua_createtable(L, 3, 0);

		lua_pushinteger(L, 1);//index1
		lua_pushinteger(L, addr->sa_family);//value1
		lua_settable(L, -3);//[-2, +0, e]

		lua_pushinteger(L, 2);//index2
		lua_pushstring(L, addr->sa_data);//value2
		lua_settable(L, -3);//[-2, +0, e]

		lua_pushinteger(L, 3);//index3
		lua_pushinteger(L, sizeof(addr->sa_data));//value3
		lua_settable(L, -3);//[-2, +0, e]
		return true;
    }
}


/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

//static int getsockaddrarg(PySocketSockObject *s, PyObject *args, struct sockaddr *addr_ret, int *len_ret)
//请保持栈平衡
//失败返回0 ,并压栈错误对象
static int getsockaddrarg(lua_State* L,int addressIdx, PySocketSockObject *s, struct sockaddr *addr_ret, int *len_ret){
	//auto top = lua_gettop(L);
	//auto func = [&]{lua_pop(L,lua_gettop(L)-top);};
	//ON_SCOPE_EXIT(func);

    switch (s->sock_family) {

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        struct sockaddr_un* addr;
        char *path;
        int len;
        if (!PyArg_Parse(args, "t#", &path, &len))
            return 0;

        addr = (struct sockaddr_un*)addr_ret;
#ifdef linux
        if (len > 0 && path[0] == 0) {
            /* Linux abstract namespace extension */
            if (len > sizeof addr->sun_path) {                
				createErrorObj(L, "cSocketError", "AF_UNIX path too long");
                return 0;
            }
        }
        else
#endif /* linux */
        {
            /* regular NULL-terminated string */
            if (len >= sizeof addr->sun_path) {
                PyErr_SetString(socket_error,
                                "AF_UNIX path too long");
                return 0;
            }
            addr->sun_path[len] = 0;
        }
        addr->sun_family = s->sock_family;
        memcpy(addr->sun_path, path, len);
#if defined(PYOS_OS2)
        *len_ret = sizeof(*addr);
#else
        *len_ret = len + offsetof(struct sockaddr_un, sun_path);
#endif
        return 1;
    }
#endif /* AF_UNIX */

#if defined(AF_NETLINK)
    case AF_NETLINK:
    {
        struct sockaddr_nl* addr;
        int pid, groups;
        addr = (struct sockaddr_nl *)addr_ret;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_NETLINK address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "II:getsockaddrarg", &pid, &groups))
            return 0;
        addr->nl_family = AF_NETLINK;
        addr->nl_pid = pid;
        addr->nl_groups = groups;
        *len_ret = sizeof(*addr);
        return 1;
    }
#endif

    case AF_INET:
    {
		if(!lua_istable(L,addressIdx)){
			luaL_argerror(L, addressIdx, "AF_INET address must be table");
            return 0;
        }
		lua_pushinteger(L, 1);//key1
		lua_gettable(L, addressIdx);//[-1, +1, e]
		auto host = luaL_checkstring(L, -1);//[-0, +0, v]

		lua_pushinteger(L, 2);//key2
		lua_gettable(L, addressIdx);//[-1, +1, e]
		auto port = luaL_checkinteger(L, -1);//[-0, +0, v]

        auto addr=(struct sockaddr_in*)addr_ret;
        auto result = setipaddr(L,host, (struct sockaddr *)addr,
                           sizeof(*addr),  AF_INET);
        
        if (result < 0)
            return 0;
        if (port < 0 || port > 0xffff) {
            lua_pushstring(L,"getsockaddrarg: port must be 0-65535.");
            return 0;
        }
        addr->sin_family = AF_INET;
        addr->sin_port = htons((short)port);
        *len_ret = sizeof *addr;
        return 1;
    }

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        struct sockaddr_in6* addr;
        const char *host;
        //int port, result;
        int flowinfo, scope_id;
        flowinfo = scope_id = 0;


		if (!lua_istable(L, addressIdx)) {
			lua_pushstring(L, "AF_INET6 address must be table");
			return 0;
		}
		lua_pushinteger(L, 1);//key1
		lua_gettable(L, addressIdx);//[-1, +1, e]
		host = luaL_checkstring(L, -1);//[-0, +0, v]

		lua_pushinteger(L, 2);//key2
		lua_gettable(L, addressIdx);//[-1, +1, e]
		auto port = static_cast<int>(luaL_checkinteger(L, -1));//[-0, +0, v]

		lua_pushinteger(L, 3);//key3
		lua_gettable(L, addressIdx);//[-1, +1, e]
		flowinfo = static_cast<int>(luaL_checkinteger(L, -1));//[-0, +0, v]

		lua_pushinteger(L, 4);//key4
		lua_gettable(L, addressIdx);//[-1, +1, e]
		scope_id = static_cast<int>(luaL_checkinteger(L, -1));//[-0, +0, v]

        addr = (struct sockaddr_in6*)addr_ret;
        auto result = setipaddr(L,host, (struct sockaddr *)addr,
                           sizeof(*addr), AF_INET6);
        //PyMem_Free(host);
        if (result < 0)
            return 0;
        if (port < 0 || port > 0xffff) {           
			lua_pushstring(L, "getsockaddrarg: port must be 0-65535.");//PyExc_OverflowError
            return 0;
        }
        if (flowinfo > 0xfffff) {            
			lua_pushstring(L, "getsockaddrarg: flowinfo must be 0-1048575.");//PyExc_OverflowError
            return 0;
        }
        addr->sin6_family = s->sock_family;
        addr->sin6_port = htons((short)port);
        addr->sin6_flowinfo = htonl(flowinfo);
        addr->sin6_scope_id = scope_id;
        *len_ret = sizeof *addr;
        return 1;
    }
#endif

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
    {
        switch (s->sock_proto) {
        case BTPROTO_L2CAP:
        {
            struct sockaddr_l2 *addr;
            char *straddr;

            addr = (struct sockaddr_l2 *)addr_ret;
            memset(addr, 0, sizeof(struct sockaddr_l2));
            _BT_L2_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "si", &straddr,
                                  &_BT_L2_MEMB(addr, psm))) {
                PyErr_SetString(socket_error, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_L2_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
        case BTPROTO_RFCOMM:
        {
            struct sockaddr_rc *addr;
            char *straddr;

            addr = (struct sockaddr_rc *)addr_ret;
            _BT_RC_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "si", &straddr,
                                  &_BT_RC_MEMB(addr, channel))) {
                PyErr_SetString(socket_error, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_RC_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
        case BTPROTO_HCI:
        {
            struct sockaddr_hci *addr = (struct sockaddr_hci *)addr_ret;
#if defined(__NetBSD__) || defined(__DragonFly__)
			char *straddr = PyBytes_AS_STRING(args);

			_BT_HCI_MEMB(addr, family) = AF_BLUETOOTH;
            if (straddr == NULL) {
                PyErr_SetString(socket_error, "getsockaddrarg: "
                    "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_HCI_MEMB(addr, bdaddr)) < 0)
                return 0;
#else
            _BT_HCI_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "i", &_BT_HCI_MEMB(addr, dev))) {
                PyErr_SetString(socket_error, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
#endif
            *len_ret = sizeof *addr;
            return 1;
        }
#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
        {
            struct sockaddr_sco *addr;
            char *straddr;

            addr = (struct sockaddr_sco *)addr_ret;
            _BT_SCO_MEMB(addr, family) = AF_BLUETOOTH;
            straddr = PyString_AsString(args);
            if (straddr == NULL) {
                PyErr_SetString(socket_error, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_SCO_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
#endif
        default:
            PyErr_SetString(socket_error, "getsockaddrarg: unknown Bluetooth protocol");
            return 0;
        }
    }
#endif

#if defined(HAVE_NETPACKET_PACKET_H) && defined(SIOCGIFINDEX)
    case AF_PACKET:
    {
        struct sockaddr_ll* addr;
        struct ifreq ifr;
        char *interfaceName;
        int protoNumber;
        int hatype = 0;
        int pkttype = 0;
        char *haddr = NULL;
        unsigned int halen = 0;

        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_PACKET address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "si|iis#", &interfaceName,
                              &protoNumber, &pkttype, &hatype,
                              &haddr, &halen))
            return 0;
        strncpy(ifr.ifr_name, interfaceName, sizeof(ifr.ifr_name));
        ifr.ifr_name[(sizeof(ifr.ifr_name))-1] = '\0';
        if (ioctl(s->sock_fd, SIOCGIFINDEX, &ifr) < 0) {
            s->errorhandler(L);
            return 0;
        }
        if (halen > 8) {
          PyErr_SetString(PyExc_ValueError,
                          "Hardware address must be 8 bytes or less");
          return 0;
        }
        if (protoNumber < 0 || protoNumber > 0xffff) {
            PyErr_SetString(
                PyExc_OverflowError,
                "getsockaddrarg: protoNumber must be 0-65535.");
            return 0;
        }
        addr = (struct sockaddr_ll*)addr_ret;
        addr->sll_family = AF_PACKET;
        addr->sll_protocol = htons((short)protoNumber);
        addr->sll_ifindex = ifr.ifr_ifindex;
        addr->sll_pkttype = pkttype;
        addr->sll_hatype = hatype;
        if (halen != 0) {
          memcpy(&addr->sll_addr, haddr, halen);
        }
        addr->sll_halen = halen;
        *len_ret = sizeof *addr;
        return 1;
    }
#endif

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        unsigned int atype, v1, v2, v3;
        unsigned int scope = TIPC_CLUSTER_SCOPE;
        struct sockaddr_tipc *addr;

        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_TIPC address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }

        if (!PyArg_ParseTuple(args,
                                "IIII|I;Invalid TIPC address format",
                                &atype, &v1, &v2, &v3, &scope))
            return 0;

        addr = (struct sockaddr_tipc *) addr_ret;
        memset(addr, 0, sizeof(struct sockaddr_tipc));

        addr->family = AF_TIPC;
        addr->scope = scope;
        addr->addrtype = atype;

        if (atype == TIPC_ADDR_NAMESEQ) {
            addr->addr.nameseq.type = v1;
            addr->addr.nameseq.lower = v2;
            addr->addr.nameseq.upper = v3;
        } else if (atype == TIPC_ADDR_NAME) {
            addr->addr.name.name.type = v1;
            addr->addr.name.name.instance = v2;
        } else if (atype == TIPC_ADDR_ID) {
            addr->addr.id.node = v1;
            addr->addr.id.ref = v2;
        } else {
            /* Shouldn't happen */
            //PyErr_SetString(PyExc_TypeError, "Invalid address type");
			lua_pushstring(L, "Invalid address type");
            return 0;
        }

        *len_ret = sizeof(*addr);
        return 1;
    }
#endif
    /* More cases here... */
    default:
		createErrorObj(L, "cSocketError", "getsockaddrarg: bad family");
        return 0;
    }
}


/* Get the address length according to the socket object's address family.
   Return 1 if the family is known, 0 otherwise.  The length is returned
   through len_ret. */
//如果有异常,把错误对象压栈,失败返回0
static int getsockaddrlen(lua_State* L, PySocketSockObject *s, socklen_t *len_ret){
    switch (s->sock_family) {

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        *len_ret = sizeof (struct sockaddr_un);
        return 1;
    }
#endif /* AF_UNIX */

#if defined(AF_NETLINK)
    case AF_NETLINK:
    {
        *len_ret = sizeof (struct sockaddr_nl);
        return 1;
    }
#endif

    case AF_INET:
    {
        *len_ret = sizeof (struct sockaddr_in);
        return 1;
    }

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        *len_ret = sizeof (struct sockaddr_in6);
        return 1;
    }
#endif

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
    {
        switch(s->sock_proto)
        {

        case BTPROTO_L2CAP:
            *len_ret = sizeof (struct sockaddr_l2);
            return 1;
        case BTPROTO_RFCOMM:
            *len_ret = sizeof (struct sockaddr_rc);
            return 1;
        case BTPROTO_HCI:
            *len_ret = sizeof (struct sockaddr_hci);
            return 1;
#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
            *len_ret = sizeof (struct sockaddr_sco);
            return 1;
#endif
        default:
            //PyErr_SetString(socket_error, "getsockaddrlen: ""unknown BT protocol");
			createErrorObj(L, "cSocketError", "getsockaddrlen: ""unknown BT protocol");
            return 0;
        }
    }
#endif

#ifdef HAVE_NETPACKET_PACKET_H
    case AF_PACKET:
    {
        *len_ret = sizeof (struct sockaddr_ll);
        return 1;
    }
#endif

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        *len_ret = sizeof (struct sockaddr_tipc);
        return 1;
    }
#endif

    /* More cases here... */

    default:        
		createErrorObj(L, "cSocketError", "getsockaddrlen: bad family");
        return 0;
    }
}


/* s.accept() method */
//static PyObject *sock_accept(PySocketSockObject *s)
static int sock_accept(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1),SELF = 1;//[-0, +0, -]
	
	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");	
	checkInstance(L, SELF, CLASS);//检查传过来的实例

    sock_addr_t addrbuf;
    SOCKET_T newfd;
    socklen_t addrlen;
	
    if (!getsockaddrlen(L,*s, &addrlen))
        return lua_error(L);
    memset(&addrbuf, 0, addrlen);

    newfd = INVALID_SOCKET;

	if (!IS_SELECTABLE(*s)) {
		select_error(L);
		return lua_error(L);
	}
    BEGIN_SELECT_LOOP((*s))
    
    auto timeout = internal_select_ex(*s, 0, interval);
    if (!timeout)
        newfd = accept((*s)->sock_fd, SAS2SA(&addrbuf), &addrlen);
    
    if (timeout == 1) {
        //PyErr_SetString(socket_timeout, "timed out");
		return luaL_error(L, "timed out");
    }
    END_SELECT_LOOP(s)

	if (newfd == INVALID_SOCKET) {
		(*s)->errorhandler(L);
		return lua_error(L);
	}

    /* Create the new object with unspecified family,
       to avoid calls to bind() etc. on it. */
    auto ret = new_sockobject(L,SELF,newfd,(*s)->sock_family,(*s)->sock_type,(*s)->sock_proto);//成功就会压栈socket对象
    if (!ret) {
        SOCKETCLOSE(newfd);
        goto finally;
    }
    bool ok = makesockaddr(L,static_cast<int>((*s)->sock_fd), SAS2SA(&addrbuf),addrlen, (*s)->sock_proto);//成功就会压栈address对象
    if (!ok)
        goto finally;

	return 2;
finally://失败
    return lua_error(L);
}

/*PyDoc_STRVAR(accept_doc,
"accept() -> (socket object, address info)\n\
\n\
Wait for an incoming connection.  Return a new socket representing the\n\
connection, and the address of the client.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");*/

/* s.setblocking(flag) method.  Argument:
   False -- non-blocking mode; same as settimeout(0)
   True -- blocking mode; same as settimeout(None)
*/

//static PyObject * sock_setblocking(PySocketSockObject *s, PyObject *arg)
static int sock_setblocking(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1,BLOCK = 2;
	    
	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

	auto block = static_cast<int>(luaL_checkinteger(L, BLOCK));

    (*s)->sock_timeout = block ? -1.0 : 0.0;
    internal_setblocking(*s, block);	    
    return 0;
}

/*PyDoc_STRVAR(setblocking_doc,
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
setblocking(True) is equivalent to settimeout(None);\n\
setblocking(False) is equivalent to settimeout(0.0).");*/

/* s.settimeout(timeout) method.  Argument:
   None -- no timeout, blocking mode; same as setblocking(True)
   0.0  -- non-blocking mode; same as setblocking(False)
   > 0  -- timeout mode; operations time out after timeout seconds
   < 0  -- illegal; raises an exception
*/
//static PyObject *
//sock_settimeout(PySocketSockObject *s, PyObject *arg)
//static int sock_settimeout(lua_State* L) {
//    double timeout;
//
//    if (arg == Py_None)
//        timeout = -1.0;
//    else {
//        timeout = PyFloat_AsDouble(arg);
//        if (timeout < 0.0) {
//            if (!PyErr_Occurred())
//                PyErr_SetString(PyExc_ValueError,
//                                "Timeout value out of range");
//            return NULL;
//        }
//    }
//
//    s->sock_timeout = timeout;
//    internal_setblocking(s, timeout < 0.0);
//
//    Py_INCREF(Py_None);
//    return Py_None;
//}

/*PyDoc_STRVAR(settimeout_doc,
"settimeout(timeout)\n\
\n\
Set a timeout on socket operations.  'timeout' can be a float,\n\
giving in seconds, or None.  Setting a timeout of None disables\n\
the timeout feature and is equivalent to setblocking(1).\n\
Setting a timeout of zero is the same as setblocking(0).");*/

/* s.gettimeout() method.
   Returns the timeout associated with a socket. */
//static PyObject *
//sock_gettimeout(PySocketSockObject *s)
//static int sock_gettimeout(lua_State* L) {
//    if (s->sock_timeout < 0.0) {
//        lua_pushnil(L);        
//    }
//    else
//         lua_pushnumber(L,s->sock_timeout);
//	return 1;
//}

/*PyDoc_STRVAR(gettimeout_doc,
"gettimeout() -> timeout\n\
\n\
Returns the timeout in seconds (float) associated with socket \n\
operations. A timeout of None indicates that timeouts on socket \n\
operations are disabled.");*/

#ifdef RISCOS
/* s.sleeptaskw(1 | 0) method */

// static PyObject *
// sock_sleeptaskw(PySocketSockObject *s,PyObject *arg)
// {
//     int block;
//     block = PyInt_AsLong(arg);
//     if (block == -1 && PyErr_Occurred())
//         return NULL;
//     //,.Py_BEGIN_ALLOW_THREADS
//     socketioctl(s->sock_fd, 0x80046679, (u_long*)&block);
//     //,.Py_END_ALLOW_THREADS

//     Py_INCREF(Py_None);
//     return Py_None;
// }
/*PyDoc_STRVAR(sleeptaskw_doc,
"sleeptaskw(flag)\n\
\n\
Allow sleeps in taskwindows.");*/
#endif


/* s.setsockopt() method.
   With an integer third argument, sets an integer option.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string. */
/*
static PyObject *
sock_setsockopt(PySocketSockObject *s, PyObject *args)
{
    int level;
    int optname;
    int res;
    char *buf;
    int buflen;
    int flag;

    if (PyArg_ParseTuple(args, "iii:setsockopt",
                         &level, &optname, &flag)) {
        buf = (char *) &flag;
        buflen = sizeof flag;
    }
    else {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "iis#:setsockopt",
                              &level, &optname, &buf, &buflen))
            return NULL;
    }
    res = setsockopt(s->sock_fd, level, optname, (void *)buf, buflen);
    if (res < 0)
        return s->errorhandler(L);
    Py_INCREF(Py_None);
    return Py_None;
}*/

/*PyDoc_STRVAR(setsockopt_doc,
"setsockopt(level, option, value)\n\
\n\
Set a socket option.  See the Unix manual for level and option.\n\
The value argument can either be an integer or a string.");*/


/* s.getsockopt() method.
   With two arguments, retrieves an integer option.
   With a third integer argument, retrieves a string buffer of that size;
   use optional built-in module 'struct' to decode the string. */
//
//static PyObject *
//sock_getsockopt(PySocketSockObject *s, PyObject *args)
//{
//    int level;
//    int optname;
//    int res;
//    PyObject *buf;
//    socklen_t buflen = 0;
//
//#ifdef __BEOS__
//    /* We have incomplete socket support. */
//    PyErr_SetString(socket_error, "getsockopt not supported");
//    return NULL;
//#else
//
//    if (!PyArg_ParseTuple(args, "ii|i:getsockopt",
//                          &level, &optname, &buflen))
//        return NULL;
//
//    if (buflen == 0) {
//        int flag = 0;
//        socklen_t flagsize = sizeof flag;
//        res = getsockopt(s->sock_fd, level, optname,
//                         (void *)&flag, &flagsize);
//        if (res < 0)
//            return s->errorhandler(L);
//        return PyInt_FromLong(flag);
//    }
//#ifdef __VMS
//    /* socklen_t is unsigned so no negative test is needed,
//       test buflen == 0 is previously done */
//    if (buflen > 1024) {
//#else
//    if (buflen <= 0 || buflen > 1024) {
//#endif
//        PyErr_SetString(socket_error,
//                        "getsockopt buflen out of range");
//        return NULL;
//    }
//    buf = PyString_FromStringAndSize((char *)NULL, buflen);
//    if (buf == NULL)
//        return NULL;
//    res = getsockopt(s->sock_fd, level, optname,
//                     (void *)PyString_AS_STRING(buf), &buflen);
//    if (res < 0) {
//        Py_DECREF(buf);
//        return s->errorhandler(L);
//    }
//    _PyString_Resize(&buf, buflen);
//    return buf;
//#endif /* __BEOS__ */
//}

/*PyDoc_STRVAR(getsockopt_doc,
"getsockopt(level, option[, buffersize]) -> value\n\
\n\
Get a socket option.  See the Unix manual for level and option.\n\
If a nonzero buffersize argument is given, the return value is a\n\
string of that length; otherwise it is an integer.");*/


/* s.bind(sockaddr) method */
//static PyObject *sock_bind(PySocketSockObject *s, PyObject *addro)
static int sock_bind(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1,ADDRESS = 2;

	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

	luaL_checkany(L, ADDRESS);
    sock_addr_t addrbuf;
    int addrlen;
    
    if (!getsockaddrarg(L, ADDRESS, *s, SAS2SA(&addrbuf), &addrlen))
        return lua_error(L);
    auto res = ::bind((*s)->sock_fd, SAS2SA(&addrbuf), addrlen);

	if (res < 0) {
		(*s)->errorhandler(L);
		return lua_error(L);
	}
    return 0;
}

/*PyDoc_STRVAR(bind_doc,
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host. For raw packet\n\
sockets the address is a tuple (ifname, proto [,pkttype [,hatype]])");*/


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

//static PyObject *
//sock_close(PySocketSockObject *s)
static int sock_close(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1;
	
	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");	
	checkInstance(L, SELF, CLASS);//检查传过来的实例

    SOCKET_T fd;
    if ((fd = (*s)->sock_fd) != -1) {
        (*s)->sock_fd = -1;        
        (void) SOCKETCLOSE(fd);        
    }
    return 0;
}

/*PyDoc_STRVAR(close_doc,
"close()\n\
\n\
Close the socket.  It cannot be used after this call.");*/

//static int
//internal_connect(PySocketSockObject *s, struct sockaddr *addr, int addrlen,
//                 int *timeoutp)
//{
//    int res, timeout;
//
//    timeout = 0;
//    res = connect(s->sock_fd, addr, addrlen);
//
//#ifdef MS_WINDOWS
//
//    if (s->sock_timeout > 0.0) {
//        if (res < 0 && WSAGetLastError() == WSAEWOULDBLOCK &&
//            IS_SELECTABLE(s)) {
//            /* This is a mess.  Best solution: trust select */
//            fd_set fds;
//            fd_set fds_exc;
//            struct timeval tv;
//            tv.tv_sec = (int)s->sock_timeout;
//            tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
//            FD_ZERO(&fds);
//            FD_SET(s->sock_fd, &fds);
//            FD_ZERO(&fds_exc);
//            FD_SET(s->sock_fd, &fds_exc);
//            res = select(s->sock_fd+1, NULL, &fds, &fds_exc, &tv);
//            if (res == 0) {
//                res = WSAEWOULDBLOCK;
//                timeout = 1;
//            } else if (res > 0) {
//                if (FD_ISSET(s->sock_fd, &fds))
//                    /* The socket is in the writeable set - this
//                       means connected */
//                    res = 0;
//                else {
//                    /* As per MS docs, we need to call getsockopt()
//                       to get the underlying error */
//                    int res_size = sizeof res;
//                    /* It must be in the exception set */
//                    assert(FD_ISSET(s->sock_fd, &fds_exc));
//                    if (0 == getsockopt(s->sock_fd, SOL_SOCKET, SO_ERROR,
//                                        (char *)&res, &res_size))
//                        /* getsockopt also clears WSAGetLastError,
//                           so reset it back. */
//                        WSASetLastError(res);
//                    else
//                        res = WSAGetLastError();
//                }
//            }
//            /* else if (res < 0) an error occurred */
//        }
//    }
//
//    if (res < 0)
//        res = WSAGetLastError();
//
//#else
//
//    if (s->sock_timeout > 0.0) {
//        if (res < 0 && errno == EINPROGRESS && IS_SELECTABLE(s)) {
//            timeout = internal_select(s, 1);
//            if (timeout == 0) {
//                /* Bug #1019808: in case of an EINPROGRESS,
//                   use getsockopt(SO_ERROR) to get the real
//                   error. */
//                socklen_t res_size = sizeof res;
//                (void)getsockopt(s->sock_fd, SOL_SOCKET,
//                                 SO_ERROR, &res, &res_size);
//                if (res == EISCONN)
//                    res = 0;
//                errno = res;
//            }
//            else if (timeout == -1) {
//                res = errno;            /* had error */
//            }
//            else
//                res = EWOULDBLOCK;                      /* timed out */
//        }
//    }
//
//    if (res < 0)
//        res = errno;
//
//#endif
//    *timeoutp = timeout;
//
//    return res;
//}

/* s.connect(sockaddr) method */
//
//static PyObject *sock_connect(PySocketSockObject *s, PyObject *addro)
//{
//    sock_addr_t addrbuf;
//    int addrlen;
//    int res;
//    int timeout;
//
//    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
//        return NULL;
//
//    //,.Py_BEGIN_ALLOW_THREADS
//    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, &timeout);
//    //,.Py_END_ALLOW_THREADS
//
//    if (timeout == 1) {
//        PyErr_SetString(socket_timeout, "timed out");
//        return NULL;
//    }
//    if (res != 0)
//        return s->errorhandler(L);
//    Py_INCREF(Py_None);
//    return Py_None;
//}

/*PyDoc_STRVAR(connect_doc,
"connect(address)\n\
\n\
Connect the socket to a remote address.  For IP sockets, the address\n\
is a pair (host, port).");*/


/* s.connect_ex(sockaddr) method */
//
//static PyObject *
//sock_connect_ex(PySocketSockObject *s, PyObject *addro)
//{
//    sock_addr_t addrbuf;
//    int addrlen;
//    int res;
//    int timeout;
//
//    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
//        return NULL;
//
//    //,.Py_BEGIN_ALLOW_THREADS
//    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, &timeout);
//    //,.Py_END_ALLOW_THREADS
//
//    /* Signals are not errors (though they may raise exceptions).  Adapted
//       from PyErr_SetFromErrnoWithFilenameObject(). */
//#ifdef EINTR
//    if (res == EINTR && PyErr_CheckSignals())
//        return NULL;
//#endif
//
//    return PyInt_FromLong((long) res);
//}

/*PyDoc_STRVAR(connect_ex_doc,
"connect_ex(address) -> errno\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.");*/


/* s.fileno() method */

//static PyObject * sock_fileno(PySocketSockObject *s)
static int sock_fileno(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1, BLOCK = 2;

	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

	lua_pushinteger(L, (*s)->sock_fd);
	return 1;	
}

/*PyDoc_STRVAR(fileno_doc,
"fileno() -> integer\n\
\n\
Return the integer file descriptor of the socket.");*/


#ifndef NO_DUP
/* s.dup() method */

// static PyObject *
// sock_dup(PySocketSockObject *s)
// {
//     SOCKET_T newfd;
//     PyObject *sock;

//     newfd = dup(s->sock_fd);
//     if (newfd < 0)
//         return s->errorhandler(L);
//     sock = (PyObject *) new_sockobject(L,SELF,newfd,
//                                        s->sock_family,
//                                        s->sock_type,
//                                        s->sock_proto);
//     if (sock == NULL)
//         SOCKETCLOSE(newfd);
//     return sock;
// }

/*PyDoc_STRVAR(dup_doc,
"dup() -> socket object\n\
\n\
Return a new socket object connected to the same system resource.");*/

#endif


/* s.getsockname() method */

//static PyObject * sock_getsockname(PySocketSockObject *s)
static int sock_getsockname(lua_State* L){
	auto const CLASS = lua_upvalueindex(1),SELF = 1;//[-0, +0, -]

	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

     sock_addr_t addrbuf;
     socklen_t addrlen;

     if (!getsockaddrlen(L,*s, &addrlen))
         return lua_error(L);
     memset(&addrbuf, 0, addrlen);
     auto res = getsockname((*s)->sock_fd, SAS2SA(&addrbuf), &addrlen);
	 if (res < 0) {
		 (*s)->errorhandler(L);
		 return lua_error(L);
	 }
	 auto ok = makesockaddr(L,static_cast<int>((*s)->sock_fd), SAS2SA(&addrbuf), addrlen,(*s)->sock_proto);
	 if (!ok)
		 return lua_error(L);
	 return 1;
 }

/*PyDoc_STRVAR(getsockname_doc,
"getsockname() -> address info\n\
\n\
Return the address of the local endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");*/


#ifdef HAVE_GETPEERNAME         /* Cray APP doesn't have this :-( */
/* s.getpeername() method */

// static PyObject *
// sock_getpeername(PySocketSockObject *s)
// {
//     sock_addr_t addrbuf;
//     int res;
//     socklen_t addrlen;

//     if (!getsockaddrlen(s, &addrlen))
//         return NULL;
//     memset(&addrbuf, 0, addrlen);
//     //,.Py_BEGIN_ALLOW_THREADS
//     res = getpeername(s->sock_fd, SAS2SA(&addrbuf), &addrlen);
//     //,.Py_END_ALLOW_THREADS
//     if (res < 0)
//         return s->errorhandler(L);
//     return makesockaddr(s->sock_fd, SAS2SA(&addrbuf), addrlen,
//                         s->sock_proto);
// }

/*PyDoc_STRVAR(getpeername_doc,
"getpeername() -> address info\n\
\n\
Return the address of the remote endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");*/

#endif /* HAVE_GETPEERNAME */


/* s.listen(n) method */

//static PyObject *
//sock_listen(PySocketSockObject *s, PyObject *arg)
static int sock_listen(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1,BACKLOG = 2;
	
	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例
	auto backlog = static_cast<int>(luaL_checkinteger(L, BACKLOG));

    /* To avoid problems on systems that don't allow a negative backlog
     * (which doesn't make sense anyway) we force a minimum value of 0. */
    if (backlog < 0)
        backlog = 0;
    auto res = listen((*s)->sock_fd, backlog);
    
	if (res < 0) {
		(*s)->errorhandler(L);
		return lua_error(L);
	}
    return 0;
}

/*PyDoc_STRVAR(listen_doc,
"listen(backlog)\n\
\n\
Enable a server to accept connections.  The backlog argument must be at\n\
least 0 (if it is lower, it is set to 0); it specifies the number of\n\
unaccepted connections that the system will allow before refusing new\n\
connections.");*/


#ifndef NO_DUP
/* s.makefile(mode) method.
   Create a new open file object referring to a dupped version of
   the socket's file descriptor.  (The dup() call is necessary so
   that the open file and socket objects may be closed independent
   of each other.)
   The mode argument specifies 'r' or 'w' passed to fdopen(). */

// static PyObject *
// sock_makefile(PySocketSockObject *s, PyObject *args)
// {
//     extern int fclose(FILE *);
//     char *mode = "r";
//     int bufsize = -1;
// #ifdef MS_WIN32
//     Py_intptr_t fd;
// #else
//     int fd;
// #endif
//     FILE *fp;
//     PyObject *f;
// #ifdef __VMS
//     char *mode_r = "r";
//     char *mode_w = "w";
// #endif

//     if (!PyArg_ParseTuple(args, "|si:makefile", &mode, &bufsize))
//         return NULL;
// #ifdef __VMS
//     if (strcmp(mode,"rb") == 0) {
//         mode = mode_r;
//     }
//     else {
//         if (strcmp(mode,"wb") == 0) {
//             mode = mode_w;
//         }
//     }
// #endif
// #ifdef MS_WIN32
//     if (((fd = _open_osfhandle(s->sock_fd, _O_BINARY)) < 0) ||
//         ((fd = dup(fd)) < 0) || ((fp = fdopen(fd, mode)) == NULL))
// #else
//     if ((fd = dup(s->sock_fd)) < 0 || (fp = fdopen(fd, mode)) == NULL)
// #endif
//     {
//         if (fd >= 0)
//             SOCKETCLOSE(fd);
//         return s->errorhandler(L);
//     }
//     f = PyFile_FromFile(fp, "<socket>", mode, fclose);
//     if (f != NULL)
//         PyFile_SetBufSize(f, bufsize);
//     return f;
// }

/*PyDoc_STRVAR(makefile_doc,
"makefile([mode[, buffersize]]) -> file object\n\
\n\
Return a regular file object corresponding to the socket.\n\
The mode and buffersize arguments are as for the built-in open() function.");*/

#endif /* NO_DUP */

/*
 * This is the guts of the recv() and recv_into() methods, which reads into a
 * char buffer.  If you have any inc/dec ref to do to the objects that contain
 * the buffer, do it in the caller.  This function returns the number of bytes
 * successfully read.  If there was an error, it returns -1.  Note that it is
 * also possible that we return a number of bytes smaller than the request
 * bytes.
 */
//返回负数表示错误,且需要压栈错误对象
static ssize_t sock_recv_guts(lua_State* L,PySocketSockObject *s, char* cbuf, int len, int flags)
{
    ssize_t outlen = -1;
    int timeout;
#ifdef __VMS
    int remaining;
    char *read_buf;
#endif

    if (!IS_SELECTABLE(s)) {
        select_error(L);
        return -1;
    }

#ifndef __VMS
    BEGIN_SELECT_LOOP(s)
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout)
        outlen = recv(s->sock_fd, cbuf, len, flags);
    if (timeout == 1) {
        //PyErr_SetString(socket_timeout, "timed out");
		createErrorObj(L, "cSocketTimeout", "timed out");
        return -1;
    }
    END_SELECT_LOOP(s)
    if (outlen < 0) {
        /* Note: the call to errorhandler() ALWAYS indirectly returned
           NULL, so ignore its return value */
        s->errorhandler(L);
        return -1;
    }
#else
    read_buf = cbuf;
    remaining = len;
    while (remaining != 0) {
        unsigned int segment;
        int nread = -1;

        segment = remaining /SEGMENT_SIZE;
        if (segment != 0) {
            segment = SEGMENT_SIZE;
        }
        else {
            segment = remaining;
        }

        BEGIN_SELECT_LOOP(s)
        //,.Py_BEGIN_ALLOW_THREADS
        timeout = internal_select_ex(s, 0, interval);
        if (!timeout)
            nread = recv(s->sock_fd, read_buf, segment, flags);
        //,.Py_END_ALLOW_THREADS

        if (timeout == 1) {
            //PyErr_SetString(socket_timeout, "timed out");
			createErrorObj(L, "cSocketTimeout", "timed out");
            return -1;
        }
        END_SELECT_LOOP(s)

        if (nread < 0) {
            s->errorhandler(L);
            return -1;
        }
        if (nread != remaining) {
            read_buf += nread;
            break;
        }

        remaining -= segment;
        read_buf += segment;
    }
    outlen = read_buf - cbuf;
#endif /* !__VMS */

    return outlen;
}


/* s.recv(nbytes [,flags]) method */

//static PyObject *
//sock_recv(PySocketSockObject *s, PyObject *args)
static int sock_recv(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1,RECV_LEN = 2,FLAGS = 3;	//FLAGS是可选参数

	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

	auto recvlen = static_cast<int>(luaL_checkinteger(L, RECV_LEN));
    if (recvlen < 0) {
		luaL_error(L, "negative buffersize in recv");
    }
	int  flags = 0;
    ssize_t outlen;
	if (!lua_isnoneornil(L, FLAGS)){
		if (!lua_isinteger(L, FLAGS)) {
			luaL_argerror(L, FLAGS, "第2个参数要么别传,要传就传个整型");
		}
		flags = static_cast<int>(lua_tointeger(L, FLAGS));
	}
	luaL_Buffer b;
	luaL_buffinit(L, &b);	

    /* Allocate a new string. */    
	auto buf = luaL_prepbuffsize(&b, recvlen);//[-?, +?, e]

    /* Call the guts */
    outlen = sock_recv_guts(L, (*s), buf, recvlen, flags);
    if (outlen < 0) {
        /* An error occurred, release the string and return an
           error. */
        //Py_DECREF(buf);
        return lua_error(L);
    }
	luaL_pushresultsize(&b, outlen);
    return 1;
}

/*PyDoc_STRVAR(recv_doc,
"recv(buffersize[, flags]) -> data\n\
\n\
Receive up to buffersize bytes from the socket.  For the optional flags\n\
argument, see the Unix manual.  When no data is available, block until\n\
at least one byte is available or until the remote end is closed.  When\n\
the remote end is closed and all data is read, return the empty string.");*/


/* s.recv_into(buffer, [nbytes [,flags]]) method */

// static PyObject*
// sock_recv_into(PySocketSockObject *s, PyObject *args, PyObject *kwds)
// {
//     static char *kwlist[] = {"buffer", "nbytes", "flags", 0};

//     int recvlen = 0, flags = 0;
//     ssize_t readlen;
//     Py_buffer buf;
//     Py_ssize_t buflen;

//     /* Get the buffer's memory */
//     if (!PyArg_ParseTupleAndKeywords(args, kwds, "w*|ii:recv_into", kwlist,
//                                      &buf, &recvlen, &flags))
//         return NULL;
//     buflen = buf.len;
//     assert(buf.buf != 0 && buflen > 0);

//     if (recvlen < 0) {
//         PyErr_SetString(PyExc_ValueError,
//                         "negative buffersize in recv_into");
//         goto error;
//     }
//     if (recvlen == 0) {
//          If nbytes was not specified, use the buffer's length 
//         recvlen = buflen;
//     }

//     /* Check if the buffer is large enough */
//     if (buflen < recvlen) {
//         PyErr_SetString(PyExc_ValueError,
//                         "buffer too small for requested bytes");
//         goto error;
//     }

//     /* Call the guts */
//     readlen = sock_recv_guts(L,s, buf.buf, recvlen, flags);
//     if (readlen < 0) {
//         /* Return an error. */
//         goto error;
//     }

//     PyBuffer_Release(&buf);
//     /* Return the number of bytes read.  Note that we do not do anything
//        special here in the case that readlen < recvlen. */
//     return PyInt_FromSsize_t(readlen);

// error:
//     PyBuffer_Release(&buf);
//     return NULL;
// }

/*PyDoc_STRVAR(recv_into_doc,
"recv_into(buffer, [nbytes[, flags]]) -> nbytes_read\n\
\n\
A version of recv() that stores its data into a buffer rather than creating \n\
a new string.  Receive up to buffersize bytes from the socket.  If buffersize \n\
is not specified (or 0), receive up to the size available in the given buffer.\n\
\n\
See recv() for documentation about the flags.");*/


/*
 * This is the guts of the recvfrom() and recvfrom_into() methods, which reads
 * into a char buffer.  If you have any inc/def ref to do to the objects that
 * contain the buffer, do it in the caller.  This function returns the number
 * of bytes successfully read.  If there was an error, it returns -1.  Note
 * that it is also possible that we return a number of bytes smaller than the
 * request bytes.
 *
 * 'addr' is a return value for the address object.  Note that you must decref
 * it yourself.
 */
//static ssize_t
//sock_recvfrom_guts(PySocketSockObject *s, char* cbuf, int len, int flags,
//                   PyObject** addr)
//{
//    sock_addr_t addrbuf;
//    int timeout;
//    ssize_t n = -1;
//    socklen_t addrlen;
//
//    *addr = NULL;
//
//    if (!getsockaddrlen(s, &addrlen))
//        return -1;
//
//    if (!IS_SELECTABLE(s)) {
//        select_error(L);
//        return -1;
//    }
//
//    BEGIN_SELECT_LOOP(s)
//    //,.Py_BEGIN_ALLOW_THREADS
//    memset(&addrbuf, 0, addrlen);
//    timeout = internal_select_ex(s, 0, interval);
//    if (!timeout) {
//#ifndef MS_WINDOWS
//#if defined(PYOS_OS2) && !defined(PYCC_GCC)
//        n = recvfrom(s->sock_fd, cbuf, len, flags,
//                     SAS2SA(&addrbuf), &addrlen);
//#else
//        n = recvfrom(s->sock_fd, cbuf, len, flags,
//                     (void *) &addrbuf, &addrlen);
//#endif
//#else
//        n = recvfrom(s->sock_fd, cbuf, len, flags,
//                     SAS2SA(&addrbuf), &addrlen);
//#endif
//    }
//    //,.Py_END_ALLOW_THREADS
//
//    if (timeout == 1) {
//        PyErr_SetString(socket_timeout, "timed out");
//        return -1;
//    }
//    END_SELECT_LOOP(s)
//    if (n < 0) {
//        s->errorhandler(L);
//        return -1;
//    }
//
//    if (!(*addr = makesockaddr(s->sock_fd, SAS2SA(&addrbuf),
//                               addrlen, s->sock_proto)))
//        return -1;
//
//    return n;
//}

/* s.recvfrom(nbytes [,flags]) method */

// static PyObject *
// sock_recvfrom(PySocketSockObject *s, PyObject *args)
// {
//     PyObject *buf = NULL;
//     PyObject *addr = NULL;
//     PyObject *ret = NULL;
//     int recvlen, flags = 0;
//     ssize_t outlen;

//     if (!PyArg_ParseTuple(args, "i|i:recvfrom", &recvlen, &flags))
//         return NULL;

//     if (recvlen < 0) {
//         PyErr_SetString(PyExc_ValueError,
//                         "negative buffersize in recvfrom");
//         return NULL;
//     }

//     buf = PyString_FromStringAndSize((char *) 0, recvlen);
//     if (buf == NULL)
//         return NULL;

//     outlen = sock_recvfrom_guts(s, PyString_AS_STRING(buf),
//                                 recvlen, flags, &addr);
//     if (outlen < 0) {
//         goto finally;
//     }

//     if (outlen != recvlen) {
//         /* We did not read as many bytes as we anticipated, resize the
//            string if possible and be successful. */
//         if (_PyString_Resize(&buf, outlen) < 0)
//             /* Oopsy, not so successful after all. */
//             goto finally;
//     }

//     ret = PyTuple_Pack(2, buf, addr);

// finally:
//     Py_XDECREF(buf);
//     Py_XDECREF(addr);
//     return ret;
// }

/*PyDoc_STRVAR(recvfrom_doc,
"recvfrom(buffersize[, flags]) -> (data, address info)\n\
\n\
Like recv(buffersize, flags) but also return the sender's address info.");*/


/* s.recvfrom_into(buffer[, nbytes [,flags]]) method */

// static PyObject *
// sock_recvfrom_into(PySocketSockObject *s, PyObject *args, PyObject* kwds)
// {
//     static char *kwlist[] = {"buffer", "nbytes", "flags", 0};

//     int recvlen = 0, flags = 0;
//     ssize_t readlen;
//     Py_buffer buf;
//     int buflen;

//     PyObject *addr = NULL;

//     if (!PyArg_ParseTupleAndKeywords(args, kwds, "w*|ii:recvfrom_into",
//                                      kwlist, &buf,
//                                      &recvlen, &flags))
//         return NULL;
//     buflen = buf.len;

//     if (recvlen < 0) {
//         PyErr_SetString(PyExc_ValueError,
//                         "negative buffersize in recvfrom_into");
//         goto error;
//     }
//     if (recvlen == 0) {
//          If nbytes was not specified, use the buffer's length 
//         recvlen = buflen;
//     } else if (recvlen > buflen) {
//         PyErr_SetString(PyExc_ValueError,
//                         "nbytes is greater than the length of the buffer");
//         goto error;
//     }

//     readlen = sock_recvfrom_guts(s, buf.buf, recvlen, flags, &addr);
//     if (readlen < 0) {
//         /* Return an error */
//         goto error;
//     }

//     PyBuffer_Release(&buf);
//     /* Return the number of bytes read and the address.  Note that we do
//        not do anything special here in the case that readlen < recvlen. */
//     return Py_BuildValue("lN", readlen, addr);

// error:
//     Py_XDECREF(addr);
//     PyBuffer_Release(&buf);
//     return NULL;
// }

/*PyDoc_STRVAR(recvfrom_into_doc,
"recvfrom_into(buffer[, nbytes[, flags]]) -> (nbytes, address info)\n\
\n\
Like recv_into(buffer[, nbytes[, flags]]) but also return the sender's address info.");*/


/* s.send(data [,flags]) method */

//static PyObject *
//sock_send(PySocketSockObject *s, PyObject *args)
static int sock_send(lua_State* L) {	
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1,STRING = 2,FLAGS = 3,START = 4;

	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例
	size_t len;
	auto buf = luaL_checklstring(L, STRING, &len);

	auto flags = 0;
	if (!lua_isnoneornil(L, FLAGS)) {
		if (!lua_isinteger(L, FLAGS)) {
			luaL_argerror(L, FLAGS, "flags参数要么别传,要传就传个整型");
		}
		flags = static_cast<int>(lua_tointeger(L, FLAGS));
	}
	//python版本是没有这个下标参数的
	auto start = 0;
	if (!lua_isnoneornil(L, START)) {
		if (!lua_isinteger(L, START)) {
			luaL_argerror(L, START, "start参数要么别传,要传就传个整型");
		}
		start = static_cast<int>(lua_tointeger(L, START));
		luaL_argcheck(L, start >= 1 && start<= len, START, "start必须大于等于1且小于等于字串长度");
		start -= 1;//c是以0开始的哦
	}

    if (!IS_SELECTABLE((*s))) {        
        select_error(L);
		return lua_error(L);
    }
	auto n = -1;
    BEGIN_SELECT_LOOP((*s))    
    auto timeout = internal_select_ex(*s, 1, interval);
	if (!timeout)	
#ifdef __VMS
        n = sendsegmented((*s)->sock_fd, buf + start, static_cast<int>(len - start), flags);
#else
        n = send((*s)->sock_fd, buf + start, static_cast<int>(len-start), flags);
#endif    
    if (timeout == 1) {
        return luaL_error(L, "timed out");//PyErr_SetString(socket_timeout, "timed out");
    }
    END_SELECT_LOOP((*s))
		   
	if (n < 0) {
		(*s)->errorhandler(L);
		return lua_error(L);
	}
	lua_pushinteger(L, n);
	return 1;
}

/*PyDoc_STRVAR(send_doc,
"send(data[, flags]) -> count\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  Return the number of bytes\n\
sent; this may be less than len(data) if the network is busy.");*/


/* s.sendall(data [,flags]) method */

//static PyObject *
//sock_sendall(PySocketSockObject *s, PyObject *args)
//暂不实现
//static int sock_sendall(lua_State* L) {
//    char *buf;
//    int len, n = -1, flags = 0, timeout, saved_errno;
//    Py_buffer pbuf;
//
//    if (!PyArg_ParseTuple(args, "s*|i:sendall", &pbuf, &flags))
//        return NULL;
//    buf = pbuf.buf;
//    len = pbuf.len;
//
//    if (!IS_SELECTABLE(s)) {
//        PyBuffer_Release(&pbuf);
//        return select_error();
//    }
//
//    do {
//        BEGIN_SELECT_LOOP(s)
//        //,.Py_BEGIN_ALLOW_THREADS
//        timeout = internal_select_ex(s, 1, interval);
//        n = -1;
//        if (!timeout) {
//#ifdef __VMS
//            n = sendsegmented(s->sock_fd, buf, len, flags);
//#else
//            n = send(s->sock_fd, buf, len, flags);
//#endif
//        }
//        //,.Py_END_ALLOW_THREADS
//        if (timeout == 1) {
//            PyBuffer_Release(&pbuf);
//            PyErr_SetString(socket_timeout, "timed out");
//            return NULL;
//        }
//        END_SELECT_LOOP(s)
//        /* PyErr_CheckSignals() might change errno */
//        saved_errno = errno;
//        /* We must run our signal handlers before looping again.
//           send() can return a successful partial write when it is
//           interrupted, so we can't restrict ourselves to EINTR. */
//        if (PyErr_CheckSignals()) {
//            PyBuffer_Release(&pbuf);
//            return NULL;
//        }
//        if (n < 0) {
//            /* If interrupted, try again */
//            if (saved_errno == EINTR)
//                continue;
//            else
//                break;
//        }
//        buf += n;
//        len -= n;
//    } while (len > 0);
//    PyBuffer_Release(&pbuf);
//
//    if (n < 0)
//        return s->errorhandler(L);
//
//    Py_INCREF(Py_None);
//    return Py_None;
//}

/*PyDoc_STRVAR(sendall_doc,
"sendall(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.");*/


/* s.sendto(data, [flags,] sockaddr) method */

// static PyObject *
// sock_sendto(PySocketSockObject *s, PyObject *args)
// {
//     Py_buffer pbuf;
//     PyObject *addro;
//     char *buf;
//     Py_ssize_t len;
//     sock_addr_t addrbuf;
//     int addrlen, flags, timeout;
//     long n = -1;
//     int arglen;

//     flags = 0;
//     arglen = PyTuple_Size(args);
//     switch(arglen) {
//         case 2:
//             PyArg_ParseTuple(args, "s*O:sendto", &pbuf, &addro);
//             break;
//         case 3:
//             PyArg_ParseTuple(args, "s*iO:sendto", &pbuf, &flags, &addro);
//             break;
//         default:
//             PyErr_Format(PyExc_TypeError, "sendto() takes 2 or 3"
//                          " arguments (%d given)", arglen);
//     }
//     if (PyErr_Occurred())
//         return NULL;

//     buf = pbuf.buf;
//     len = pbuf.len;

//     if (!IS_SELECTABLE(s)) {
//         PyBuffer_Release(&pbuf);
//         return select_error();
//     }

//     if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen)) {
//         PyBuffer_Release(&pbuf);
//         return NULL;
//     }

//     BEGIN_SELECT_LOOP(s)
//     //,.Py_BEGIN_ALLOW_THREADS
//     timeout = internal_select_ex(s, 1, interval);
//     if (!timeout)
//         n = sendto(s->sock_fd, buf, len, flags, SAS2SA(&addrbuf), addrlen);
//     //,.Py_END_ALLOW_THREADS

//     if (timeout == 1) {
//         PyBuffer_Release(&pbuf);
//         PyErr_SetString(socket_timeout, "timed out");
//         return NULL;
//     }
//     END_SELECT_LOOP(s)
//     PyBuffer_Release(&pbuf);
//     if (n < 0)
//         return s->errorhandler(L);
//     return PyInt_FromLong((long)n);
// }

/*PyDoc_STRVAR(sendto_doc,
"sendto(data[, flags], address) -> count\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).");*/


/* s.shutdown(how) method */

//static PyObject *
//sock_shutdown(PySocketSockObject *s, PyObject *arg)
static int sock_shutdown(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1, HOW = 2;
	
	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

    auto how = static_cast<int>(luaL_checkinteger(L, HOW));
	if (how != 1 && how != 2) {
		luaL_error(L, "shutdown参数只能1或2");
	}
    
    auto res = shutdown((*s)->sock_fd, how);
    
	if (res < 0) {
		(*s)->errorhandler(L);
		return lua_error(L);
	}
    return 0;
}

/*PyDoc_STRVAR(shutdown_doc,
"shutdown(flag)\n\
\n\
Shut down the reading side of the socket (flag == SHUT_RD), the writing side\n\
of the socket (flag == SHUT_WR), or both ends (flag == SHUT_RDWR).");*/

#if defined(MS_WINDOWS) && defined(SIO_RCVALL)
	//static PyObject*
	//sock_ioctl(PySocketSockObject *s, PyObject *arg)
	//{
	//	unsigned long cmd = SIO_RCVALL;
	//	PyObject *argO;
	//	DWORD recv;

	//	if (!PyArg_ParseTuple(arg, "kO:ioctl", &cmd, &argO))
	//		return NULL;

	//	switch (cmd) {
	//	case SIO_RCVALL: {
	//		unsigned int option = RCVALL_ON;
	//		if (!PyArg_ParseTuple(arg, "kI:ioctl", &cmd, &option))
	//			return NULL;
	//		if (WSAIoctl(s->sock_fd, cmd, &option, sizeof(option),
	//						 NULL, 0, &recv, NULL, NULL) == SOCKET_ERROR) {
	//			return set_error();
	//		}
	//		return PyLong_FromUnsignedLong(recv); }
	//	case SIO_KEEPALIVE_VALS: {
	//		struct tcp_keepalive ka;
	//		if (!PyArg_ParseTuple(arg, "k(kkk):ioctl", &cmd,
	//						&ka.onoff, &ka.keepalivetime, &ka.keepaliveinterval))
	//			return NULL;
	//		if (WSAIoctl(s->sock_fd, cmd, &ka, sizeof(ka),
	//						 NULL, 0, &recv, NULL, NULL) == SOCKET_ERROR) {
	//			return set_error();
	//		}
	//		return PyLong_FromUnsignedLong(recv); }
	//	default:
	//		PyErr_Format(PyExc_ValueError, "invalid ioctl command %d", cmd);
	//		return NULL;
	//	}
	//}
	///PyDoc_STRVAR(sock_ioctl_doc,
	//"ioctl(cmd, option) -> long\n\
	//\n\
	//Control the socket with WSAIoctl syscall. Currently supported 'cmd' values are\n\
	//SIO_RCVALL:  'option' must be one of the socket.RCVALL_* constants.\n\
	//SIO_KEEPALIVE_VALS:  'option' is a tuple of (onoff, timeout, interval).");*/

#endif

/* List of methods for socket objects */
//
//static PyMethodDef sock_methods[] = {
//    //{"accept",            (PyCFunction)sock_accept, METH_NOARGS,accept_doc},
//    //-- {"bind",              (PyCFunction)sock_bind, METH_O,bind_doc},
//    //{"close",             (PyCFunction)sock_close, METH_NOARGS,close_doc},
//    //--{"connect",           (PyCFunction)sock_connect, METH_O,connect_doc},
//    //{"connect_ex",        (PyCFunction)sock_connect_ex, METH_O,connect_ex_doc},
//#ifndef NO_DUP
//    //{"dup",               (PyCFunction)sock_dup, METH_NOARGS,dup_doc},
//#endif
//    //--{"fileno",            (PyCFunction)sock_fileno, METH_NOARGS,fileno_doc},
//#ifdef HAVE_GETPEERNAME
//    //{"getpeername",       (PyCFunction)sock_getpeername,METH_NOARGS, getpeername_doc},
//#endif
//    //{"getsockname",       (PyCFunction)sock_getsockname,METH_NOARGS, getsockname_doc},
//    //{"getsockopt",        (PyCFunction)sock_getsockopt, METH_VARARGS,getsockopt_doc},
//#if defined(MS_WINDOWS) && defined(SIO_RCVALL)    
//	//{"ioctl",(PyCFunction)sock_ioctl, METH_VARARGS,sock_ioctl_doc},
//#endif
//    //{"listen",            (PyCFunction)sock_listen, METH_O,listen_doc},
//#ifndef NO_DUP
//    //{"makefile",          (PyCFunction)sock_makefile, METH_VARARGS,makefile_doc},
//#endif
//    //{"recv",              (PyCFunction)sock_recv, METH_VARARGS,recv_doc},
//    // {"recv_into",         (PyCFunction)sock_recv_into, METH_VARARGS | METH_KEYWORDS,recv_into_doc},
//    //{"recvfrom",          (PyCFunction)sock_recvfrom, METH_VARARGS,recvfrom_doc},
//    //{"recvfrom_into",  (PyCFunction)sock_recvfrom_into, METH_VARARGS | METH_KEYWORDS,recvfrom_into_doc},send_doc},
//    //{"sendall",           (PyCFunction)sock_sendall, METH_VARARGS,sendall_doc},
//    //{"sendto",            (PyCFunction)sock_sendto, METH_VARARGS,sendto_doc},
//    //{"setblocking",       (PyCFunction)sock_setblocking, METH_O,setblocking_doc},
//    //{"settimeout",    (PyCFunction)sock_settimeout, METH_O,settimeout_doc},
//    //{"gettimeout",    (PyCFunction)sock_gettimeout, METH_NOARGS,gettimeout_doc},
//    //{"setsockopt",        (PyCFunction)sock_setsockopt, METH_VARARGS,setsockopt_doc},
//    //{"shutdown",          (PyCFunction)sock_shutdown, METH_O,shutdown_doc},
//#ifdef RISCOS
//    //{"sleeptaskw",        (PyCFunction)sock_sleeptaskw, METH_O,sleeptaskw_doc},
//#endif
//    {NULL,                      NULL}           /* sentinel */
//};

/* SockObject members */
//static PyMemberDef sock_memberlist[] = {
//       {"family", T_INT, offsetof(PySocketSockObject, sock_family), READONLY, "the socket family"},
//       {"type", T_INT, offsetof(PySocketSockObject, sock_type), READONLY, "the socket type"},
//       {"proto", T_INT, offsetof(PySocketSockObject, sock_proto), READONLY, "the socket protocol"},
//       {"timeout", T_DOUBLE, offsetof(PySocketSockObject, sock_timeout), READONLY, "the socket timeout"},
//       {0},
//};

/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

//static void sock_dealloc(PySocketSockObject *s)
static int __gc(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1),SELF = 1;//[-0, +0, -]

	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例
    if ((*s)->sock_fd != -1)
        (void) SOCKETCLOSE((*s)->sock_fd);

	delete (*s);
	return 0;
}


//static PyObject *
//sock_repr(PySocketSockObject *s)
//{
//    char buf[512];
//    long sock_fd;
//    /* On Windows, this test is needed because SOCKET_T is unsigned */
//    if (s->sock_fd == INVALID_SOCKET) {
//        sock_fd = -1;
//    }
//#if SIZEOF_SOCKET_T > SIZEOF_LONG
//    else if (s->sock_fd > LONG_MAX) {
//        /* this can occur on Win64, and actually there is a special
//           ugly printf formatter for decimal pointer length integer
//           printing, only bother if necessary*/
//        PyErr_SetString(PyExc_OverflowError,
//                        "no printf formatter to display "
//                        "the socket descriptor in decimal");
//        return NULL;
//    }
//#endif
//    else
//        sock_fd = (long)s->sock_fd;
//    PyOS_snprintf(
//        buf, sizeof(buf),
//        "<socket object, fd=%ld, family=%d, type=%d, protocol=%d>",
//        sock_fd, s->sock_family,
//        s->sock_type,
//        s->sock_proto);
//    return PyString_FromString(buf);
//}


/* Create a new, uninitialized socket object. */
//static PyObject * sock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
static int __new__(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1),ARG_CLS = 1;//[-0, +0, -]
	//判断传过来的类合不合法
	//---begin
	lua_pushcfunction(L, checkIsSubClass);
	lua_pushvalue(L, ARG_CLS);
	lua_pushvalue(L, CLASS);
	lua_call(L, 2, 0);
	//---end
	auto s = (PySocketSockObject**)lua_newuserdata(L, sizeof(PySocketSockObject*));//[-0, +1, m]	
	*s = new PySocketSockObject;
    (*s)->sock_fd = -1;
    (*s)->sock_timeout = -1.0;
    (*s)->errorhandler = &set_error;

	lua_pushvalue(L, ARG_CLS);//把实际传过来的类复制到栈顶
	lua_setmetatable(L, -2);//[-1, +0, -]
	return 1;
}

/* Initialize a new socket object. */

/*ARGSUSED*/
//static int
//sock_initobj(PyObject *self, PyObject *args, PyObject *kwds)
static int __init__(lua_State* L) {
	auto const CLASS = lua_upvalueindex(1);//[-0, +0, -]
	auto const SELF = 1, FAMILY = 2, TYPE = 3, PROTO =4, FD = 5;
	
	auto s = (PySocketSockObject**)lua_touserdata(L, SELF);
	luaL_argcheck(L, s, SELF, "invalid socket");
	checkInstance(L, SELF, CLASS);//检查传过来的实例

	int family = AF_INET, type = SOCK_STREAM, proto = 0;
	lua_Integer fd;

	if (!lua_isnoneornil(L, FAMILY)) {
		if (!lua_isinteger(L, FAMILY)) {
			luaL_argerror(L, FAMILY, "family参数要么别传,要传就传个整型");
		}
		family = static_cast<int>(lua_tointeger(L, FAMILY));
	}
	if (!lua_isnoneornil(L, TYPE)) {
		if (!lua_isinteger(L, TYPE)) {
			luaL_argerror(L, TYPE, "type参数要么别传,要传就传个整型");
		}
		type = static_cast<int>(lua_tointeger(L, TYPE));
	}
	if (!lua_isnoneornil(L, PROTO)) {
		if (!lua_isinteger(L, PROTO)) {
			luaL_argerror(L, PROTO, "proto参数要么别传,要传就传个整型");
		}
		proto = static_cast<int>(lua_tointeger(L, PROTO));
	}
	//python版本不允许直接传fd参数
	if (!lua_isnoneornil(L, FD)) {
		if (!lua_isinteger(L, FD)) {
			luaL_argerror(L, FD, "fd参数要么别传,要传就传个整型");
		}
		fd = static_cast<int>(lua_tointeger(L, FD));
	}
	else {
		fd = socket(family, type, proto);
		if (fd == INVALID_SOCKET) {
			set_error(L);
			return lua_error(L);
		}
	}
    init_sockobject((*s), fd, family, type, proto);
    return 0;
}


/* Type object for socket objects. */
//
//static PyTypeObject sock_type = {
//    PyVarObject_HEAD_INIT(0, 0)         /* Must fill in type value later */
//    "_socket.socket",                           /* tp_name */
//    sizeof(PySocketSockObject),                 /* tp_basicsize */
//    0,                                          /* tp_itemsize */
//    (destructor)sock_dealloc,                   /* tp_dealloc */
//    0,                                          /* tp_print */
//    0,                                          /* tp_getattr */
//    0,                                          /* tp_setattr */
//    0,                                          /* tp_compare */
//    (reprfunc)sock_repr,                        /* tp_repr */
//    0,                                          /* tp_as_number */
//    0,                                          /* tp_as_sequence */
//    0,                                          /* tp_as_mapping */
//    0,                                          /* tp_hash */
//    0,                                          /* tp_call */
//    0,                                          /* tp_str */
//    PyObject_GenericGetAttr,                    /* tp_getattro */
//    0,                                          /* tp_setattro */
//    0,                                          /* tp_as_buffer */
//    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
//    sock_doc,                                   /* tp_doc */
//    0,                                          /* tp_traverse */
//    0,                                          /* tp_clear */
//    0,                                          /* tp_richcompare */
//    offsetof(PySocketSockObject, weakreflist),  /* tp_weaklistoffset */
//    0,                                          /* tp_iter */
//    0,                                          /* tp_iternext */
//    sock_methods,                               /* tp_methods */
//    sock_memberlist,                            /* tp_members */
//    0,                                          /* tp_getset */
//    0,                                          /* tp_base */
//    0,                                          /* tp_dict */
//    0,                                          /* tp_descr_get */
//    0,                                          /* tp_descr_set */
//    0,                                          /* tp_dictoffset */
//    sock_initobj,                               /* tp_init */
//    PyType_GenericAlloc,                        /* tp_alloc */
//    sock_new,                                   /* tp_new */
//    PyObject_Del,                               /* tp_free */
//};



//---------------以下是我加的-------------------------
static const struct luaL_Reg socket_m[] = {
	{ "__new__", __new__ },
	{ "__init__", __init__},

	{ "bind", sock_bind},
	{ "setblocking", sock_setblocking},
	{ "listen", sock_listen},
	{ "accept", sock_accept },
	{ "recv", sock_recv},
	{ "send", sock_send},
	{ "shutdown", sock_shutdown},
	{ "close", sock_close},

	{ "fileno", sock_fileno },
	{ "getsockname", sock_getsockname },
	{ "__gc", __gc},
	//{ "__tostring", __tostring },
	{ nullptr, nullptr }
};

int create_socket_class(lua_State *L) {
	lua_pushcfunction(L, craeteClass);
	lua_call(L, 0, 1);//创建类

	//检查创建的类是不是类
	//---begin
	lua_pushcfunction(L, checkIsClass);
	lua_pushvalue(L, -2);
	lua_call(L, 1, 0);
	//---end

	lua_pushvalue(L, -1);//复制一个类作为各个成员函数的upvalue	
	luaL_setfuncs(L, socket_m, 1);//把类设为全部成员函数的upvalue.[-nup, +0, e]
	return 1;
}
