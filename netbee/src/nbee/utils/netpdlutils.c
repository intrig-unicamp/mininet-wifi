/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../globals/debug.h"
#include "../globals/globals.h"


/*!
	\brief It translates a single hex char (in the range '0' - 'F' or '0' - 'f')
	into its decimal value.

	\param HexChar: an ascii character that contains the Hex digit to be converted

	\return The decimal value (e.g. '10' if HexChar is equal to 'A' or 'a') of the given
	character.
*/
char ConvertHexCharToDec(char HexChar)
{
	if ((HexChar >= '0') && (HexChar <= '9'))
		return (HexChar - '0');

	if ((HexChar >= 'A') && (HexChar <= 'F'))
		return (HexChar - 'A' + 10);

	if ((HexChar >= 'a') && (HexChar <= 'f'))
		return (HexChar - 'a' + 10);

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, NULL, 0, "An Hex number is out of range");
	return 0;
}


//! Convert the specified ASCII string into an integer; 'radix' contains the base (e.g. 16 for an hex number) of the string
long NetPDLAsciiStringToLong(char *String, int Len, int radix)
{
int i;
char *HexChar;
long power= 1;
long res= 0;

	// initialize the pointer to the last char of the string
	HexChar= String + (Len - 1);

	for (i= (Len-1); i >= 0; i--)
	{
	long value= 0;
		
		if (power == 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, NULL, 0, "Error: a field value spans over 32+ bits");
			return nbFAILURE;
		}

		value= ConvertHexCharToDec(*HexChar);

		value = value * power;
		res+= value;
		power*= radix;

		// Move to the previous char into the string
		HexChar--;
	}
	
	return res;
}


/*!
	\brief It translates a value (contained in the binary hex dump of a packet) into a number.

	This function is used to convert the value of a field (which is contained in the 
	binary hex dump of the packet) into a number.
	This function takes into account both the host byte order and the "network" byte order
	of the given protocol (e.g. IP fields are stored in network byte order, which is 
	big-endian, while Bluetooth are stored in the opposite way).

	\param HexDumpPtr: pointer to the hex dump (in the packet dump) that contains the
	field that has to be translated

	\param ResultSize: size of the field, which must be <= 4.

	\param HexDumpIsInNetworkByteOrder: 'true' if the field is stored in network byte order,
	'false' otherwise.

	\return The value of the field.

	\warning The field size MUST be less or equal to four.
*/
unsigned long NetPDLHexDumpToLong(unsigned char *HexDumpPtr, int ResultSize, int HexDumpIsInNetworkByteOrder)
{

	if (ResultSize > 4)
	{
	char TmpBuf[1024];

		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, TmpBuf, sizeof(TmpBuf), "Internal error: an integer cannot be larger than 4 bytes.");
		return 0;
	}

#ifdef HOST_IS_LITTLE_ENDIAN
	if (HexDumpIsInNetworkByteOrder)
	{
		int i;

		unsigned long Result;
		unsigned char *ResultUChar;

		Result= 0;
		ResultUChar= (unsigned char *) &Result;

		for (i=0; i < ResultSize; i++)
		{
			ResultUChar[i]= HexDumpPtr[ResultSize - i - 1];
		}

		return Result;
	}
	else
	{
		switch(ResultSize)
		{
			case 1:
			{
				uint8_t *Result= (uint8_t *)HexDumpPtr;
				return (unsigned long) (*Result);
				break;
			}
			case 2:
			{
				uint16_t *Result= (uint16_t *)HexDumpPtr;
				return (unsigned long) (*Result);
				break;
			}
			case 4:
			{
				uint32_t *Result= (uint32_t *)HexDumpPtr;
				return (unsigned long) (*Result);
				break;
			}
			default:
			{
			char TmpBuf[1024];

				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, TmpBuf, sizeof(TmpBuf), "Internal error: requested a conversion for a number is is neither 2 or 4 bytes.");
				return 0;
			}
		}
	}

#endif

#ifdef HOST_IS_BIG_ENDIAN
	if (HexDumpIsInNetworkByteOrder)
	{
		switch(ResultSize)
		{
			case 1:
			{
				uint8_t *Result= (uint8_t *)HexDumpPtr;
				return (unsigned long) (*Result);
				break;
			}
			case 2:
			{
				uint16_t *Result= (uint16_t *)HexDumpPtr;
				return (unsigned long) (*Result);
				break;
			}
			case 4:
			{
				uint32_t *Result= (uint32_t *)HexDumpPtr;
				return (unsigned long) (*Result);
				break;
			}
			default:
			{
			char TmpBuf[1024];

				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, TmpBuf, sizeof(TmpBuf), "Internal error: requested a conversion for a number is is neither 2 or 4 bytes.");
				return 0;
			}
		}
	}
	else
	{
		int i;

		unsigned long Result;
		unsigned long *ResultUChar;

		Result= 0;
		ResultUChar= (unsigned char *) &Result;

		for (i=0; i < ResultSize; i++)
			ResultUChar[i]=HexDumpPtr[ResultSize - i - 1];

		return Result;
	}
#endif

#if (!defined(HOST_IS_LITTLE_ENDIAN) && !defined(HOST_IS_BIG_ENDIAN))
	#error Host byte order (little/big endian) has not been defined.

	return 0;
#endif

}



/*!
	\brief Converts an hex dump in ascii chars into a binary hex dump.

	This method converts an ascii dump (e.g. "AD810A") into the corresponding binary
	dump (the same, but "AD" will be a single 8bit char).

	\param HexDumpAscii: a string that contains the hex dump in ascii format.
	
	\param HexDumpBin: a buffer that will contain the binary dump (according 
	to the pcap format) when the function ends.
	This buffer must be allocated by the user.

	\param HexDumpBinSize: size of the previous buffer.

	\return The number of bytes currently written into the HexDumpBin buffer, or
	nbFAILURE in case the target buffer is not large enough.
*/
int ConvertHexDumpAsciiToHexDumpBin(char *HexDumpAscii, unsigned char *HexDumpBin, int HexDumpBinSize)
{
int DumpIndex= 0;

	while ( (HexDumpAscii[DumpIndex*2] != 0) && (DumpIndex < HexDumpBinSize) )
	{
	char Low, High;

		High= ConvertHexCharToDec(HexDumpAscii[DumpIndex*2]) << 4;
		Low= ConvertHexCharToDec(HexDumpAscii[DumpIndex*2 + 1]);

		HexDumpBin[DumpIndex]= High + Low;

		DumpIndex++;
	}

	// Check if the return buffer is big enough
	if (HexDumpAscii[DumpIndex*2] != 0)
		return nbFAILURE;
	else
		return DumpIndex;
}


/*!
	\brief Converts a binary hex dump into a hex dump made up of ascii chars.

	This method converts a binary dump (e.g. 'AD810A') into the corresponding ascii
	dump (the same, but the previous string will use 6 bytes instead of three).

	\param HexDumpBin: a buffer that contains the hex dump in binary format.
	
	\param HexDumpBinSize: the number of valid bytes in the HexDumpBin buffer. This parameter
	is needed because we can no longer rely on the string terminator (the NULL character)
	which is used to terminate ascii strings.

	\param HexDumpIsInNetworkByteOrder: 'true' if the hex dump is in network byte order (the
	default case), 'false' otherwise. For instance, this parameter is used to convert the hex dump
	related to fields that are not in network byte order, such as some fields in Bluetooth protocol.

	\param HexDumpAscii: a string that will contain the ascii dump when the function ends,
	This string must be allocated by the user.

	\param HexDumpAsciiSize: size of the previous string. Be carefully that the HexDumpAscii string
	will be 'zero' terminated; therefore the maximum number of valid characters that are allowed
	into the ascii string are (HexDumpAsciiSize - 1).

	\return The number of bytes currently written into the HexDumpAscii buffer, or
	nbFAILURE in case the target buffer is not large enough.
*/
int ConvertHexDumpBinToHexDumpAscii(char *HexDumpBin, int HexDumpBinSize, int HexDumpIsInNetworkByteOrder, char *HexDumpAscii, int HexDumpAsciiSize)
{
int DumpIndex;

	// Check if the return buffer is big enough
	if (HexDumpBinSize >= (HexDumpAsciiSize/2 - 1))
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "The ascii buffer is not large enough to keep the data.");
		return nbFAILURE;
	}

	for (DumpIndex= 0; DumpIndex < HexDumpBinSize; DumpIndex++)
	{
	char Low, High;

		if (HexDumpIsInNetworkByteOrder)
		{
			High= (HexDumpBin[DumpIndex] >> 4) & '\x0F'; // There may be problems due to the sign, so I have to mask the first nibble
			Low= HexDumpBin[DumpIndex] & '\x0F';
		}
		else
		{
			High= (HexDumpBin[HexDumpBinSize - DumpIndex - 1] >> 4) & '\x0F'; // There may be problems due to the sign, so I have to mask the first nibble
			Low= HexDumpBin[HexDumpBinSize - DumpIndex - 1] & '\x0F';
		}


		if (High > 9)
			HexDumpAscii[DumpIndex*2]= 'A' + (High - 10);
		else
			HexDumpAscii[DumpIndex*2]= '0' + High;

		if (Low > 9)
			HexDumpAscii[DumpIndex*2+1]= 'A' + (Low - 10);
		else
			HexDumpAscii[DumpIndex*2+1]= '0' + Low;
	}

	HexDumpAscii[DumpIndex*2]= 0;

	return (DumpIndex * 2);
}


/*!
	\brief It translates a numeric value into the corresponding n hex buffer.

	This function is used to convert a number into a buffer, formatted in network byte order.

	\param Value: the numeric value that has to be converted
	\param HexDumpPtr: pointer to the hex dump (in the packet dump) that contains the
	field that has to be translated

	\param ResultSize: size of the buffer, which must be <= sizeof(int).

	\param HexDumpPtr: pointer to the hex buffer that will contain the result.
	This buffer must be allocated by the user.

	\warning The field size MUST be less or equal to sizeof(int).
*/
void NetPDLLongToHexDump(int Value, int ResultSize, unsigned char *HexDumpPtr)
{
int i;
unsigned char *ValueUChar;

	if (ResultSize > sizeof(int))
	{
	char TmpBuf[1024];

		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, TmpBuf, sizeof(TmpBuf), "Internal error: an integer cannot be larger than 4 bytes.");
		return;
	}

#ifdef HOST_IS_LITTLE_ENDIAN

	ValueUChar= (unsigned char *) &Value;

	for (i=0; i < ResultSize; i++)
		HexDumpPtr[i]= ValueUChar[ResultSize - i - 1];

#endif

#ifdef HOST_IS_BIG_ENDIAN

	ValueUChar= (unsigned char *) &Value;

	for (i=0; i < ResultSize; i++)
		HexDumpPtr[i]= ValueUChar[sizeof(int) - ResultSize + i];

#endif

#if (!defined(HOST_IS_LITTLE_ENDIAN) && !defined(HOST_IS_BIG_ENDIAN))
	#error Host byte order (little/big endian) has not been defined.
#endif

}
