/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


/*!
	\file native_showfunctions.h

	This file containsthe prototypes of the functions that implement native visualization.
*/


// Native function lists
// Please remember that those functions can either return nbSUCCESS or nbFAILURE; the 'warning' code is not allowed.
int NativePrintIPv4Address(const unsigned char *FieldPacketPtr, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize);
int NativePrintAsciiField(const unsigned char *FieldPacketPtr, unsigned long FieldSize, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize);
int NativePrintAsciiLineField(const unsigned char *FieldPacketPtr, unsigned long FieldSize, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize);
int NativePrintHTTPContentField(const unsigned char *FieldPacketPtr, unsigned long FieldSize, char *FormattedField, unsigned long FormattedFieldSize, unsigned long *FormattedFieldLength, char *ErrBuf, int ErrBufSize);

// Helper functions
unsigned int uint2a(unsigned int Value, char* ResultBuffer);
