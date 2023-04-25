/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include <stdio.h>
#include <stdarg.h>


/*!
	\brief This class implements some functions needed to manage strings and to prevent buffer overruns.
*/
class CAsciiBuffer
{
public:
	CAsciiBuffer();
	virtual ~CAsciiBuffer();

	int Initialize();

	void ClearBuffer(bool ResizeAllowed);

	char *TranscodeAndStore(wchar_t *SourceString);
	char *Store(const char *SourceString);
	char *StoreLong(long Number);

	int Append(const char *SourceString);
	int Append(const char *SourceString, int NumberofChars);
	int AppendFormatted(const char *Format, ...);

	void SetBufferCurrentSize(unsigned long BufferCurrentIndex);

	/*!
		\brief Returns a pointer to the beginning of the internal buffer that is used to store data.

		\warning In case you want use this function to manage manually the content of the internal buffer,
		please keep in mind that the internal variables private to this class are not updated
		in this case. For instance, this class cannot know the level of occupancy
		of the buffer if the user fills the buffer manually.
	*/
	char *GetStartBufferPtr() {return m_asciiBuffer; };

	/*!
		\brief Returns a pointer to the beginning of the internal buffer that is currently free.

		You can use this pointer in case you want to insert your data manually in the buffer.

		\warning In case you want use this function to manage manually the content of the internal buffer,
		please keep in mind that the internal variables private to this class are not updated
		in this case. For instance, this class cannot know the level of occupancy
		of the buffer if the user fills the buffer manually.
		In this case, you may use the function SetBufferCurrentSize() in order to update this variable.
	*/
	char *GetStartFreeBufferPtr() {return &m_asciiBuffer[m_bufferCurrentIndex]; };

	/*!
		\brief Returns the total size of the internal buffer.
		\return Return the total size of the internal buffer.
	*/
	unsigned long GetBufferTotSize() {return m_bufferTotSize; };

	/*!
		\brief Returns the number of chars (not counting the last '\\0') present in the internal buffer.
		\return Returns the number of chars (not counting the last '\\0') present in the internal buffer.
	*/
	unsigned long GetBufferCurrentSize() {return m_bufferCurrentIndex; };

	/*! 
		\brief Returns a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	char *GetLastError() { return m_errbuf; }

private:
	int ExpandBuffer();

	char *m_asciiBuffer;					//!< Buffer that will contain the data.
	unsigned long m_bufferCurrentIndex;		//!< Index of the first free cell in the buffer.
	unsigned long m_bufferTotSize;			//!< Current total size of the buffer.
	bool m_resizeAllowed;					//!< 'true' if we can resize the internal ascii buffer
	bool m_appendInProgress;				//!< 'true' if we're currently appending data
	char m_errbuf[2048];					//!< Buffer that keeps the error message (if any)
};

