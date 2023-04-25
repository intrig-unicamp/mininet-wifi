/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>					// for the stderr file
#include <stdlib.h>					// for malloc() and free()

#include <nbsockutils.h>
#include "../../src/nbee/globals/utils.h"		// for nbGetLastError()



// WinSock Initialization
#ifdef WIN32
	#define WINSOCK_MAJOR_VERSION 2		/*!< Ask for Winsock 2.2 */
	#define WINSOCK_MINOR_VERSION 2		/*!< Ask for Winsock 2.2 */
	int sockcount= 0;					/*!< Variable that allows calling the WSAStartup() only one time */

	// Some minor differences between UNIX and Win32
	#define SHUT_WR SD_SEND			/*!< The control code for shutdown() is different in Win32 */
#endif



// Constants; used in order to keep strings here
#define SOCKET_NAME_NULL_DAD "Null address (possibly DAD Phase)"




/****************************************************
 *                                                  *
 * Locally defined functions                        *
 *                                                  *
 ****************************************************/

int sock_ismcastaddr(const struct sockaddr *saddr);





/****************************************************
 *                                                  *
 * Function bodies                                  *
 *                                                  *
 ****************************************************/

int sock_init(char *errbuf, int errbuflen)
{
#ifdef WIN32
	if (sockcount == 0)
	{
	WSADATA wsaData;			// helper variable needed to initialize Winsock

		// Ask for Winsock version 2.2.
		if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			ssnprintf(errbuf, errbuflen, "Failed to initialize Winsock\n");

			WSACleanup();

			return sockFAILURE;
		}
	}

	sockcount++;
#endif

	return sockSUCCESS;
}



void sock_cleanup()
{
#ifdef WIN32
	sockcount--;

	if (sockcount == 0)
		WSACleanup();
#endif
}


/*!
	\brief It checks if the sockaddr variable contains a multicast address.

	\return sockSUCCESS if the address is multicast, sockFAILURE if it is not. 
*/
int sock_ismcastaddr(const struct sockaddr *saddr)
{
	if (saddr->sa_family == PF_INET)
	{
		struct sockaddr_in *saddr4 = (struct sockaddr_in *) saddr;
		if (IN_MULTICAST(ntohl(saddr4->sin_addr.s_addr))) return sockSUCCESS;
		else return sockFAILURE;
	}
	else
	{
		struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *) saddr;
		if (IN6_IS_ADDR_MULTICAST(&saddr6->sin6_addr)) return sockSUCCESS;
		else return sockFAILURE;
	}
}


int sock_open(struct addrinfo *addrinfo, int server, int nconn, char *errbuf, int errbuflen)
{
int sock;

	sock = (int) socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
	if (sock == -1)
	{
		nbGetLastError("socket(): ", errbuf, errbuflen);
		return sockFAILURE;
	}


	// This is a server socket
	if (server)
	{
#ifndef WIN32
		int on;
#endif
#ifdef BSD
		// Force the use of IPv6-only addresses; in BSD you can accept both v4 and v6
		// connections if you have a "NULL" pointer as the nodename in the getaddrinfo()
		// This behaviour is not clear in the RFC 2553, so each system implements the
		// bind() differently from this point of view

		if (addrinfo->ai_family == PF_INET6)
		{

			if (setsockopt(sock, IPPROTO_IPV6, IPV6_BINDV6ONLY, (char *)&on, sizeof (int)) == -1)
			{
				ssnprintf(errbuf, errbuflen, "setsockopt(IPV6_BINDV6ONLY)");
				return sockFAILURE;
			}
		} 
#endif

		/* With Unix like systems, turn on socket option
		 * SO_REUSEADDR, because it's really really
		 * annoying to wait that the system frees the binding
		 * while testing a server
		 * 	-- Simone Basso [Tue Nov 22 00:08:56 CET 2005] */
#ifndef WIN32
		if (-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
					(void *)&on, sizeof(on)))
		{
			ssnprintf(errbuf, errbuflen,
				"setsockopt(SO_REUSEADDR)");
			return sockFAILURE;
		}

#endif

		// WARNING: if the address is a mcast one, we should place the proper Win32 code here
		if (bind(sock, addrinfo->ai_addr, (int) addrinfo->ai_addrlen) != 0)
		{
			nbGetLastError("bind(): ", errbuf, errbuflen);
			return sockFAILURE;
		}

		if (addrinfo->ai_socktype == SOCK_STREAM)
			if (listen(sock, nconn) == -1)
			{
				nbGetLastError("listen(): ", errbuf, errbuflen);
				return sockFAILURE;
			}

		// server side ended
		return sock;
	}
	else	// we're the client
	{
	struct addrinfo *tempaddrinfo;
	char *errbufptr;
	size_t bufspaceleft;

		tempaddrinfo= addrinfo;
		errbufptr= errbuf;
		bufspaceleft= errbuflen;
		*errbufptr= 0;

		// We have to loop though all the addinfo returned.
		// For instance, we can have both IPv6 and IPv4 addresses, but the service we're trying
		// to connect to is unavailable in IPv6, so we have to try in IPv4 as well
		while (tempaddrinfo)
		{	
			if (connect(sock, tempaddrinfo->ai_addr, (int) tempaddrinfo->ai_addrlen) == -1)
			{
			size_t msglen;
			char TmpBuffer[100];
			char SocketErrorMessage[2048];

				// We have to retrieve the error message before any other socket call completes, otherwise
				// the error message is lost
				nbGetLastError(NULL, SocketErrorMessage, sizeof(SocketErrorMessage) );

				// Returns the numeric address of the host that triggered the error
				sock_getascii_addrport( (struct sockaddr_storage *) tempaddrinfo->ai_addr, TmpBuffer, sizeof(TmpBuffer), NULL, 0, NI_NUMERICHOST, TmpBuffer, sizeof(TmpBuffer) );

				ssnprintf(errbufptr, (int) bufspaceleft, 
					"Is the server properly installed on %s?  connect() failed: %s", TmpBuffer, SocketErrorMessage);

				// In case more then one 'connect' fails, we manage to keep all the error messages
				msglen= strlen(errbufptr);

				errbufptr[msglen]= ' ';
				errbufptr[msglen + 1]= 0;

				bufspaceleft= bufspaceleft - (msglen + 1);
				errbufptr+= (msglen + 1);

				tempaddrinfo= tempaddrinfo->ai_next;
			}
			else
				break;
		}

		// Check how we exit from the previous loop
		// If tempaddrinfo is equal to NULL, it means that all the connect() failed.
		if (tempaddrinfo == NULL) 
		{
			closesocket(sock);
			return sockFAILURE;
		}
		else
			return sock;
	}
}


int sock_accept(int socket, struct sockaddr_storage *sockaddr_from, char *errbuf, int errbuflen)
{
socklen_t fromlength;				// temp variable that keeps the length of the 'From' variable
int childsocket;

	// Usually, the value of 'FromLen' should be filled in by the system in the following functions.
	// However, Win32 requires this value being initialized.
	fromlength = sizeof(struct sockaddr_storage);

	childsocket= (int) accept(socket, (struct sockaddr *) sockaddr_from, &fromlength);

	if (childsocket == -1)
		nbGetLastError("accept(): ", errbuf, errbuflen);

	return childsocket;
}


int sock_close(int socket, char *errbuf, int errbuflen)
{
	// 25/02/2011
	// shutdown() is a very rarely used function
	// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#shutdownman
	// I would suggest to change this function so that we do not use that
	// function and we use the close() instead. Add a new note in the manual so
	// that the user is warned: when you call 'close', the other endpoint of the
	// connection (e.g. if it's reading data) it gets zero, and the data is
	// no longer valid
	//
	// So, I comment the old code out, update the documentation of this function
	// and call the closesocket() directly.

	// SHUT_WR: subsequent calls to the send function are disallowed. 
	// For TCP sockets, a FIN will be sent after all data is sent and 
	// acknowledged by the Server.
	//
	//if (shutdown(socket, SHUT_WR) != 0)
	//{
	//	nbGetLastError("shutdown(): ", errbuf, errbuflen);

	//	// Close the socket anyway
	//	closesocket(socket);

	//	return sockFAILURE;
	//}

	closesocket(socket);
	return sockSUCCESS;
}


int sock_initaddress(const char *address, const char *port,
							struct addrinfo *hints, struct addrinfo **addrinfo, char *errbuf, int errbuflen)
{
int retval;
	
	retval = getaddrinfo(address, port, hints, addrinfo);
	if (retval != 0)
	{
		// if the getaddrinfo() fails, you have to use gai_strerror(), instead of using the standard
		// error routines (errno) in UNIX; WIN32 suggests using the nbGetLastError() instead.
		if (errbuf)
#ifdef WIN32
			nbGetLastError("getaddrinfo(): ", errbuf, errbuflen);
#else
			ssnprintf(errbuf, errbuflen, "getaddrinfo() %s", gai_strerror(retval));
#endif
		return sockFAILURE;
	}
/*!
	\warning SOCKET: I should check all the accept() in order to bind to all addresses in case
	addrinfo has more han one pointers
*/

	// This software only supports PF_INET and PF_INET6.
	if (( (*addrinfo)->ai_family != PF_INET) && ( (*addrinfo)->ai_family != PF_INET6))
	{
		ssnprintf(errbuf, errbuflen, "getaddrinfo(): socket type not supported");
		return sockFAILURE;
	}

	if ( ( (*addrinfo)->ai_socktype == SOCK_STREAM) && (sock_ismcastaddr( (*addrinfo)->ai_addr) == 0) )
	{
		ssnprintf(errbuf, errbuflen, "getaddrinfo(): multicast addresses are not valid when using TCP streams");
		return sockFAILURE;
	}

	return sockSUCCESS;
}


void sock_freeaddrinfo(struct addrinfo *addrinfo)
{
	if (addrinfo)
		freeaddrinfo(addrinfo);
}


int sock_send(int socket, const char *buffer, int size, char *errbuf, int errbuflen)
{
int nsent;

send:
#ifdef linux
/*
	Another pain... in Linux there's this flag 
	MSG_NOSIGNAL
		Requests not to send SIGPIPE on errors on stream-oriented 
		sockets when the other end breaks the connection.
		The EPIPE error is still returned.
*/
	nsent = send(socket, buffer, size, MSG_NOSIGNAL);
#else
	nsent = send(socket, buffer, size, 0);
#endif

	if (nsent == -1)
	{
		nbGetLastError("send(): ", errbuf, errbuflen);
		return sockFAILURE;
	}

	if (nsent != size)
	{
		size-= nsent;
		buffer+= nsent;
		goto send;
	}

	return sockSUCCESS;
}


int sock_bufferize(const char *buffer, int size, char *tempbuf, int *offset, int totsize, int checkonly, char *errbuf, int errbuflen)
{

	if ((*offset + size) > totsize)
	{
		ssnprintf(errbuf, errbuflen, "Not enough space in the temporary send buffer.");
		return sockFAILURE;
	};

	if (!checkonly)
		memcpy(tempbuf + (*offset), buffer, size);

	(*offset)+= size;

	return sockSUCCESS;
}


int sock_recvdgram(int socket, char *buffer, int size, int receiveall, unsigned int maxtimeout, char *errbuf, int errbuflen)
{
int nread;
int totread= 0;

	if (size == 0)
	{
		sockASSERT("I have been requested to read zero bytes", 1);
		return sockSUCCESS;
	}

again:
	nread= recvfrom(socket, &(buffer[totread]), size - totread, 0, NULL, NULL);

	if (nread == -1)
	{
		nbGetLastError("recv(): ", errbuf, errbuflen);
		return sockFAILURE;
	}

	if (nread == 0)
	{
		ssnprintf(errbuf, errbuflen, "The other host terminated the connection.");
		return sockWARNING;
	}

	// If we want to return as soon as some data has been received, 
	// let's do the job
	if (!receiveall)
		return nread;

	totread+= nread;

	if (totread != size)
		goto again;

	return totread;
}


int sock_recv(int socket, char *buffer, int size, int receiveall, unsigned int maxtimeout, char *errbuf, int errbuflen)
{
int nread;
int totread= 0;

	if (size == 0)
	{
		sockASSERT("I have been requested to read zero bytes", 1);
		return sockSUCCESS;
	}

	// We can obtain the same result using the MSG_WAITALL flag
	// However, this is not supported by recv() in Win32
again:
	if (maxtimeout)
	{
	int retval;

		retval= sock_check4waitingdata(socket, maxtimeout, errbuf, errbuflen);

		// If retval is '0', it means that the socket has data in it waiting to be read
		if (retval != sockSUCCESS)
			return retval;
	}

	nread= recv(socket, &(buffer[totread]), size - totread, 0);

	if (nread == -1)
	{
		nbGetLastError("recv(): ", errbuf, errbuflen);
		return sockFAILURE;
	}

	if (nread == 0)
	{
		ssnprintf(errbuf, errbuflen, "The other host terminated the connection.");
		return sockWARNING;
	}

	// If we want to return as soon as some data has been received, 
	// let's do the job
	if (!receiveall)
		return nread;

	totread+= nread;

	if (totread != size)
		goto again;

	return totread;
}


int sock_discard(int sock, int size, char *errbuf, int errbuflen)
{
#define TEMP_BUF_SIZE 32768

char buffer[TEMP_BUF_SIZE];		// network buffer, to be used when the message is discarded

	// A static allocation avoids the need of a 'malloc()' each time we want to discard a message
	// Our feeling is that a buffer if 32KB is enough for most of the application;
	// in case this is not enough, the "while" loop discards the message by calling the 
	// sockrecv() several times.
	// We do not want to create a bigger variable because this causes the program to exit on
	// some platforms (e.g. BSD)

	while (size > TEMP_BUF_SIZE)
	{
		if (sock_recv(sock, buffer, TEMP_BUF_SIZE, SOCK_RECEIVEALL_YES, 0, errbuf, errbuflen) == -1)
			return sockFAILURE;

		size-= TEMP_BUF_SIZE;
	}

	// If there is still data to be discarded
	// In this case, the data can fit into the temporaty buffer
	if (size)
	{
		if (sock_recv(sock, buffer, size, SOCK_RECEIVEALL_YES, 0, errbuf, errbuflen) == -1)
			return sockFAILURE;
	}

	sockASSERT("I'm currently discarding data\n", 1);

	return sockSUCCESS;
}


int sock_check_hostlist(char *hostlist, const char *sep, struct sockaddr_storage *from, char *errbuf, int errbuflen)
{
char *token;					// temp, needed to separate items into the hostlist
struct addrinfo *addrinfo, *ai_next;
char *temphostlist;

	if ( (hostlist == NULL) || (hostlist[0] == 0) )
	{
		ssnprintf(errbuf, errbuflen, "The host list is empty.");
		return sockFAILURE;
	}

	// Checks if the connecting host is among the ones allowed

	// strtok() modifies the original variable by putting '0' at the end of each token
	// So, we have to create a new temporary string in which the original content is kept
	temphostlist= (char *) malloc (strlen(hostlist) + 1);
	if (temphostlist == NULL)
	{
		nbGetLastError("sock_check_hostlist(), malloc() failed", errbuf, errbuflen);
		return sockFAILURE;
	}
	
	strcpy(temphostlist, hostlist);

	token= strtok(temphostlist, sep);

	// it avoids a warning in the compilation ('addrinfo used but not initialized')
	addrinfo = NULL;

	while( token != NULL )
	{
	struct addrinfo hints;
	int retval;

		addrinfo = NULL;
		memset(&hints, 0, sizeof (struct addrinfo) );
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype= SOCK_STREAM;

		retval = getaddrinfo(token, "0", &hints, &addrinfo);
		if (retval != 0)
		{
			ssnprintf(errbuf, errbuflen, "getaddrinfo() %s", gai_strerror(retval));

			sockASSERT(errbuf, 1);

			// Get next token
			token = strtok( NULL, sep);
			continue;
		}

		// ai_next is required to preserve the content of addrinfo, in order to deallocate it properly
		ai_next= addrinfo;
		while(ai_next)
		{
			if (sock_cmpaddr(from, (struct sockaddr_storage *) ai_next->ai_addr) == 0)
			{
				free(temphostlist);
				return sockSUCCESS;
			}

			// If we are here, it means that the current address does not matches
			// Let's try with the next one in the header chain
			ai_next= ai_next->ai_next;
		}

		freeaddrinfo(addrinfo);
		addrinfo= NULL;

		// Get next token
		token = strtok( NULL, sep);
	}

	if (addrinfo)
	{
		freeaddrinfo(addrinfo);
		addrinfo= NULL;
	}

	// Host is not in the allowed host list. Connection refused

	free(temphostlist);
	return sockWARNING;
}


int sock_cmpaddr(struct sockaddr_storage *first, struct sockaddr_storage *second)
{
	if (first->ss_family == second->ss_family)
	{
		if (first->ss_family == AF_INET)
		{
			if (memcmp(		&(((struct sockaddr_in *) first)->sin_addr), 
							&(((struct sockaddr_in *) second)->sin_addr),
							sizeof(struct in_addr) ) == 0)
								return sockSUCCESS;
		}
		else // address family is AF_INET6
		{
			if (memcmp(		&(((struct sockaddr_in6 *) first)->sin6_addr), 
							&(((struct sockaddr_in6 *) second)->sin6_addr),
							sizeof(struct in6_addr) ) == 0)
								return sockSUCCESS;
		}
	}

	return sockFAILURE;
}


int sock_getmyinfo(int sock, char *address, int addrlen, char *port, int portlen, int flags, char *errbuf, int errbuflen)
{
struct sockaddr_storage mysockaddr;
socklen_t sockaddrlen;

	sockaddrlen = sizeof(struct sockaddr_storage);

	if (getsockname(sock, (struct sockaddr *) &mysockaddr, &sockaddrlen) == -1)
	{
		nbGetLastError("getsockname(): ", errbuf, errbuflen);
		return sockFAILURE;
	}

	// Returns the address / port of corresponding to mysockaddr
	return sock_getascii_addrport(&mysockaddr, address, addrlen, port, portlen, flags, errbuf, errbuflen);
}


int sock_getpeerinfo(int sock, char *address, int addrlen, char *port, int portlen, int flags, char *errbuf, int errbuflen)
{
struct sockaddr_storage peersockaddr;
socklen_t sockaddrlen;

	sockaddrlen = sizeof(struct sockaddr_storage);

	if (getpeername(sock, (struct sockaddr *) &peersockaddr, &sockaddrlen) == -1)
	{
		nbGetLastError("getpeername(): ", errbuf, errbuflen);
		return sockFAILURE;
	}

	// Returns the address / port of corresponding to peersockaddr
	return sock_getascii_addrport(&peersockaddr, address, addrlen, port, portlen, flags, errbuf, errbuflen);
}


int sock_getascii_addrport(const struct sockaddr_storage *sockaddr, char *address, int addrlen, char *port, int portlen, int flags, char *errbuf, int errbuflen)
{
socklen_t sockaddrlen;

#ifdef WIN32
	if (sockaddr->ss_family == AF_INET)
		sockaddrlen = sizeof(struct sockaddr_in);
	else
		sockaddrlen = sizeof(struct sockaddr_in6);
#else
	sockaddrlen = sizeof(struct sockaddr_storage);
#endif

	if ((flags & NI_NUMERICHOST) == 0)	// Check that we want literal names
	{
		if ( (sockaddr->ss_family == AF_INET6) &&
			(memcmp( &((struct sockaddr_in6 *) sockaddr)->sin6_addr, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", sizeof(struct in6_addr) ) == 0) )
		{
			if (address)
				strncpy(address, SOCKET_NAME_NULL_DAD, addrlen);

			return sockSUCCESS;
		}
	}

	// Initialize address and port (just in case)
	if (address)
		address[0]= 0;
	if (port)
		port[0]= 0;

	if ( getnameinfo((struct sockaddr *) sockaddr, sockaddrlen, address, addrlen, port, portlen, flags) != 0)
	{
		// If the user wants to receive an error message
		if (errbuf)
		{
			nbGetLastError("getnameinfo(): ", errbuf, errbuflen);
			errbuf[errbuflen-1]= 0;
		}

		return sockFAILURE;
	}

	return sockSUCCESS;
}


int sock_present2network(const char *address, struct sockaddr_storage *sockaddr, int addr_family, char *errbuf, int errbuflen)
{
int retval;
struct addrinfo *addrinfo;
struct addrinfo hints;

	memset(&hints, 0, sizeof(hints) );

	hints.ai_family= addr_family;
	// Fake protocol, to avoid the problem of some OS that return 
	// a chain of addrinfo structures, one for each transport protocol
	hints.ai_socktype= SOCK_STREAM;	

	if ( (retval= sock_initaddress(address, "22222" /* fake port */, &hints, &addrinfo, errbuf, errbuflen)) == -1 )
		return sockFAILURE;

	if (addrinfo->ai_family == PF_INET)
		memcpy(sockaddr, addrinfo->ai_addr, sizeof(struct sockaddr_in) );
	else
		memcpy(sockaddr, addrinfo->ai_addr, sizeof(struct sockaddr_in6) );

	if (addrinfo->ai_next != NULL)
	{
		ssnprintf(errbuf, errbuflen, "More than one address available; using the first one returned");

		freeaddrinfo(addrinfo);
		return sockWARNING;
	}

	freeaddrinfo(addrinfo);
	return sockSUCCESS;
}


int sock_check4waitingdata(int socket, unsigned int maxtimeout, char *errbuf, int errbuflen)
{
// Structures needed for the select() call
fd_set rfds;						// set of socket descriptors we have to check
struct timeval tv;					// maximum time the select() can block waiting for data
int retval;							// select() return value

	FD_ZERO(&rfds);

	tv.tv_sec= maxtimeout;
	tv.tv_usec= 0;
	
	FD_SET(socket, &rfds);

	retval= select(socket + 1, &rfds, NULL, NULL, &tv);
	if (retval == -1)
	{
		nbGetLastError("select(): ", errbuf, errbuflen);
		return sockFAILURE;
	}

	// The timeout has expired. Return the appropriate code to the caller
	if (retval == 0)
		return sockWARNING;

	// There's data waiting in the selected socket.
	return sockSUCCESS;
}
