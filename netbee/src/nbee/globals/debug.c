/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "debug.h"

#if (defined(_WIN32) && defined(_MSC_VER))
#include <crtdbg.h>				// for _CrtDbgReport
#endif


#ifdef ENABLE_NETVM_LOGGING

void logdata(int LogScope, const char *Format, ...)
{
char Buffer[2028];
va_list Args;
FILE *LogFile;

	LogFile= NULL;
	va_start(Args, Format);

#if defined(_WIN32) && (_MSC_VER >= 1400)	/* Visual Studio 2005 */
	_vsnprintf_s(Buffer, sizeof(Buffer) - 1, _TRUNCATE, Format, Args);
#else
	vsnprintf(Buffer, sizeof(Buffer) - 1, Format, Args);
#endif

	// Terminate buffer, just in case
	Buffer[sizeof(Buffer) - 1] = 0;

	switch (LogScope)
	{
		case LOG_NETIL_INTERPRETER:
		{
			LogFile= fopen("log-netil.txt", "a+");
			break;
		}

		case LOG_JIT_BUILD_BLOCK_LVL2:
		{
			LogFile= fopen("log-buildingblocks.txt", "a+");
			break;
		}

		case LOG_JIT_LISTS_DEBUG_LVL:
		{
			LogFile= fopen("log-jitlists.txt", "a+");
			break;
		}

		case LOG_BYTECODE_LOAD_SAVE:
		{
			LogFile= fopen("log-bytecode-loadsave.txt", "a+");
			break;
		}

		case LOG_RUNTIME_CREATE_PEGRAPH:
		{
			LogFile= fopen("log-runtime-creation.txt", "a+");
			break;
		}
		default:
		{
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "\nError logging data: cannot find file for that scope.");
		}
	}

	if (LogFile)
	{
		fprintf(LogFile, "%s\n", Buffer);
		fclose(LogFile);
	}
	else
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "\nError logging data: cannot open log file.");
}
#endif


int errorsnprintf(const char *File, const char *Function, int Line, char* Buffer, int BufSize, const char *Format, ...)
{
int WrittenBytes;
va_list Args;

	va_start(Args, Format);

	if (Buffer == NULL)
		return -1;

#if defined(_WIN32) && (_MSC_VER >= 1400)	/* Visual Studio 2005 */
	WrittenBytes= _vsnprintf_s(Buffer, BufSize - 1, _TRUNCATE, Format, Args);
#else
	WrittenBytes= vsnprintf(Buffer, BufSize - 1, Format, Args);
#endif

	if (BufSize >= 1)
		Buffer[BufSize - 1] = 0;
	else
		Buffer[0]= 0;

#ifdef _DEBUG
	nbPrintError(File, Function, Line, Buffer);
#endif

	return WrittenBytes;
};


void nbPrintError(const char *File, const char *Function, int Line, char* Buffer)
{
	//	FILE *fp;
	//		fp= fopen("log.txt", "a+");
	//		fprintf(fp, "%s:%d (%s()): %s", File, Line, Function, Buffer);
	//		fclose(fp);

#ifdef _DEBUG
	fprintf(stderr, "%s:%d (%s()): %s\n", File, Line, Function, Buffer);
#else
	fprintf(stderr, "%s", Buffer);
#endif

#if (defined(_WIN32) && defined(_DEBUG))
	// _CRT_ERROR create a debug window on screen, while
	// _CRT_WARN prints the string in the output window of the Visual Studio IDE
	_CrtDbgReport(_CRT_WARN, File, Line, Function, Buffer);
#endif
}


int errorprintf(const char *File, const char *Function, int Line, const char *Format, ...)
{
int WrittenBytes;
va_list Args;
// The following variables are required to cope in case the called does not specify a valid buffer
// (e.g. because we're in debug mode and we want only to print some info on the screen)
char Buffer[2048];

	va_start(Args, Format);

#if defined(_WIN32) && (_MSC_VER >= 1400)	/* Visual Studio 2005 */
	WrittenBytes= _vsnprintf_s(Buffer, sizeof(Buffer) - 1, _TRUNCATE, Format, Args);
#else
	WrittenBytes= vsnprintf(Buffer, sizeof(Buffer) - 1, Format, Args);
#endif

	Buffer[sizeof(Buffer) - 1] = 0;

	nbPrintError(File, Function, Line, Buffer);

	return WrittenBytes;
};



// Prints debugging information
void nbPrintDebugLine(const char* Msg, unsigned int DebugType, const char *File, const char *Function, int Line, unsigned int Indentation)
{
	char buf[10240];
	char *p=buf;
	char source[1024];
	char *s=source;
	unsigned int i=0;

	p+=sprintf(p, "\n");

	for (; i<Indentation; i++)
		p+=sprintf(p, "  ");

	switch (DebugType)
	{
		case DBG_TYPE_INFO:
			p+=sprintf(p, "~ info ~ ");
			break;
		case DBG_TYPE_ASSERT:
			p+=sprintf(p, ": ASSRT: ");
			break;
		case DBG_TYPE_WARNING:
			p+=sprintf(p, "@ WARN @ ");
			break;
		case DBG_TYPE_ERROR:
			p+=sprintf(p, "# ERR ## ");
			break;
	}


	if (File!=NULL || Function!=NULL || Line!=-1)
	{
		int len;
		int maxFileLength=30;
		int maxSourceLength=50;
		if (File!=NULL)
		{
			len=(int)strlen(File)-maxFileLength;
			if (len > 0)
				s+=sprintf(s, "..%s ", File+len);
			else
				s+=sprintf(s, "%s ", File);
		}

		if (Line!=-1)
			s+=sprintf(s, "%d ", Line);
		if (Function!=NULL)
			s+=sprintf(s, "%s ", Function);

		*(s-1)='\0';	// remove last blankspace

		len=(int)strlen(source)-maxSourceLength;
		if (len > 2)
		p+=sprintf(p, "<..%50s>", source+len);
		else
		p+=sprintf(p, "<%52s>", source);
	}

	sprintf(p, ": %s", Msg);

	switch (DebugType)
	{
		case DBG_TYPE_INFO:
			printf("%s", buf);
			break;
		case DBG_TYPE_ASSERT:
//#if (defined(_WIN32) && defined(_DEBUG))
//			_CrtDbgReport(_CRT_ASSERT, NULL, 0, NULL, buf);
//#endif
			printf("%s", buf);
			break;
		case DBG_TYPE_WARNING:
//#if (defined(_WIN32) && defined(_DEBUG))
//			_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, buf);
//#endif
			printf("%s", buf);
			break;
		case DBG_TYPE_ERROR:
#if (defined(_WIN32) && defined(_DEBUG))
			_CrtDbgReport(_CRT_ERROR, NULL, 0, NULL, buf);
#endif
			printf("%s", buf);
			break;
	}
}

