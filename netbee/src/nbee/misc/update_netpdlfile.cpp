/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include <cstdio>
#include <cstring>
#include <nbee.h>
#include <nbsockutils.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"
#include "os_utils.h"
#include <cstdlib>

using namespace std;


#define NETPDL_DOWNLOAD_SITE "www.nbee.org"
#define NETPDL_DOWNLOAD_PORT "80"
#define NETPDL_DOWNLOAD_STRING "GET /download/netpdl.xml HTTP/1.0\r\nHost: www.nbee.org\r\n\r\n"
#define NETPDLSCHEMA_DOWNLOAD_STRING "GET /download/netpdl-schema.xml HTTP/1.0\r\nHost: www.nbee.org\r\n\r\n"
// Currently the 'min schema' is not supported; btw, not sure this is a good idea as the schema should be unique.
#define NETPDLSCHEMAMIN_DOWNLOAD_STRING "GET /download/netpdl-schema.xml HTTP/1.0\r\nHost: www.nbee.org\r\n\r\n"
//#define NETPDL_DOWNLOAD_STRING "GET /netpdllib/extractor.php HTTP/1.0\r\nHost: www.nbee.org\r\n\r\n"
//#define NETPDLSCHEMA_DOWNLOAD_STRING "GET /netpdllib/extractor.php HTTP/1.0\r\nHost: www.nbee.org\r\n\r\n"
//#define NETPDLSCHEMAMIN_DOWNLOAD_STRING "GET /netpdllib/extractor.php HTTP/1.0\r\nHost: www.nbee.org\r\n\r\n"

#define NETPDL_VERSION_ATTRIBUTE "version=\""


//! Buffer that will contain the new NetPDL file.
extern char* NetPDLDataFile;


// Function Definition
int CheckServerResponse(char* DataBuffer, int ReadBytes, char* ErrBuf, int ErrBufSize);
int CheckNetPDLVersion(char* ErrBuf, int ErrBufSize);
int DownloadNetPDLFile(const char* DownloadString, char** NetPDLDataBuffer, char* ErrBuf, int ErrBufSize);

#ifndef WIN32
#define strnicmp strncasecmp
#endif



int nbDownloadNewNetPDLFile(char** NetPDLDataBuffer, char* ErrBuf, int ErrBufSize)
{
	return DownloadNetPDLFile(NETPDL_DOWNLOAD_STRING, NetPDLDataBuffer, ErrBuf, ErrBufSize);
}


int nbDownloadNewNetPDLSchemaFile(char** NetPDLSchemaDataBuffer, char** NetPDLSchemaMinDataBuffer, char* ErrBuf, int ErrBufSize)
{
int RetVal;

	RetVal= DownloadNetPDLFile(NETPDLSCHEMA_DOWNLOAD_STRING, NetPDLSchemaDataBuffer, ErrBuf, ErrBufSize);

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	return DownloadNetPDLFile(NETPDLSCHEMAMIN_DOWNLOAD_STRING, NetPDLSchemaMinDataBuffer, ErrBuf, ErrBufSize);
}


int DownloadNetPDLFile(const char* DownloadString, char** NetPDLDataBuffer, char* ErrBuf, int ErrBufSize)
{
char DataBuffer[4096];
int ClientSocket;				// keeps the socket ID for this connection
struct addrinfo Hints;			// temporary struct to keep settings needed to open the new socket
struct addrinfo *AddrInfo;		// keeps the addrinfo chain; required to open a new socket
int WrittenBytes;				// Number of bytes read from the socket
int ReadBytes;					// Number of bytes read from the socket
char* TempFileName;
FILE* TempFileHandle;
int FileSize;
char *Ptr;

	// Prepare to open a new server socket
	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AF_UNSPEC;
	Hints.ai_socktype= SOCK_STREAM;	// Open a TCP connection

	if (sock_initaddress (NETPDL_DOWNLOAD_SITE, NETPDL_DOWNLOAD_PORT, &Hints, &AddrInfo, ErrBuf, ErrBufSize) == sockFAILURE)
		return nbFAILURE;
 
	if ( (ClientSocket= sock_open(AddrInfo, 0, 0, ErrBuf, ErrBufSize)) == sockFAILURE)
	{
		// AddrInfo is no longer required
		sock_freeaddrinfo(AddrInfo);
		return nbFAILURE;
	}

	// Connection established
	// Now, AddrInfo is no longer required
	sock_freeaddrinfo(AddrInfo);

	WrittenBytes= sock_send(ClientSocket, DownloadString, strlen(DownloadString), ErrBuf, ErrBufSize);
	if (WrittenBytes == sockFAILURE)
		return nbFAILURE;

	ReadBytes= sock_recv(ClientSocket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 30, ErrBuf, ErrBufSize);

	if (ReadBytes == sockFAILURE)
		return nbFAILURE;

	if (ReadBytes == sockWARNING)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"We waited for enough time and no data has been received so far. Aborting the connection.");
		return nbFAILURE;
	}

	// Check the response of the server, if it is 200 OK
	if (CheckServerResponse(DataBuffer, ReadBytes, ErrBuf, ErrBufSize) == nbFAILURE)
		return nbFAILURE;

	// Get temp file name
	TempFileName= tempnam(NULL, NULL);
	if (TempFileName == NULL)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Fatal error: cannot open a temp file to store the new NetPDL file.",
			ErrBuf, ErrBufSize);

		return nbFAILURE;
	}

// b: we have to use binary files in order to avoid problems with \n and such (fseek() does not work well with them)
#if defined(WIN32) || defined(WIN64)
	TempFileHandle= fopen(TempFileName, "w+bT");
#else
	TempFileHandle= fopen(TempFileName, "w+bT");
#endif
	if (TempFileHandle == NULL)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Fatal error: cannot open a temp file to store the new NetPDL file.",
			ErrBuf, ErrBufSize);

		return nbFAILURE;
	}

	if ((Ptr= strstr(DataBuffer, "<?xml version=\"1.0\"")) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not able to locate the beginning of the NetPDL data.");
		return nbFAILURE;
	}

	FileSize= ReadBytes - (Ptr - DataBuffer);

	if (fwrite(Ptr, sizeof(char), FileSize, TempFileHandle) != (size_t) FileSize)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Error writing data on disk.", ErrBuf, ErrBufSize);
		return nbFAILURE;
	}

	while(1)
	{
		ReadBytes= sock_recv(ClientSocket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 30, ErrBuf, ErrBufSize);

		if (ReadBytes == sockFAILURE)
			return nbFAILURE;

		// The transfer has been completed
		if (ReadBytes == sockWARNING)
			break;

		FileSize+= ReadBytes;
		if (fwrite(DataBuffer, sizeof(char), ReadBytes, TempFileHandle) != (size_t) ReadBytes)
		{
			nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
				"Error writing data on disk.", ErrBuf, ErrBufSize);
			return nbFAILURE;
		}
	}

	// Move pointer to the beginning of the file
	fseek(TempFileHandle, 0, SEEK_SET);

	// Check if this buffer has already been used. In this case, we need to clear this first.
	if (NetPDLDataFile)
		free(NetPDLDataFile);

	if ((NetPDLDataFile= (char*) malloc(FileSize + 1)) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, ERROR_ALLOC_FAILED);
		return nbFAILURE;
	}
	
	if (fread(NetPDLDataFile, sizeof(char), FileSize, TempFileHandle) != (size_t) FileSize)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Error reading temp data on disk.", ErrBuf, ErrBufSize);
		return nbFAILURE;
	}

	NetPDLDataFile[FileSize]= 0;

	// Check if current NetPDL version is compatible with the library
	if (CheckNetPDLVersion(ErrBuf, ErrBufSize) == nbFAILURE)
		return nbFAILURE;

	*NetPDLDataBuffer= NetPDLDataFile;

	// Close the temp file
	if (TempFileHandle)
	{
		fclose(TempFileHandle);
		// Now, let's remove the temp file from disk
		remove(TempFileName);
	}

	if (TempFileName)
		free(TempFileName);

	return nbSUCCESS;
}


int CheckServerResponse(char* DataBuffer, int ReadBytes, char* ErrBuf, int ErrBufSize)
{
char HeaderLine[1024];
char *Ptr;
int i;

	for (i= 0; i < ReadBytes; i++)
	{
		if (i >= (int) (sizeof(HeaderLine)))
			break;

		if ((DataBuffer[i] == '\r') || (DataBuffer[i] == '\n'))
			break;

		HeaderLine[i]= DataBuffer[i];
		HeaderLine[i + 1]= 0;
	}

	// Now, HeaderLine contains the header line.
	if (strnicmp(HeaderLine, "HTTP", strlen("HTTP")) != 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Bad responde from server. Is the server using HTTP?");
		return nbFAILURE;
	}

	if ((Ptr= strchr(HeaderLine, ' ')) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Bad responde from server. Is the server using HTTP?");
		return nbFAILURE;
	}

	if (strnicmp(Ptr + 1, "200 OK", strlen("200 OK")))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Bad responde from server: %s", Ptr + 1);
		return nbFAILURE;
	}

	return nbSUCCESS;
}


int CheckNetPDLVersion(char* ErrBuf, int ErrBufSize)
{
char NetPDLElementString[1024];
char *Ptr;
int i;
int VersionMajor, VersionMinor;

	if ((Ptr= strstr(NetPDLDataFile, "<netpdl")) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not able to locate the beginning of the NetPDL element.");
		return nbFAILURE;
	}

	for (i= 0; i < (int) sizeof(NetPDLElementString); i++)
	{
		if (*Ptr == '>')
			break;

		NetPDLElementString[i]= *Ptr;
		NetPDLElementString[i + 1]= 0;

		Ptr++;
	}

	// Now, NetPDLElementString contains the <netpdl...> tag.
	if ((Ptr= strstr(NetPDLElementString, NETPDL_VERSION_ATTRIBUTE)) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not able to locate the NetPDL version.");
		return nbFAILURE;
	}

	sscanf(Ptr + strlen(NETPDL_VERSION_ATTRIBUTE), "%d.%d", &VersionMajor, &VersionMinor);

	if ((VersionMajor == NETPDL_VERSION_MAJOR) && (VersionMinor == NETPDL_VERSION_MINOR))
		return nbSUCCESS;
	else
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Cannot load this version of the NetPDL file: the library supports version %d.%d,"
			"while the downloaded file has version '%d.%d'.",
			NETPDL_VERSION_MAJOR, NETPDL_VERSION_MINOR, VersionMajor, VersionMinor);
		return nbFAILURE;
	}
}


int nbDownloadAndUpdateNetPDLFile(const char* NetPDLFileLocation, char* ErrBuf, int ErrBufSize)
{
char* NetPDLDataBuffer;
FILE* NetPDLFileHandle;
char* TempNetPDLFileName;
int FileSize;

	// This function can be called also without having the NetBee library initialized
	// This is useful in case you deleted the netpdl database and you want to get a new one
	// from the online repository.
	if (nbIsInitialized() == nbFAILURE)
	{
		// Initialize Socket support
		if (sock_init(ErrBuf, ErrBufSize) == sockFAILURE)
			return nbFAILURE;
	}

	if (nbDownloadNewNetPDLFile(&NetPDLDataBuffer, ErrBuf, ErrBufSize) == nbFAILURE)
		return nbFAILURE;

	// Now, let's save this data into a temp file, so that we can check that the downloaded file loads correctly
	if ((TempNetPDLFileName= tempnam(NULL, NULL)) == NULL)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
		"Error creating temp file.", ErrBuf, ErrBufSize);
	}

	// T: Specifies a file as temporary. If possible, it is not flushed to disk. 
	// b: we have to use binary files in order to avoid problems with \n and such (fseek() does not work well with them)
	// Unfortunately, we cannot set the "D" flag ("w+bTD"), so that the file is not deleted automatically.
	// Since there are no standard functions to remove this file from disk, this must be deleted manually :-(
#if defined(WIN32) || defined(WIN64)
	NetPDLFileHandle= fopen(TempNetPDLFileName, "w+bT");
#else
	NetPDLFileHandle= fopen(TempNetPDLFileName, "w+b");
#endif
	if (NetPDLFileHandle == NULL)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Fatal error: cannot open a temp file to store the new NetPDL file.",
			ErrBuf, ErrBufSize);
		return nbFAILURE;
	}

	FileSize= strlen(NetPDLDataBuffer);

	if (fwrite(NetPDLDataBuffer, sizeof(char), FileSize, NetPDLFileHandle) != (size_t) FileSize)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
		"Error writing data in the temp file.", ErrBuf, ErrBufSize);
	}

	fclose(NetPDLFileHandle);

	if (nbUpdateNetPDLDescription(TempNetPDLFileName, ErrBuf, ErrBufSize) == nbFAILURE)
	{
		// Now the temp file is no longer required, so let's free it
		remove(TempNetPDLFileName);
		free(TempNetPDLFileName);

		// Ops.... the file does not load correctly. So, let's fall back
		// with the "default" one (otherwise, we're having no description
		// loaded at this time)
		nbUpdateNetPDLDescription(NULL, ErrBuf, ErrBufSize);

		return nbFAILURE;
	}

	// Now the temp file is no longer required, so let's free it
	remove(TempNetPDLFileName);
	free(TempNetPDLFileName);

	// If no errors occurred, it means that the content of the file is fine.
	// So, we can safely replace the old NetPDL file with the new one.

	if (NetPDLFileLocation)
	{
		NetPDLFileHandle= fopen(NetPDLFileLocation, "w");
		TempNetPDLFileName= (char*) NetPDLFileLocation;
	}
	else
	{
	char NetPDLFileDefaultLocation[1024];

		if (LocateLibraryLocation(NetPDLFileDefaultLocation, sizeof(NetPDLFileDefaultLocation), ErrBuf, ErrBufSize) == nbFAILURE)
			return nbFAILURE;

		NetPDLFileHandle= fopen(NetPDLFileDefaultLocation, "w");
		TempNetPDLFileName= NetPDLFileDefaultLocation;
	}

	if (NetPDLFileHandle == NULL)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Error opening NetPDL file.", ErrBuf, ErrBufSize);
		return nbFAILURE;
	}

	if (fwrite(NetPDLDataBuffer, sizeof(char), FileSize, NetPDLFileHandle) != (size_t) FileSize)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__,
			"Error writing data in the temp file.", ErrBuf, ErrBufSize);
	}

	fclose(NetPDLFileHandle);

	if (nbUpdateNetPDLDescription(TempNetPDLFileName, ErrBuf, ErrBufSize) == nbFAILURE)
		return nbFAILURE;

	return nbSUCCESS;
}

