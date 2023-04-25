/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#ifdef SOCKUTILS_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


#ifdef WIN32
/* Prevents a compiler warning in case this was already defined (to avoid that */
/* windows.h includes winsock.h */
#ifdef _WINSOCKAPI_
#undef _WINSOCKAPI_
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <stdio.h>
#include <string.h>	/* for memset() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>	/* DNS lookup */
#include <unistd.h>	/* close() */
#include <errno.h>	/* errno() */
#include <netinet/in.h> /* for sockaddr_in, in BSD at least */
#include <arpa/inet.h>
#include <net/if.h>
#endif


#ifndef sockSUCCESS
	#define sockSUCCESS 0		//!< Return code for 'success'
	#define sockFAILURE -1		//!< Return code for 'failure'
	#define sockWARNING -2		//!< Return code for 'warning' (we can go on, but something wrong occurred)
#endif

/*!
	\mainpage Cross-platform socket utilities (IPv4-IPv6)

	This module provides a common set of primitives for socket manipulation.
	Although the socket interface defined in the RFC 3493 is excellent, several
	minor issues still remain when supporting several operating systems.

	These calls do not want to provide a better socket interface; vice versa, they intend to
	provide a set of calls that is portable among several operating systems, hiding their
	differences.


	\section use Using SockUtils
	These calls are fairly easy to use. However, please remember to initialize socket support by mean of 
	the sock_init() funtion before any other call.
	In the same way, you need to cleanup socket support by means of the sock_cleanup() before exiting
	from your program.

	Both sock_init() and sock_cleanup() must be called <b>once</b> per program.


	\section content Content

	This module includes the following sections:

	- \ref ExportedStruct
	- \ref ExportedFunc
*/





/*!
	\defgroup ExportedStruct Exported Structures and Definitions
*/

/*! \addtogroup ExportedStruct
	\{
*/




// Some minor differences between UNIX and Win32
#ifndef WIN32
/*!
	\brief In Win32, the close() call cannot be used for sockets.
	
	So, we define a generic closesocket() call in order to be cross-platform compatible.
*/
	#define closesocket(a) close(a) 
#endif





/*!
	\brief DEBUG facility: it prints an error message on the screen (stderr)
	
	This macro prints the error on the standard error stream (stderr);
	if we are working in debug mode (i.e. there is no NDEBUG defined) and we are in
	Microsoft Visual C++, the error message will appear on the MSVC console as well.

	When NDEBUG is defined, this macro is empty.

	\param msg: the message you want to print.

	\param expr: 'false' if you want to abort the program, 'true' it you want
	to print the message and continue.

	\return No return values.
*/
#ifdef NDEBUG
#define sockASSERT(msg, expr) ((void)0)
#else
	#include <assert.h>
	#if (defined(WIN32) && defined(_MSC_VER))
		#include <crtdbg.h>				// for _CrtDbgReport
		// Use MessageBox(NULL, msg, "warning", MB_OK)' instead of the other calls if you want to debug a Win32 service
		// Remember to activate the 'allow service to interact with desktop' flag of the service
		#define sockASSERT(msg, expr) { _CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%s\n", msg); fprintf(stderr, "%s\n", msg); assert(expr); }
	#else
		#define sockASSERT(msg, expr) { fprintf(stderr, "%s\n", msg); assert(expr); }
	#endif
#endif




/****************************************************
 *                                                  *
 * Exported functions / definitions                 *
 *                                                  *
 ****************************************************/

//! 'checkonly' flag, into the sock_bufferize()
#define SOCKBUF_CHECKONLY 1	
//! no 'checkonly' flag, into the sock_bufferize()
#define SOCKBUF_BUFFERIZE 0

//! no 'server' flag; it opens a client socket
#define SOCKOPEN_CLIENT 0
//! 'server' flag; it opens a server socket
#define SOCKOPEN_SERVER 1

//! Changes the behaviour of the sock_recv(); it does not wait to receive all data
#define SOCK_RECEIVEALL_NO 0
//! Changes the behaviour of the sock_recv(); it waits to receive all data
#define SOCK_RECEIVEALL_YES 1


/*!
	\}
*/



#ifdef __cplusplus
extern "C" {
#endif



/*!
	\defgroup ExportedFunc Exported Functions
*/

/*! \addtogroup ExportedFunc
	\{
*/


/*!
	\brief It initializes the socket library support from the Operating System.

	This function is pretty useless on UNIX, since socket initialization is not required.
	However it is required on Win32. In UNIX, this function appears to be completely empty.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if everything is fine, sockFAILURE if some errors occurred. The error message is returned
	in the 'errbuf' variable (which is always correctly terminated).

	\warning This function must be called only <b>one time per program</b>.
*/
DLL_EXPORT int sock_init(char *errbuf, int errbuflen);


/*!
	\brief It deallocates the socket library support from the Operating System.

	This function is pretty useless on UNIX, since socket deallocation is not required.
	However it is required on Win32. In UNIX, this function appears to be completely empty.

	\warning This function must be called only <b>one time per program</b>.
*/
DLL_EXPORT void sock_cleanup();


/*!
	\brief Checks that the address, port and flags given are valids and it returns an 'addrinfo' stucture.

	This function basically calls the getaddrinfo() calls, and it performs a set of sanity checks
	to control that everything is fine (e.g. a TCP socket cannot have a mcast address, and such).
	If an error occurs, it writes the error message into 'errbuf'.

	\param address: a pointer to a user-allocated buffer containing the network address to check.
	It could be both a numeric - literal address, and it can be NULL or "" (useful in case of a server
	socket which has to bind to all addresses).

	\param port: a pointer to a user-allocated buffer containing the network port to use.

	\param hints: an addrinfo variable (passed by reference) containing the flags needed to create the 
	addrinfo structure appropriately.

	\param addrinfo: it represents the true returning value. This is a pointer to an addrinfo variable
	(passed by reference), which will be allocated by this function and returned back to the caller.
	This variable will be used in the next sockets calls.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if everything is fine, sockFAILURE if some errors occurred. The error message is returned
	in the 'errbuf' variable. The addrinfo variable that has to be used in the following sockets calls is 
	returned into the addrinfo parameter.

	\warning The 'addrinfo' variable has to be deleted by the programmer by calling sock_freeaddrinfo() when
	it is no longer needed. Please note that although the sock_freeaddrinfo() does the same job of the standard
	freeaddrinfo() socket function, you MUST not allocate a variable within a context (e.g. a dinamic link
	library) and deallocate it within another context (e.g. the calling program). This is why you should use
	the sock_freeaddrinfo() instead of the freeaddrinfo().

	\warning This function requires the 'hints' variable as parameter. The semantic of this variable is the same
	of the one of the corresponding variable used into the standard getaddrinfo() socket function. We suggest
	the programmer to look at that function in order to set the 'hints' variable appropriately.

	\warning This function may return a linked list of 'addrinfo' structure. In case the depth of the structure
	is more than '1', and you are on a server socket, you should call the sock_open() once per every structure
	in order to allow waiting on all allowed socket (this is the case, for example, in which we can accept 
	both IPv4 and IPv6 connections).

	In case of a client socket, the sock_open() will loop automatically through all the structures until a connection
	is established, and it fails only if no structures give a positive result.
*/
DLL_EXPORT int sock_initaddress(const char *address, const char *port,
						struct addrinfo *hints, struct addrinfo **addrinfo, char *errbuf, int errbuflen);


/*!
	\brief Deallocates the 'addrinfo' structure which has been allocated in the sock_initaddress().

	This function does the deallocation of the addrinfo structure.

	\warning Please note that although the sock_freeaddrinfo() does the same job of the standard
	freeaddrinfo() socket function, you MUST not allocate a variable within a context (e.g. a dinamic link
	library) and deallocate it within another context (e.g. the calling program). This is why you should use
	the sock_freeaddrinfo() instead of the freeaddrinfo().

	\param addrinfo: a pointer to the the addrinfo structure that has to be freed.
*/
DLL_EXPORT void sock_freeaddrinfo(struct addrinfo *addrinfo);


/*!
	\brief It initializes a network connection both from the client and the server side.

	In case of a client socket, this function calls socket() and connect().
	In the meanwhile, it checks for any socket error.
	If an error occurs, it writes the error message into 'errbuf'.

	In case of a server socket, the function calls socket(), bind() and listen().

	This function is usually preceeded by the sock_initaddress().

	\param addrinfo: pointer to an addrinfo variable which will be used to
	open the socket and such. This variable is the one returned by the previous call to
	sock_initaddress().

	Warning: in case the sock_initaddress() returned more than one linked structure in the
	'addrinfo' linked list, you may need to create a cycle and call the sock_open() for each
	structure in it.

	\param server: '1' if this is a server socket, '0' otherwise.

	\param nconn: number of the connections that are allowed to wait into the listen() call.
	This value has no meanings in case of a client socket.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return The socket that has been opened (that has to be used in the following sockets calls)
	if everything is fine, sockFAILURE if some errors occurred. The error message is returned
	in the 'errbuf' variable.

	\warning In order to be able to use the returned socket, you have still to call the sock_accept()
	for accepting a new connection if you are on a server.

	\warning In case of a client socket, the sock_open() will loop automatically through all the 
	structures until a connection is established, and it fails only if no structures give a positive result.

*/
DLL_EXPORT int sock_open(struct addrinfo *addrinfo, int server, int nconn, char *errbuf, int errbuflen);


/*!
	\brief This function waits on a server socket and returns when a new connection is accepted.

	\param socket: the socket identifier of the server socket.

	\param sockaddr_from: a structure (allocated by the user) that will contain the parameters
	related to the host that is connecting to the server. This can be used to know the remote
	host address, port and such (e.g. to be printed within the sock_getascii_addrport() ).

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return The identifier of the child socket that has to be used for sending and receiving data,
	or sockFAILURE in case the function fails. The error message is returned in the 'errbuf' variable.
*/
DLL_EXPORT int sock_accept(int socket, struct sockaddr_storage *sockaddr_from, char *errbuf, int errbuflen);


/*!
	\brief Closes the present (TCP and UDP) socket connection.

	This function closes the socket.
	
	Please note that any following operation on the socket is unavailable upon closure,
	on both sides of the socket (for TCP connections).
	For instance, if the server calls this function and, later, the client tries to 
	read some data that should still be there in the buffer, the data will not be
	returned any more. In fact, the sock_close() call purges the data on both
	sides of the TCP socket.

	Due to this behaviour, we suggest to use this function with care. For course,
	the socket has to be closed, but please do so when you are sure that the client
	will not read anything from it. A possible solution is to call the sock_close()
	long time after the other processing has ended, e.g., by setting a reasonable
	'sleep' time between the last socket call on the server and the sock_close()
	invoke.

	\param socket: the socket identifier of the connection that has to be closed.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if everything is fine, sockFAILURE if some errors occurred. The error message is returned
	in the 'errbuf' variable.
*/
DLL_EXPORT int sock_close(int socket, char *errbuf, int errbuflen);


/*!
	\brief It sends the amount of data contained into 'buffer' on the given socket.

	This function basically calls the send() socket function and it checks that all
	the data specified in 'buffer' (of size 'size') will be sent. If an error occurs, 
	it writes the error message into 'errbuf'.
	In case the socket buffer does not have enough space, it loops until all data 
	has been sent.

	\param socket: the connected socket currently opened.

	\param buffer: a char pointer to a user-allocated buffer in which data is contained.

	\param size: number of bytes that have to be sent.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if everything is fine, sockFAILURE if some errors occurred. The error message is returned
	in the 'errbuf' variable.
*/
DLL_EXPORT int sock_send(int socket, const char *buffer, int size, char *errbuf, int errbuflen);


/*!
	\brief It waits on a connected socket and it manages to receive data.

	This function basically calls the recv() socket function and it checks that no
	error occurred. If that happens, it writes the error message into 'errbuf'.

	This function changes its behaviour according to the 'receiveall' flag: if we
	want to receive exactly 'size' byte, it loops on the recv()	until all the requested 
	data is arrived. Otherwise, it returns the data currently available.

	In case the socket does not have enough data available, it cycles on the recv()
	util the requested data (of size 'size') is arrived.
	In this case, it blocks until the number of bytes read is equal to 'size', unless
	the 'maxtimeout' forces this function to return earlier.

	\param socket: the connected socket currently opened.

	\param buffer: a char pointer to a user-allocated buffer in which data has to be stored

	\param size: size of the allocated buffer. WARNING: this indicates the number of bytes
	that we are expecting to be read.

	\param receiveall: if '0' (or SOCK_RECEIVEALL_NO), it returns as soon as some data 
	is ready; otherwise, (or SOCK_RECEIVEALL_YES) it waits until 'size' data has been 
	received (in case the socket does not have enough data available).

	\param maxtimeout: if greather than zero, it tells the function that it cannot block
	for more than 'maxtimeout' seconds. In this case, whatever the status of the receiving
	process will be, we are sure that this function returns after max 'maxitimeout' seconds.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return the number of bytes read if everything is fine, sockFAILURE if some errors occurred, sockWARNING if
	the function returns because the timeout expires or no data is available.
	The latest return code is available:
	- in case the 'maxtimeout' is greather than zero (the socket waited for some time, but no data was available)
	- in case the socket has been closed correctly from the other party (which means that the transfer has been
	completed).
	The sockWARNING is not considered an error (therefore the 'errbuf' does not contain any error).

	In case of errors, the error message is returned in the 'errbuf' variable.

	\warning This function works only on stream sockets.
*/
DLL_EXPORT int sock_recv(int socket, char *buffer, int size, int receiveall, unsigned int maxtimeout, char *errbuf, int errbuflen);


/*!
	\brief It waits on a datagram socket and it manages to receive data.

	This function basically calls the recv() socket function and it checks that no
	error occurred. If that happens, it writes the error message into 'errbuf'.

	This function changes its behaviour according to the 'receiveall' flag: if we
	want to receive exactly 'size' byte, it loops on the recv()	until all the requested 
	data is arrived. Otherwise, it returns the data currently available.

	In case the socket does not have enough data available, it cycles on the recv()
	util the requested data (of size 'size') is arrived.
	In this case, it blocks until the number of bytes read is equal to 'size', unless
	the 'maxtimeout' forces this function to return earlier.

	\param socket: the connected socket currently opened.

	\param buffer: a char pointer to a user-allocated buffer in which data has to be stored

	\param size: size of the allocated buffer. WARNING: this indicates the number of bytes
	that we are expecting to be read.

	\param receiveall: if '0' (or SOCK_RECEIVEALL_NO), it returns as soon as some data 
	is ready; otherwise, (or SOCK_RECEIVEALL_YES) it waits until 'size' data has been 
	received (in case the socket does not have enough data available).

	\param maxtimeout: if greather than zero, it tells the function that it cannot block
	for more than 'maxtimeout' seconds. In this case, whatever the status of the receiving
	process will be, we are sure that this function returns after max 'maxitimeout' seconds.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return the number of bytes read if everything is fine, sockFAILURE if some errors occurred, sockWARNING if
	the function returns because the timeout expires or no data is available.
	The latest return code is available:
	- in case the 'maxtimeout' is greather than zero (the socket waited for some time, but no data was available)
	- in case the socket has been closed correctly from the other party (which means that the transfer has been
	completed).
	The sockWARNING is not considered an error (therefore the 'errbuf' does not contain any error).

	In case of errors, the error message is returned in the 'errbuf' variable.

	\warning This function works only on datagram sockets and it is largely untested Particularly, 'maxtimeout' does not work.
*/
DLL_EXPORT int sock_recvdgram(int socket, char *buffer, int size, int receiveall, unsigned int maxtimeout, char *errbuf, int errbuflen);


/*!
	\brief It checks if 'socket' has data waiting to be read on it.

	This function can be used to check if the socket has some data waiting to be
	processed. The most commo case is to check if there is data on a buffer, in order
	to avoid blocking on a recv() call.

	We can also use this call to avoid blocking on the sock_accept() call: if this
	call returns sockSUCCESS, a new connection is waiting to be processed and therefore
	we cann call the sock_accept() and be sure it does not block the execution flow.

	\param socket: the connected socket currently opened.

	\param maxtimeout: the maximum amout of time this function blocks waiting for data,
	in case the socket does not have anything waiting to be read.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if there is data waiting to be read, sockWARNING if the function returns 
	because of timeout expires (i.e. no data has been read), sockFAILURE if some errors occurred.
	The error message is returned in the 'errbuf' variable.
*/
DLL_EXPORT int sock_check4waitingdata(int socket, unsigned int maxtimeout, char *errbuf, int errbuflen);


/*!
	\brief It copies the amount of data contained into 'buffer' into 'tempbuf'.
	and it checks for buffer overflows.

	This function basically copies 'size' bytes of data contained into 'buffer'
	into 'tempbuf', starting at offset 'offset'. Before that, it checks that the 
	resulting buffer will not be larger	than 'totsize'. Finally, it updates 
	the 'offset' variable in order to point to the first empty location of the buffer.

	In case the function is called with 'checkonly' equal to 1, it does not copy
	the data into the buffer. It only checks for buffer overflows and it updates the
	'offset' variable. This mode can be useful when the buffer already contains the 
	data (maybe because the producer writes directly into the target buffer), so
	only the buffer overflow check has to be made.
	In this case, both 'buffer' and 'tempbuf' can be NULL values.

	This function is useful in case the userland application does not know immediately
	all the data it has to write into the socket. This function provides a way to create
	the "stream" step by step, appendning the new data to the old one. Then, when all the
	data has been bufferized, the application can call the sock_send() function.

	\param buffer: a char pointer to a user-allocated buffer that keeps the data
	that has to be copied.

	\param size: number of bytes that have to be copied.

	\param tempbuf: user-allocated buffer (of size 'totsize') in which data
	has to be copied.

	\param offset: an index into 'tempbuf' which keeps the location of its first
	empty location.

	\param totsize: total size of the buffer in which data is being copied.

	\param checkonly: '1' if we do not want to copy data into the buffer and we
	want just do a buffer ovreflow control, '0' if data has to be copied as well.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if everything is fine, sockFAILURE if some errors occurred. The error message
	is returned in the 'errbuf' variable. When the function returns, 'tempbuf' will 
	have the new string appended, and 'offset' will keep the length of that buffer.
	In case of 'checkonly == 1', data is not copied, but 'offset' is updated in any case.

	\warning This function assumes that the buffer in which data has to be stored is
	large 'totbuf' bytes.

	\warning In case of 'checkonly', be carefully to call this function *before* copying
	the data into the buffer. Otherwise, the control about the buffer overflow is useless.
*/
DLL_EXPORT int sock_bufferize(const char *buffer, int size, char *tempbuf, int *offset, int totsize, int checkonly, char *errbuf, int errbuflen);


/*!
	\brief It discards N bytes that are currently waiting to be read on the current socket.

	This function is useful in case we receive a message we cannot undestand (e.g.
	wrong version number when receiving a network packet), so that we have to discard all 
	data before reading a new message.

	This function will read 'size' bytes from the socket and discard them.
	It defines an internal buffer in which data will be copied; however, in case
	this buffer is not large enough, it will cycle in order to read everything as well.

	\param socket: the connected socket currently opened.

	\param size: number of bytes that have to be discarded.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if everything is fine, sockFAILURE if some errors occurred.
	The error message is returned in the 'errbuf' variable.
*/
DLL_EXPORT int sock_discard(int socket, int size, char *errbuf, int errbuflen);


/*!
	\brief Checks that one host (identified by the sockaddr_storage structure) belongs to an 'allowed list'.

	This function is useful after an accept() call in order to check if the connecting
	host is allowed to connect to me. To do that, we have a buffer that keeps the list of the
	allowed host; this function checks the sockaddr_storage structure of the connecting host 
	against this host list, and it returns '0' is the host is included in this list.

	\param hostlist: pointer to a string that contains the list of the allowed host.

	\param sep: a string that keeps the separators used between the hosts (for example the 
	space character) in the host list.

	\param from: a sockaddr_storage structure, as it is returned by the accept() call.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return It returns:
	- sockSUCCESS if the host belongs to the host list (and therefore it is allowed to connect)
	- sockWARNING in case the host does not belong to the host list (and therefore it is not allowed to connect)
	- sockFAILURE in case or error. The error message is returned in the 'errbuf' variable.
*/
DLL_EXPORT int sock_check_hostlist(char *hostlist, const char *sep, struct sockaddr_storage *from, char *errbuf, int errbuflen);


/*!
	\brief Compares two addresses contained into two sockaddr_storage structures.

	This function is useful to compare two addresses, given their internal representation,
	i.e. an sockaddr_storage structure.

	The two structures do not need to be sockaddr_storage; you can have both 'sockaddr_in' and
	sockaddr_in6, properly acsted in order to be compliant to the function interface.

	This function will return '0' if the two addresses matches, '-1' if not.

	\param first: a sockaddr_storage structure, (for example the one that is returned by an 
	accept() call), containing the first address to compare.

	\param second: a sockaddr_storage structure containing the second address to compare.

	\return sockSUCCESS if the addresses are equal, sockFAILURE if they are different.
*/
DLL_EXPORT int sock_cmpaddr(struct sockaddr_storage *first, struct sockaddr_storage *second);


/*!
	\brief It gets the address/port the system picked for this socket (on connected sockets).

	It is used to return the addess and port the server picked for our socket on the local machine.
	It works only on:
	- connected sockets
	- server sockets

	On unconnected client sockets it does not work because the system dynamically chooses a port
	only when the socket calls a send() call.

	\param sock: the connected socket currently opened.

	\param address: it contains the address that will be returned by the function. This buffer
	must be properly allocated by the user. The address can be either literal or numeric depending
	on the value of 'Flags'.

	\param addrlen: the length of the 'address' buffer.

	\param port: it contains the port that will be returned by the function. This buffer
	must be properly allocated by the user.

	\param portlen: the length of the 'port' buffer.

	\param flags: a set of flags (the ones defined into the getnameinfo() standard socket function)
	that determine if the resulting address must be in numeric / literal form, and so on.
	Allowed values are:
	- <b>NI_NOFQDN</b>: When the NI_NAMEREQD flag is set, host names that cannot be resolved by 
	Domain Name System (DNS) result in an error.
	- <b>NI_NUMERICHOST</b>: Setting the NI_NOFQDN flag results in local hosts having only their 
	Relative Distinguished Name (RDN) returned in the host parameter.
	- <b>NI_NAMEREQD</b>: Setting the NI_NUMERICHOST flag returns the numeric form of the host name
	instead of its name. The numeric form of the host name is also returned if the host name
	cannot be resolved by DNS.
	- <b>NI_NUMERICSERV</b>: Setting the NI_NUMERICSERV flag returns the port number of the service 
	instead of its name. If NI_NUMERICSERV is not specified and the port number contained in 
	the (internal) sockaddr structure does not resolve to a well known service, this function
	function fails. When NI_NUMERICSERV is specified, the port number is returned as a numeric string.
	- <b>NI_DGRAM</b>: Setting the NI_DGRAM flag indicates that the service is a datagram service.
	This flag is necessary for the few services that provide different port numbers for 
	UDP and TCP service.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return It returns sockSUCCESS if this function succeedes, sockFAILURE otherwise.
	The address and port corresponding are returned back in the buffers 'address' and 'port'.
	In any case, the returned strings are '0' terminated.

	\warning If the socket is using a connectionless protocol, the address may not be available 
	until I/O occurs on the socket.

	\warning The capability to perform reverse lookups using the this function is convenient,
	but such lookups are considered inherently unreliable and may fail due to network problems.

	\sa sock_getpeerinfo().
*/
DLL_EXPORT int sock_getmyinfo(int sock, char *address, int addrlen, char *port, int portlen, int flags, char *errbuf, int errbuflen);


/*!
	\brief It gets the address/port the remote system picked for this socket (on connected sockets).

	It is used to return the addess and port of the remote side of our socket. It works almost in the
	same way of sock_getmyinfo(); it works only on connected sockets.
	
	In order to get the peer name on unconnected sockets, the sock_getascii_addrport() may be used.

	\param sock: the connected socket currently opened.

	\param address: it contains the address that will be returned by the function. This buffer
	must be properly allocated by the user. The address can be either literal or numeric depending
	on the value of 'flags'.

	\param addrlen: the length of the 'address' buffer.

	\param port: it contains the port that will be returned by the function. This buffer
	must be properly allocated by the user.

	\param portlen: the length of the 'port' buffer.

	\param flags: a set of flags (the ones defined into the getnameinfo() standard socket function)
	that determine if the resulting address must be in numeric / literal form, and so on.
	Allowed values are:
	- <b>NI_NOFQDN</b>: When the NI_NAMEREQD flag is set, host names that cannot be resolved by 
	Domain Name System (DNS) result in an error.
	- <b>NI_NUMERICHOST</b>: Setting the NI_NOFQDN flag results in local hosts having only their 
	Relative Distinguished Name (RDN) returned in the host parameter.
	- <b>NI_NAMEREQD</b>: Setting the NI_NUMERICHOST flag returns the numeric form of the host name
	instead of its name. The numeric form of the host name is also returned if the host name
	cannot be resolved by DNS.
	- <b>NI_NUMERICSERV</b>: Setting the NI_NUMERICSERV flag returns the port number of the service 
	instead of its name. If NI_NUMERICSERV is not specified and the port number contained in 
	the (internal) sockaddr structure does not resolve to a well known service, this function
	function fails. When NI_NUMERICSERV is specified, the port number is returned as a numeric string.
	- <b>NI_DGRAM</b>: Setting the NI_DGRAM flag indicates that the service is a datagram service.
	This flag is necessary for the few services that provide different port numbers for 
	UDP and TCP service.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return It returns sockSUCCESS if this function succeedes, sockFAILURE otherwise.
	The address and port corresponding are returned back in the buffers 'address' and 'port'.
	In any case, the returned strings are '0' terminated.

	\warning If the socket is using a connectionless protocol, the address may not be available 
	until I/O occurs on the socket.

	\warning The capability to perform reverse lookups using the this function is convenient,
	but such lookups are considered inherently unreliable and may fail due to network problems.

	\sa sock_getmyinfo(), sock_getascii_addrport().
*/
DLL_EXPORT int sock_getpeerinfo(int sock, char *address, int addrlen, char *port, int portlen, int flags, char *errbuf, int errbuflen);


/*!
	\brief It retrieves two strings containing the address and the port of a given 'sockaddr' variable.

	This function is basically an extended version of the inet_ntop(), which does not exist in
	WIN32 because the same result can be obtained by using the getnameinfo().
	However, differently from inet_ntop(), this function is able to return also literal names
	(e.g. 'locahost') dependingly from the 'Flags' parameter.

	The function accepts a sockaddr_storage variable (which can be returned by several functions
	like bind(), connect(), accept(), and more) and it transforms its content into a 'human'
	form. So, for instance, it is able to translate an hex address (stored in bynary form) into
	a standard IPv6 address like "::1".

	The behaviour of this function depends on the parameters we have in the 'Flags' variable, which
	are the ones allowed in the standard getnameinfo() socket function.
	
	\param sockaddr: a 'sockaddr_in' or 'sockaddr_in6' structure containing the address that 
	need to be translated from network form into the presentation form. This structure must be 
	zero-ed prior using it, and the address family field must be filled with the proper value. 
	The user must cast any 'sockaddr_in' or 'sockaddr_in6' structures to 'sockaddr_storage' before 
	calling this function.

	\param address: it contains the address that will be returned by the function. This buffer
	must be properly allocated by the user. The address can be either literal or numeric depending
	on the value of 'Flags'.

	\param addrlen: the length of the 'address' buffer.

	\param port: it contains the port that will be returned by the function. This buffer
	must be properly allocated by the user.

	\param portlen: the length of the 'port' buffer.

	\param flags: a set of flags (the ones defined into the getnameinfo() standard socket function)
	that determine if the resulting address must be in numeric / literal form, and so on.
	Allowed values are:
	- <b>NI_NOFQDN</b>: When the NI_NAMEREQD flag is set, host names that cannot be resolved by 
	Domain Name System (DNS) result in an error.
	- <b>NI_NUMERICHOST</b>: Setting the NI_NOFQDN flag results in local hosts having only their 
	Relative Distinguished Name (RDN) returned in the host parameter.
	- <b>NI_NAMEREQD</b>: Setting the NI_NUMERICHOST flag returns the numeric form of the host name
	instead of its name. The numeric form of the host name is also returned if the host name
	cannot be resolved by DNS.
	- <b>NI_NUMERICSERV</b>: Setting the NI_NUMERICSERV flag returns the port number of the service 
	instead of its name. If NI_NUMERICSERV is not specified and the port number contained in 
	the (internal) sockaddr structure does not resolve to a well known service, this function
	function fails. When NI_NUMERICSERV is specified, the port number is returned as a numeric string.
	- <b>NI_DGRAM</b>: Setting the NI_DGRAM flag indicates that the service is a datagram service.
	This flag is necessary for the few services that provide different port numbers for 
	UDP and TCP service.

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return It returns sockSUCCESS if this function succeedes, sockFAILURE otherwise.
	The address and port corresponding to the given SockAddr are returned back in the buffers 'address'
	and 'port'.
	In any case, the returned strings are '0' terminated.

	\warning The capability to perform reverse lookups using the this function is convenient,
	but such lookups are considered inherently unreliable and may fail due to network problems.
*/
DLL_EXPORT int sock_getascii_addrport(const struct sockaddr_storage *sockaddr, char *address, int addrlen, char *port, int portlen, int flags, char *errbuf, int errbuflen);


/*!
	\brief It translates an address from the 'presentation' form into the 'network' form.

	This function basically replaces inet_pton(), which does not exist in WIN32 because
	the same result can be obtained by using the getaddrinfo().
	An addictional advantage is that 'Address' can be both a numeric address (e.g. '127.0.0.1',
	like in inet_pton() ) and a literal name (e.g. 'localhost').

	This function does the reverse job of sock_getascii_addrport().

	\param address: a zero-terminated string which contains the name you have to
	translate. The name can be either literal (e.g. 'localhost') or numeric (e.g. '::1').

	\param sockaddr: a user-allocated sockaddr_storage structure which will contains the
	'network' form of the requested address.

	\param addr_family: a constant which can assume the following values:
		- 'AF_INET' if we want to ping an IPv4 host
		- 'AF_INET6' if we want to ping an IPv6 host
		- 'AF_UNSPEC' if we do not have preferences about the protocol to be used

	\param errbuf: a pointer to an user-allocated buffer that will contain the complete
	error message. This buffer has to be at least 'errbuflen' in length.
	It can be NULL; in this case the error cannot be printed.

	\param errbuflen: length of the buffer that will contains the error. The error message cannot be
	larger than 'errbuflen - 1' because the last char is reserved for the string terminator.

	\return sockSUCCESS if the translation succeded, sockWARNING if there was some non critical error,
	sockFAILURE otherwise. In case it fails, the content of the 'sockaddr' variable remains unchanged.
	A 'non critical error' can occur in case the 'address' is a literal name, which can be mapped
	to several network addresses (e.g. 'foo.bar.com' => '10.2.2.2' and '10.2.2.3'). In this case
	the content of the 'sockaddr parameter will be the address corresponding to the first mapping.

	\warning The sockaddr_storage structure MUST be allocated by the user.
*/
DLL_EXPORT int sock_present2network(const char *address, struct sockaddr_storage *sockaddr, int addr_family, char *errbuf, int errbuflen);


#ifdef __cplusplus
}
#endif


/*!
	\}
*/
