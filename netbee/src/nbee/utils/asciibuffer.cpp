/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <xercesc/util/XMLString.hpp>

#include "asciibuffer.h"
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"


// Indicate using Xerces-C++ namespace in general
// This is required starting from Xerces 2.2.0
XERCES_CPP_NAMESPACE_USE


//! Initial size of this buffer
#define ASCII_BUFFER_START_SIZE 512000



//! Class constructor
CAsciiBuffer::CAsciiBuffer()
{
	m_asciiBuffer= NULL;
	memset(m_errbuf, 0, sizeof(m_errbuf));
};

//! Class destructor
CAsciiBuffer::~CAsciiBuffer()
{
	if (m_asciiBuffer)
		delete[] m_asciiBuffer;

	m_asciiBuffer= NULL;
};


/*!
	\brief Initialize the class.

	It allocates some internal parameters and checks that everything is fine.
	This method must be called after class constructor.

	\return nbSUCCESS if everyting is fine, nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CAsciiBuffer::Initialize()
{
	m_bufferCurrentIndex= 0;
	m_bufferTotSize= ASCII_BUFFER_START_SIZE * 10;

	// Allocate ascii buffer; XMLString functions require a '+1'
	m_asciiBuffer= new char [m_bufferTotSize + 1];
	if (m_asciiBuffer == NULL)
	{
		m_bufferTotSize= 0;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate temporary ascii buffer.");
		return nbFAILURE;
	}

	m_asciiBuffer[0]= 0;
	m_asciiBuffer[1]= 0;
	m_asciiBuffer[m_bufferTotSize]= 0;
	m_asciiBuffer[m_bufferTotSize - 1]= 0;

	m_resizeAllowed= false;
	m_appendInProgress= false;

	return nbSUCCESS;
}


/*!
	\brief Clears the content of the internal buffer.

	\param ResizeAllowed: 'true' if the internal buffer can be resized in case
	it is not enough to contain everything, 'false' if it can't.
*/
void CAsciiBuffer::ClearBuffer(bool ResizeAllowed)
{
	m_bufferCurrentIndex= 0;

	m_asciiBuffer[0]= 0;
	m_asciiBuffer[1]= 0;

	m_resizeAllowed= ResizeAllowed;
	m_appendInProgress= false;
};


/*!
	\brief Updates the number of chars (counting also the terminator character) present in the internal buffer.

	\param BufferCurrentIndex: total number of bytes (in the buffer) that contains valid data
	(counting also the last '\\0', which terminates the buffer).
*/
void CAsciiBuffer::SetBufferCurrentSize(unsigned long BufferCurrentIndex) 
{
	if (BufferCurrentIndex < m_bufferTotSize)
		m_bufferCurrentIndex= BufferCurrentIndex;
	else
		m_bufferCurrentIndex= m_bufferTotSize;

	// let's put the terminator at the end, just in case
	m_asciiBuffer[m_bufferCurrentIndex]= 0;
};


/*!
	\brief Transcode a UNICODE buffer, and store the new string into an internal buffer.

	This function uses some of the space remaining in the internal variable m_asciiBuffer
	to store some new string (in ascii), after translating it from Unicode.

	This function will store all data in separate strings (i.e. a '\\0' is inserted 
	between any string).

	\param SourceString: pointer to the string that has to be transcoded and stored in the internal buffer.

	\return A pointer to the appended string, NULL if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
char *CAsciiBuffer::TranscodeAndStore(wchar_t *SourceString)
{
unsigned long PointerToString;

	// Chech if previously we were appending data. In this case, let's terminate
	// the string properly 
	if (m_appendInProgress)
	{
		m_appendInProgress= false;

		if ( (m_bufferCurrentIndex + 1) >= m_bufferTotSize)
		{
			if (ExpandBuffer() == nbFAILURE)
				return NULL;
		}

		m_bufferCurrentIndex++;
		m_asciiBuffer[m_bufferCurrentIndex] = 0;
	}

	PointerToString= m_bufferCurrentIndex;

	XMLString::transcode((XMLCh *)SourceString, &m_asciiBuffer[m_bufferCurrentIndex], m_bufferTotSize - m_bufferCurrentIndex, XMLPlatformUtils::fgMemoryManager);
	m_bufferCurrentIndex= m_bufferCurrentIndex + strlen( &m_asciiBuffer[m_bufferCurrentIndex] ) + 1;
	// Just in case
	m_asciiBuffer[m_bufferCurrentIndex]= 0;

	//! \bug In case we have to expand the buffer, the string that exceeds the size of the buffer is not formatted appropriately
	if (m_bufferCurrentIndex >= m_bufferTotSize)
	{
		if (ExpandBuffer() == nbFAILURE)
			return NULL;
	}

	return &m_asciiBuffer[PointerToString];
};


/*!
	\brief Store the given string into an internal buffer.

	This function uses some of the space remaining in the internal variable m_asciiBuffer
	to store the new string.

	This function will store all data in separate strings (i.e. a '\\0' is inserted 
	between any string).

	\param SourceString: pointer to the string that has to be stored in the internal buffer.

	\return A pointer to the appended string, NULL if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
char *CAsciiBuffer::Store(const char *SourceString)
{
unsigned long PointerToString;
unsigned long Count;

	// Check if previously we were appending data. In this case, let's terminate
	// the string properly 
	if (m_appendInProgress)
	{
		m_appendInProgress= false;

		if ( (m_bufferCurrentIndex + 1) >= m_bufferTotSize)
		{
			if (ExpandBuffer() == nbFAILURE)
				return NULL;
		}

		m_bufferCurrentIndex++;
		m_asciiBuffer[m_bufferCurrentIndex] = 0;
	}

	PointerToString= m_bufferCurrentIndex;

	// Let's put to 'NULL' the cell in which data has to be copied
	// (for instance, sstrncat() requires the string to be NULL terminated)
	m_asciiBuffer[m_bufferCurrentIndex]= 0;

	Count= sstrncat(&m_asciiBuffer[m_bufferCurrentIndex], SourceString, m_bufferTotSize - m_bufferCurrentIndex);

	m_bufferCurrentIndex= m_bufferCurrentIndex + Count + 1;
	// Just in case
	m_asciiBuffer[m_bufferCurrentIndex]= 0;

	//! \bug In case we have to expand the buffer, the string that exceeds the size of the buffer is not formatted appropriately
	if (m_bufferCurrentIndex >= m_bufferTotSize)
	{
		if (ExpandBuffer() == nbFAILURE)
			return NULL;
	}

	return &m_asciiBuffer[PointerToString];
};


/*!
	\brief Convert a 'long' into a string, ans stores it into an internal buffer.

	This function uses some of the space remaining in the internal variable m_asciiBuffer
	to convert a number into a string and store it.

	This function will store all data in separate strings (i.e. a '\\0' is inserted 
	between any string).

	\param Number: number that has to be converted and stored in the internal buffer.

	\return A pointer to the appended string, NULL if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
char *CAsciiBuffer::StoreLong(long Number)
{
unsigned long PointerToString;
unsigned long Count;

	// Chech if previously we were appending data. In this case, let's terminate
	// the string properly 
	if (m_appendInProgress)
	{
		m_appendInProgress= false;

		if ( (m_bufferCurrentIndex + 1) >= m_bufferTotSize)
		{
			if (ExpandBuffer() == nbFAILURE)
				return NULL;
		}

		m_bufferCurrentIndex++;
		m_asciiBuffer[m_bufferCurrentIndex] = 0;
	}

	PointerToString= m_bufferCurrentIndex;

	Count= ssnprintf(&m_asciiBuffer[m_bufferCurrentIndex], m_bufferTotSize - m_bufferCurrentIndex, "%d", Number);

	m_bufferCurrentIndex= m_bufferCurrentIndex + Count + 1;
	// Just in case
	m_asciiBuffer[m_bufferCurrentIndex]= 0;

	//! \bug In case we have to expand the buffer, the string that exceeds the size of the buffer is not formatted appropriately
	if (m_bufferCurrentIndex >= m_bufferTotSize)
	{
		if (ExpandBuffer() == nbFAILURE)
			return NULL;
	}

	return &m_asciiBuffer[PointerToString];
};


/*!
	\brief It replaces the strncat(); it appends a given source string to the internal m_asciiBuffer buffer.

	This function prevents buffer overruns. This function guarantees that the returned 
	string is always correctly terminated ('\\0').

	This function will append the new string to the previously existing data in the 
	internal buffer; no '\\0' characters are appended between the strings.

	The first time it is called, this function sets an internal flag in order to set
	that an 'append' operation is in progress. Therefore, following calls to this function
	lead to append further strings to the previous ones.
	This flag is 'unset' as soon as another function (such as Store()) is called which
	do not require to continue appending data.

	\param SourceString: String to be appended

	\return The total number of characters present in the buffer (not counting the last '\\0' 
	character), or nbFAILURE if the size of the buffer was not enough to contain
	the resulting string.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CAsciiBuffer::Append(const char *SourceString)
{
int WrittenBytes;

	m_appendInProgress= true;

	WrittenBytes= -1;
	while (WrittenBytes == -1)
	{
		WrittenBytes= sstrncat(&m_asciiBuffer[m_bufferCurrentIndex], SourceString, m_bufferTotSize - m_bufferCurrentIndex - 1);

		if (WrittenBytes == -1)
		{
			if (ExpandBuffer() == nbFAILURE)
				return nbFAILURE;
		}
	}

	if (WrittenBytes > 0)
		m_bufferCurrentIndex+= WrittenBytes;

	m_asciiBuffer[m_bufferCurrentIndex] = 0;

	return m_bufferCurrentIndex;
}


/*!
	\brief It replaces the strncat(); it appends a given source string to the internal m_asciiBuffer buffer.

	This function prevents buffer overruns. This function guarantees that the returned 
	string is always correctly terminated ('\\0').

	This function will append the new string to the previously existing data in the 
	internal buffer; no '\\0' characters are appended between the strings.

	The first time it is called, this function sets an internal flag in order to set
	that an 'append' operation is in progress. Therefore, following calls to this function
	lead to append further strings to the previous ones.
	This flag is 'unset' as soon as another function (such as Store()) is called which
	do not require to continue appending data.

	\param SourceString: String to be appended.
	\param NumberofChars: Number of characters present in previous string (which may not 
	be zero-terminated).

	\return The total number of characters present in the buffer (not counting the last '\\0' 
	character), or nbFAILURE if the size of the buffer was not enough to contain
	the resulting string.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CAsciiBuffer::Append(const char *SourceString, int NumberofChars)
{
int WrittenBytes;

	m_appendInProgress= true;

	if (NumberofChars == 0)
		return m_bufferCurrentIndex;

	WrittenBytes= -1;
	while (WrittenBytes == -1)
	{

		// "+1" because we have to allocate also the \0 at the end of the string
		WrittenBytes= sstrncat(&m_asciiBuffer[m_bufferCurrentIndex], SourceString, NumberofChars + 1);

		if (WrittenBytes == -1)
		{
			if (ExpandBuffer() == nbFAILURE)
				return nbFAILURE;
		}
	}

	if (WrittenBytes > 0)
		m_bufferCurrentIndex+= WrittenBytes;

	m_asciiBuffer[m_bufferCurrentIndex] = 0;

	return m_bufferCurrentIndex;
}


/*!
	\brief It replaces the snprintf(); it formats a string according to a given format, and it appends 
	everything to the internal m_asciiBuffer buffer.

	This function has the same parameters of the snprintf(), but it prevents
	buffer overruns. This function guarantees that the returned string is always 
	correctly terminated ('\\0').

	This function will append the new string to the previously existing data in the 
	internal buffer; no '\\0' characters are appended between the strings.

	\param Format: format-control string.

	\return The total numer of characters present in the buffer (not counting the last '\\0' 
	character), or nbFAILURE if the size of the buffer was not enough to contain
	the resulting string.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CAsciiBuffer::AppendFormatted(const char *Format, ...)
{
int WrittenBytes;
va_list Args;
va_start(Args, Format);

	m_appendInProgress= true;

	WrittenBytes= -1;
	while (WrittenBytes == -1)
	{
		WrittenBytes= vsnprintf(&m_asciiBuffer[m_bufferCurrentIndex], m_bufferTotSize - m_bufferCurrentIndex - 1, Format, Args);

		if (WrittenBytes == -1)
		{
			if (ExpandBuffer() == nbFAILURE)
				return nbFAILURE;
		}
	}

	if (WrittenBytes > 0)
		m_bufferCurrentIndex+= WrittenBytes;

	m_asciiBuffer[m_bufferCurrentIndex] = 0;

	return m_bufferCurrentIndex;
}


/*!
	\brief In case the size of the m_asciiBuffer internal buffer is not enough, expands the buffer.

	\return nbSUCCESS if everthing is fine, nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CAsciiBuffer::ExpandBuffer()
{

	if (m_resizeAllowed)
	{
		char *NewAsciiBuffer;

		// Allocate ascii buffer
		NewAsciiBuffer= new char [m_bufferTotSize * 10 + 1];
		if (NewAsciiBuffer == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to enlarge ascii buffer.");
			return nbFAILURE;
		}
		// If everything is fine, let's use the new buffer.
		memcpy(NewAsciiBuffer, m_asciiBuffer, m_bufferTotSize + 1);

		// Delete old buffer and replace it with the new one
		delete m_asciiBuffer;
		m_asciiBuffer= NewAsciiBuffer;

		m_bufferTotSize*= 10;

		m_asciiBuffer[m_bufferTotSize]= 0;
		m_asciiBuffer[m_bufferTotSize - 1]= 0;

		return nbSUCCESS;
	}
	else
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
			"Not enough memory in the ascii buffer and re-allocation not permitted.");
		return nbFAILURE;
	}
}

