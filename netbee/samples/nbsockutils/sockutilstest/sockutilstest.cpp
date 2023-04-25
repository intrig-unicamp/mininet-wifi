/*
 * Copyright (c) 2002 - 2011
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following condition 
 * is met:
 * 
 * Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nbsockutils.h>


void Server(char *Address, char *Port, int AddressFamily, int TransportProtocol);
void Client(char *Address, char *Port, int AddressFamily, int TransportProtocol);


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  SockUtilsTest [-s|c] [-a address] -p port [-f 4|6]\n\n"								\
	"Options:\n"																			\
	" -s|c: starts a server (-s) or client (-c) application.\n"								\
	"       In case it is omitted, it starts a server.\n"									\
	" -a address: the address we want to bind to (in case of a server),\n"					\
	"             the address of the remote server (in case of a client).\n"				\
	"             This parameter is mandatory for a client, while it can be omitted\n"		\
	"             for a server (in this case it binds to all local addresses).\n"			\
	" -p port: the port we want to bind to (in case of a server), the port of the\n"		\
	"          remote server (in case of a client). This parameter is mandatory.\n"			\
	" -u: use UDP (default uses a TCP connection).\n"										\
	" -f 4|6: '4' if we want to use IPv4 protocol, '6' if we want to use IPv6.\n"			\
	"         In case it is missing, the system selects an address family\n"				\
	"         automatically.\n"																\
	" -h: prints this help message.\n"														\
	"\nExample:\n"																			\
	"  SockUtilsTest -s -p 2000\n\n"														\
	"    Starts a server waiting on all local addresses on port 2000.\n\n"					\
	"  SockUtilsTest -c -p 2000 -a 127.0.0.1\n\n"											\
	"    Starts a client tying to connect to a server waiting on address 127.0.0.1\n"		\
	"    and port 2000.\n\n"																\
	"Description\n"																			\
	"============================================================================\n"		\
	"This program can be used to see how to use the SockUtils cross-platform\n"				\
	"  socket class.\n"																		\
	"This program should be launched twice, one time in 'server' mode and one in\n"			\
	"  client mode. Once this has been done, the client will send a string \n"				\
	"  (typed by the user) to the server, which will show the string on screen.\n\n";

	printf("%s", string);
}



// Parses a command line switch (in case the switch requires a parameter)
// and returns a pointer to the parameter.
// The variable 'CurrentParam', which is a pointer to the currently parsed
// argument, will be modified in order to take into account the last argument (from the
// command line) that has been parsed.
char *get_parameter(int argc, char **argv, int *CurrentParam)
{
	// Check if the 'switch' has the parameter appended to it or not
	// (e.g. '-i 4' does not have the parameter appended to it)
	if (argv[*CurrentParam][2] == 0)
		(*CurrentParam)++;
	else
		return &(argv[*CurrentParam][2]);

	if (*CurrentParam < argc)
	{
		return argv[*CurrentParam];
	}
	else
	{
		printf ("Option '-%c' specified, but no port found.\n\n", argv[*CurrentParam - 1][1]);
		Usage();
	}
	return NULL;
}


int main(int argc, char *argv[])
{
char ErrBuf[1024];
int CurrentParam;
char *Address, *Port;
bool IsServer;
int AddressFamily;
int TransportProtocol;

	// Initialization
	IsServer= true;
	Address= NULL;
	Port= NULL;
	AddressFamily= AF_UNSPEC;
	TransportProtocol= SOCK_STREAM;

	CurrentParam= 1;

	while (CurrentParam < argc)
	{
		// This is an option
		if (argv[CurrentParam][0] == '-')
		{
			switch (argv[CurrentParam][1])
			{
				case 'h': Usage(); return -1; break;
				case 's': IsServer= 1; break;
				case 'c': IsServer= 0; break;
				case 'u': TransportProtocol= SOCK_DGRAM; break;
				case 'p': Port= get_parameter(argc, argv, &CurrentParam); break;
				case 'a': Address= get_parameter(argc, argv, &CurrentParam); break;
				case 'f': 
				{
				char *AddrFamily;

					AddrFamily= get_parameter(argc, argv, &CurrentParam); break;

					if (AddrFamily != NULL)
					{
						AddressFamily= atoi(AddrFamily);

						if ( (AddressFamily != 4) && (AddressFamily != 6))
							printf("Only IPv4 and IPv6 protocols are supported");
						else
						{
							if (AddressFamily == 4)
								AddressFamily= AF_INET;
							else
								AddressFamily= AF_INET6;
						}
					};
				}; break;
				default:
				{
					printf ("Option '-%c' not recognized.\n\n", argv[CurrentParam][1]);
					Usage();
					break;
				}
			}

			// Move to the next option
			CurrentParam++;
		}
	}

	if (Port == NULL)
	{
		printf("The port has not been specified.\n\n");
		Usage();
		return -1;
	}

	if ((!IsServer) && (Address == NULL))
	{
		printf("The address has not been specified.\n\n");
		Usage();
		return -1;
	}

	// Initializes socket library
	if (sock_init(ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		printf("Error initializing SockUtils library: %s\n", ErrBuf);
		return -1;
	}

	printf("\n\n");

	if (IsServer)
		Server(Address, Port, AddressFamily, TransportProtocol);
	else
		Client(Address, Port, AddressFamily, TransportProtocol);

	// Deallocates socket library
	sock_cleanup();

	printf("\n\nNow exiting from the program.\n\n");
	return 0;
}


void Server(char *Address, char *Port, int AddressFamily, int TransportProtocol)
{
char ErrBuf[1024];
char DataBuffer[1024];
int ServerSocket, ChildSocket;	// keeps the socket ID for this connection
struct addrinfo Hints;			// temporary struct to keep settings needed to open the new socket
struct addrinfo *AddrInfo;		// keeps the addrinfo chain; required to open a new socket
struct sockaddr_storage From;	// temp variable that keeps the parameters of the incoming connection
int ReadBytes;					// Number of bytes read from the socket
char RemoteAddress[1024];				// temp variable to store the address of the connecting host
char RemotePort[1024];				// temp variable to store the port used by the connecting host

	// Prepare to open a new server socket
	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AddressFamily;
	Hints.ai_socktype= TransportProtocol;	// Open a TCP/UDP connection
	Hints.ai_flags = AI_PASSIVE;		// This is a server: ready to bind() a socket 

	if (sock_initaddress (Address, Port, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		printf("Error resolving given address/port: %s\n\n", ErrBuf);
		return;
	}

	printf("Server waiting on address %s, port %s, using protocol %s\n", 
		Address ? Address : "all local addresses", Port, (AddrInfo->ai_family == AF_INET) ? "IPv4" : "IPv6");
 
	if ( (ServerSocket= sock_open(AddrInfo, 1, 10,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		// AddrInfo is no longer required
		sock_freeaddrinfo(AddrInfo);
		printf("Cannot opening the socket: %s\n\n", ErrBuf);
		return;
	}

	// AddrInfo is no longer required
	sock_freeaddrinfo(AddrInfo);

	if (TransportProtocol == SOCK_STREAM)
	{
		if ( (ChildSocket= sock_accept(ServerSocket, &From, ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			printf("Error when accepting a new connection: %s\n\n", ErrBuf);
			return;
		}

		if (sock_getascii_addrport(&From, RemoteAddress, sizeof(RemoteAddress), RemotePort, sizeof(RemotePort),
			NI_NUMERICHOST | NI_NUMERICSERV, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			printf("Error getting information related to the connecting host: %s\n\n", ErrBuf);
			return;
		}
		printf("Accepting a new connection from host %s, using remote port %s.\n\n", RemoteAddress, RemotePort);

		ReadBytes= sock_recv(ChildSocket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 30, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			printf("Error reading data: %s\n\n", ErrBuf);
			return;
		}
	}
	else
	{
		ReadBytes= sock_recvdgram(ServerSocket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 30, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			printf("Error reading data: %s\n\n", ErrBuf);
			return;
		}
	}

	if (ReadBytes == sockWARNING)
	{
		printf("We waited for enough time and no data has been received so far.\nAborting the connection.\n\n");
		return;
	}

	// Terminate buffer, just for printing purposes
	// Warning: this can originate a buffer overflow
	DataBuffer[ReadBytes]= 0;
	printf("Received the following string: '%s'\n\n", DataBuffer);
}



void Client(char *Address, char *Port, int AddressFamily, int TransportProtocol)
{
char ErrBuf[1024];
char DataBuffer[1024];
int ClientSocket;				// keeps the socket ID for this connection
struct addrinfo Hints;			// temporary struct to keep settings needed to open the new socket
struct addrinfo *AddrInfo;		// keeps the addrinfo chain; required to open a new socket
int WrittenBytes;				// Number of bytes written on the socket

	// Prepare to open a new server socket
	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AddressFamily;
	Hints.ai_socktype= TransportProtocol;	// Open a TCP or UDP connection

	if (sock_initaddress (Address, Port, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		printf("Error resolving given address/port: %s\n\n", ErrBuf);
		return;
	}

	printf("Trying to connect to server on address %s, port %s, using protocol %s\n", 
		Address ? Address : "all local addresses", Port, (AddrInfo->ai_family == AF_INET) ? "IPv4" : "IPv6");

	if ( (ClientSocket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		// AddrInfo is no longer required
		sock_freeaddrinfo(AddrInfo);
		printf("Cannot opening the socket: %s\n\n", ErrBuf);
		return;
	}

	// AddrInfo is no longer required
	sock_freeaddrinfo(AddrInfo);

	printf("Connection established.\n");
	printf("Type the string you want to send to the server: ");
	
	// Warning: possible buffer overflow here
	fgets(DataBuffer, sizeof(DataBuffer), stdin);

	// fgets reads also the newline character, so we have to reset it
	DataBuffer[strlen(DataBuffer) - 1]= 0;

	printf("\n\n");

	WrittenBytes= sock_send(ClientSocket, DataBuffer, strlen(DataBuffer), ErrBuf, sizeof(ErrBuf));
	if (WrittenBytes == sockFAILURE)
	{
		printf("Error sending data: %s\n\n", ErrBuf);
		return;
	}

	printf("String '%s' sent. Press ENTER to terminate the program\n", DataBuffer);
	fgets(DataBuffer, sizeof(DataBuffer), stdin);
}



