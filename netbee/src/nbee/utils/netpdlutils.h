/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/*!
	\file netpdlutils.h

	This file defines some functions used to handle common situations in NetPDL.
*/


#pragma once		/* Do not include this file more than once */


#ifdef __cplusplus
extern "C" {
#endif


char ConvertHexCharToDec(char HexChar);

long NetPDLAsciiStringToLong(char *String, int Len, int radix);
unsigned long NetPDLHexDumpToLong(const unsigned char *HexDumpPtr, int ResultSize, int HexDumpIsInNetworkByteOrder);

int ConvertHexDumpAsciiToHexDumpBin(char *HexDumpAscii, unsigned char *HexDumpBin, int HexDumpBinSize);
int ConvertHexDumpBinToHexDumpAscii(char *HexDumpBin, int HexDumpBinSize, int HexDumpIsInNetworkByteOrder, char *HexDumpAscii, int HexDumpAsciiSize);

void NetPDLLongToHexDump(int Value, int ResultSize, unsigned char *HexDumpPtr);

#ifdef __cplusplus
}
#endif

