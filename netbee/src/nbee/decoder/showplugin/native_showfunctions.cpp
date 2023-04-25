/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "native_showfunctions.h"
#include "../../globals/globals.h"
#include "../../globals/debug.h"


// it takes into account of the '\\0' at tne end of the string
int NativePrintIPv4Address(const unsigned char *FieldPacketPtr, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize)
{
unsigned long FormattedDigitSize;
unsigned long FormattedBufferOffset;
unsigned long Value;
int i;

	FormattedBufferOffset= 0;

	for (i= 0; i < 4; i++)
	{
		Value= (int) FieldPacketPtr[i];

		if (Value < 10)
			FormattedDigitSize= 1;
		else if (Value < 100)
				FormattedDigitSize= 2;
		else
				FormattedDigitSize= 3;

		// '+1' is tue to the possible "dot" after each digit
		if ((FormattedBufferOffset + FormattedDigitSize + 1) > FormattedFieldSize)
		{
			*FormattedFieldLength= 0;
			FormattedField[0]= 0;
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Internal error: the buffer required to print field value is too small.");
			return nbFAILURE;
		}

		uint2a(Value, &FormattedField[FormattedBufferOffset]);

		FormattedBufferOffset+= FormattedDigitSize;

		if (i != 3)
			FormattedField[FormattedBufferOffset] = '.';
		else
			FormattedField[FormattedBufferOffset] = '\0';

		FormattedBufferOffset++;
	}

	*FormattedFieldLength= FormattedBufferOffset;
	return nbSUCCESS;
}


int NativePrintAsciiField(const unsigned char *FieldPacketPtr, unsigned long FieldSize, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize)
{
	// '+1' because we need to take into account also the '\0' at the end
	if ( (FieldSize + 1) > FormattedFieldSize)
	{
		*FormattedFieldLength= 0;
		FormattedField[0]= 0;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Internal error: the buffer required to print field value is too small.");
		return nbFAILURE;
	}

	memcpy(FormattedField, FieldPacketPtr, FieldSize);
	FormattedField[FieldSize] = 0;

	*FormattedFieldLength= FieldSize + 1;
	return nbSUCCESS;
}


int NativePrintAsciiLineField(const unsigned char *FieldPacketPtr, unsigned long FieldSize, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize)
{
	// '+1' because we need to take into account also the '\0' at the end
	// Not very precise, since we may get rid of the '\r\n' at the end, but I think is enough here
	if ( (FieldSize + 1) > FormattedFieldSize)
	{
		*FormattedFieldLength= 0;
		FormattedField[0]= 0;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Internal error: the buffer required to print field value is too small.");
		return nbFAILURE;
	}

	// In case of wrong data in the packet, we may have less than 2 bytes
	// (which is very strange, but it may happen)
	if (FieldSize > 2)
	{
		memcpy(FormattedField, FieldPacketPtr, FieldSize);
		FormattedField[FieldSize] = 0;
	}

	if ((FieldPacketPtr[FieldSize - 2] == '\r') || (FieldPacketPtr[FieldSize - 2] == '\n'))
	{
		FormattedField[FieldSize - 2] = 0;
		*FormattedFieldLength= FieldSize - 2 + 1;
		return nbSUCCESS;
	}

	if ((FieldPacketPtr[FieldSize - 1] == '\r') || (FieldPacketPtr[FieldSize - 1] == '\n'))
	{
		FormattedField[FieldSize - 1] = 0;
		*FormattedFieldLength= FieldSize - 1 + 1;
		return nbSUCCESS;
	}

	*FormattedFieldLength= FieldSize + 1;
	return nbSUCCESS;
}


int NativePrintHTTPContentField(const unsigned char *FieldPacketPtr, unsigned long FieldSize, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize)
{
unsigned int CurrentIndex;
unsigned int DataLength;

	CurrentIndex= 0;

	while ((FieldPacketPtr[CurrentIndex] != ':') && (CurrentIndex < FieldSize))
		CurrentIndex++;

	if (CurrentIndex == FieldSize)
	{
		*FormattedFieldLength= 0;
		FormattedField[0]= 0;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "The field does not appear to have the ':' character inside.");
		return nbFAILURE;
	}	
	else
		CurrentIndex++;

	while ((FieldPacketPtr[CurrentIndex] == ' ') && (CurrentIndex < FieldSize))
		CurrentIndex++;

	DataLength= FieldSize - CurrentIndex;

	// '+1' because we need to take into account also the '\0' at the end
	// Not very precise, since we may get rid of the '\r\n' at the end, but I think is enough here
	if ( (DataLength + 1) > FormattedFieldSize)
	{
		*FormattedFieldLength= 0;
		FormattedField[0]= 0;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Internal error: the buffer required to print field value is too small.");
		return nbFAILURE;
	}


	// In case of wrong data in the packet, we may have less than 2 bytes
	// (which is very strange, but it may happen)
	if (DataLength > 2)
		memcpy(FormattedField, &FieldPacketPtr[CurrentIndex], DataLength);

	if ((FieldPacketPtr[FieldSize - 2] == '\r') || (FieldPacketPtr[FieldSize - 2] == '\n'))
	{
		FormattedField[DataLength - 2] = 0;
		*FormattedFieldLength= DataLength - 2 + 1;
		return nbSUCCESS;
	}

	if ((FieldPacketPtr[FieldSize - 1] == '\r') || (FieldPacketPtr[FieldSize - 1] == '\n'))
	{
		FormattedField[DataLength - 1] = 0;
		*FormattedFieldLength= DataLength - 1 + 1;
		return nbSUCCESS;
	}

	*FormattedFieldLength= DataLength + 1;
	return nbSUCCESS;
}


// itoa() is not C standard and some platforms do not have this function
// We define our own converter
unsigned int uint2a(unsigned int Value, char* ResultBuffer)
{
unsigned int i;
unsigned int digits;	// In fact, it contains the number of digits minus one
char temp;
	
	for (i= 0; Value; i++)
	{
		ResultBuffer[i] = "0123456789"[Value % 10];
		Value= Value / 10;
	}

	digits= i;

//printf("\n\n Digits %d %d ", digits, digits/2);

	// Now, reverse the string
	for(i= 0 ; i < (digits / 2) ; i++)
	{
		temp= ResultBuffer[i];
		ResultBuffer[i]= ResultBuffer[digits - 1 - i];
		ResultBuffer[digits - 1 - i]= temp;
	}

	return digits;
}

