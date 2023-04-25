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

// Currently this function is not used, but let's keep here just in case we need to make some more debugging
int errorsnprintf(char *File, char *Function, int Line, char* Buffer, int BufSize, char *Format, ...)
{
int WrittenBytes;
va_list Args;

	va_start(Args, Format);

	if (Buffer == NULL)
		return -1;

#if defined(_WIN32) && (_MSC_VER >= 1400)	// Visual Studio 2005
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


void nbPrintError(char *File, char *Function, int Line, char* Buffer)
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

