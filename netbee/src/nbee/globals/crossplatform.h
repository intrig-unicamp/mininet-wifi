/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

// FULVIO: is this file really needed?
#include <stdlib.h>

#define ultoa _ultoa

char *_ultoa (unsigned long value, char *string, int radix);


char *_ultoa (unsigned long value, char *string, int radix)
{
char *dst;
char digits[32];
int i, n;

	dst = string;
	if (radix < 2 || radix > 36)
	{
		*dst = 0;
		return string;
	}

	i = 0;
	do
	{
		n = value % radix;
		digits[i++] = (n < 10 ? (char)n+'0' : (char)n-10+'a');
		value /= radix;
	} while (value != 0);

	while (i > 0)
		*dst++ = digits[--i];

	*dst = 0;

	return string;
}

