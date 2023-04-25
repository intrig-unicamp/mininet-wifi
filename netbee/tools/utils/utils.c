/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>			// for strerror()
#include <ctype.h>			// for toupper()
#include <errno.h>			// for the errno variable

#ifdef WIN32
#include <windows.h>		// for FormatMessage()
#include <lmerr.h>			// for the error handler
#endif

#include "debug.h"


int ssnprintf(char* Buffer, int BufSize, const char *Format, ...)
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

	Buffer[BufSize - 1] = 0;

	return WrittenBytes;
};


int sstrncat(char *Buffer, char *Source, int BufSize)
{
char *EndPos;
char *Start;

	Start= Buffer;
	EndPos= &Buffer[BufSize - 1];

	while ((*Buffer) && (Buffer++ < EndPos));

	if (Source == NULL)
		return 0;

	while ((Buffer < EndPos) && (*Source) )
	{
		*Buffer= *Source;
		Buffer++;
		Source++;
	}

	*Buffer= 0;
	return (int) (Buffer - Start);
};


void sstrncat_ex(char *Buffer, int BufSize, int *BufOccupancy, const char *Source)
{
char *EndPos;
char *Start;

	if (*BufOccupancy >= BufSize)
		return;

	if (Source == NULL)
		return;

	Start= Buffer;
	EndPos= &Buffer[BufSize - 1];
	Buffer= &Buffer[*BufOccupancy];

	while ((Buffer < EndPos) && (*Source) )
	{
		*Buffer= *Source;
		Buffer++;
		Source++;
	}

	*Buffer= 0;
	*BufOccupancy= (int) (Buffer - Start);
};


void sstrncatescape_ex(char *Buffer,  int BufSize, int *BufOccupancy, char CharToBeEscaped, char EscapeChar, const char *Source)
{
char *EndPos;
char *Start;

	if (*BufOccupancy >= BufSize)
		return;

	if (Source == NULL)
		return;

	Start= Buffer;
	EndPos= &Buffer[BufSize - 1];
	Buffer= &Buffer[*BufOccupancy];

	while ((Buffer < EndPos) && (*Source) )
	{
		if (*Source == CharToBeEscaped)
		{
			*Buffer= EscapeChar;
			Buffer++;
			if (Buffer < EndPos)
			{
				*Buffer= *Source;
				Buffer++;
				Source++;
			}

		}
		else
		{
			*Buffer= *Source;
			Buffer++;
			Source++;
		}
	}

	*Buffer= 0;
	*BufOccupancy= (int) (Buffer - Start);
};

/*
int sstrncatescape(char *Buffer, char *Source, int BufSize, char EscapeChar, char AddEscapeChar)
{
char *EndPos;
char *Start;

	Start= Buffer;
	EndPos= &Buffer[BufSize - 1];

	while ((*Buffer) && (Buffer++ < EndPos));

	if (Source == NULL)
		return 0;

	while ((Buffer < EndPos) && (*Source) )
	{
		if (*Source == EscapeChar)
		{
			*Buffer= AddEscapeChar;
			Buffer++;
			if (Buffer < EndPos)
			{
				*Buffer= *Source;
				Buffer++;
				Source++;
			}

		}
		else
		{
			*Buffer= *Source;
			Buffer++;
			Source++;
		}
	}

	*Buffer= 0;
	return (int) (Buffer - Start);
};
*/


int sstrncpy(char *Destination, char *Source, int DestSize)
{
char *EndPos;
char *Start;

	Start= Destination;
	EndPos= &Destination[DestSize - 1];

	if (Source == NULL)
		return 0;

	while ((Destination < EndPos) && (*Destination++= *Source++) );

	*Destination= 0;
	return (int) (Destination - Start - 1);
};


// Courtesy from http://c.snippets.org/snip_lister.php?fname=stristr.c
char *stristr(const char *String, const char *Pattern)
{
char *pptr, *sptr, *start;

	for (start = (char *)String; *start != 0; start++)
	{
		/* find start of pattern in string */
		for ( ; ((*start!= 0) && (toupper(*start) != toupper(*Pattern))); start++)
			;
		if (*start == 0)
			return NULL;

		pptr = (char *)Pattern;
		sptr = (char *)start;

		while (toupper(*sptr) == toupper(*pptr))
		{
			sptr++;
			pptr++;

			/* if end of pattern then pattern was found */

			if (*pptr == 0)
				return (start);
		}
	}
	return NULL;
}


void nbGetLastError(const char *CallerString, char *ErrBuf, int ErrBufSize)
{
#ifdef WIN32
	int RetVal;
	int ErrorCode;
	TCHAR Message[2048];	// It will be char (if we're using ascii) or wchar_t (if we're using unicode)
    DWORD FormatFlags;
    HMODULE ModuleHandle = NULL; // default to system source

		ErrorCode= GetLastError();

		FormatFlags= FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS |
						FORMAT_MESSAGE_MAX_WIDTH_MASK;

		// Let's check if ErrorCode is in the network range, load the message source.
		// This will help printing error messages related to network services
		if (ErrorCode >= NERR_BASE && ErrorCode <= MAX_NERR)
		{
			ModuleHandle = LoadLibraryEx(TEXT("netmsg.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);

			if (ModuleHandle != NULL)
				FormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
		}

		// Call FormatMessage() to allow for message text to be acquired from the system 
		// or from the supplied module handle.
		RetVal= FormatMessage(FormatFlags,
	                  ModuleHandle, // module to get message from (NULL == system)
					  ErrorCode,
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	                  Message,
					  sizeof(Message) / sizeof(TCHAR),
					  NULL);

		// If we loaded a message source, unload it.
		if (ModuleHandle != NULL)
			FreeLibrary(ModuleHandle);

		if (RetVal == 0)
		{
			if (ErrBuf)
			{
				if ( (CallerString) && (*CallerString) )
					ssnprintf(ErrBuf, ErrBufSize, "%sUnable to get the exact error message", CallerString);
				else
					ssnprintf(ErrBuf, ErrBufSize, "Unable to get the exact error message");

				ErrBuf[ErrBufSize - 1]= 0;
			}

			return;
		}
	
		if (ErrBuf)
		{
			if ( (CallerString) && (*CallerString) )
				ssnprintf(ErrBuf, ErrBufSize, "%s%s (code %d)", CallerString, Message, ErrorCode);
			else
				ssnprintf(ErrBuf, ErrBufSize, "%s (code %d)", Message, ErrorCode);

			ErrBuf[ErrBufSize - 1]= 0;
		}

#else
	char *Message;
	
		Message= strerror(errno);

		if (ErrBuf)
		{
			if ( (CallerString) && (*CallerString) )
				ssnprintf(ErrBuf, ErrBufSize, "%s%s (code %d)", CallerString, Message, errno);
			else
				ssnprintf(ErrBuf, ErrBufSize, "%s (code %d)", Message, errno);

			ErrBuf[ErrBufSize - 1]= 0;
		}
#endif
}


void nbGetLastErrorEx(char *File, char *Function, int Line, const char *CallerString, char *ErrBuf, int ErrBufSize)
{
#ifdef WIN32
	int RetVal;
	int ErrorCode;
	TCHAR Message[2048];	// It will be char (if we're using ascii) or wchar_t (if we're using unicode)
    DWORD FormatFlags;
    HMODULE ModuleHandle = NULL; // default to system source

		ErrorCode= GetLastError();

		FormatFlags= FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS |
						FORMAT_MESSAGE_MAX_WIDTH_MASK;

		// Let's check if ErrorCode is in the network range, load the message source.
		// This will help printing error messages related to network services
		if (ErrorCode >= NERR_BASE && ErrorCode <= MAX_NERR)
		{
			ModuleHandle = LoadLibraryEx(TEXT("netmsg.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);

			if (ModuleHandle != NULL)
				FormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
		}

		// Call FormatMessage() to allow for message text to be acquired from the system 
		// or from the supplied module handle.
		RetVal= FormatMessage(FormatFlags,
	                  ModuleHandle, // module to get message from (NULL == system)
					  ErrorCode,
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	                  Message,
					  sizeof(Message) / sizeof(TCHAR),
					  NULL);

		// If we loaded a message source, unload it.
		if (ModuleHandle != NULL)
			FreeLibrary(ModuleHandle);

		if (RetVal == 0)
		{
			if (ErrBuf)
			{
#ifdef _DEBUG
				if ( (CallerString) && (*CallerString) )
					ssnprintf(ErrBuf, ErrBufSize, "%s:%d (%s()): %sUnable to get the exact error message", File, Line, Function, CallerString);
				else
					ssnprintf(ErrBuf, ErrBufSize, "%s:%d (%s()): %s Unable to get the exact error message", File, Line, Function);
#else
				if ( (CallerString) && (*CallerString) )
					ssnprintf(ErrBuf, ErrBufSize, "%sUnable to get the exact error message", CallerString);
				else
					ssnprintf(ErrBuf, ErrBufSize, "Unable to get the exact error message");
#endif

				ErrBuf[ErrBufSize - 1]= 0;
			}

			return;
		}
	
		if (ErrBuf)
		{
#ifdef _DEBUG
			if ( (CallerString) && (*CallerString) )
				ssnprintf(ErrBuf, ErrBufSize, "%s:%d (%s()): %s%s (code %d)", File, Line, Function, CallerString, Message, ErrorCode);
			else
				ssnprintf(ErrBuf, ErrBufSize, "%s:%d (%s()): %s (code %d)", File, Line, Function, Message, ErrorCode);
#else
			if ( (CallerString) && (*CallerString) )
				ssnprintf(ErrBuf, ErrBufSize, "%s%s (code %d)", CallerString, Message, ErrorCode);
			else
				ssnprintf(ErrBuf, ErrBufSize, "%s (code %d)", Message, ErrorCode);
#endif

			ErrBuf[ErrBufSize - 1]= 0;
		}

#else
	char *Message;
	
		Message= strerror(errno);

		if (ErrBuf)
		{
#ifdef _DEBUG
			if ( (CallerString) && (*CallerString) )
				ssnprintf(ErrBuf, ErrBufSize, "%s:%d (%s()): %s%s (code %d)", File, Line, Function, CallerString, Message, errno);
			else
				ssnprintf(ErrBuf, ErrBufSize, "%s:%d (%s()): %s (code %d)", File, Line, Function, Message, errno);
#else
			if ( (CallerString) && (*CallerString) )
				ssnprintf(ErrBuf, ErrBufSize, "%s%s (code %d)", CallerString, Message, errno);
			else
				ssnprintf(ErrBuf, ErrBufSize, "%s (code %d)", Message, errno);
#endif

			ErrBuf[ErrBufSize - 1]= 0;
		}
#endif
}
