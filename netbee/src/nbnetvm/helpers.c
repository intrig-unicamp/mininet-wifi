/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file helpers.c
 *	\brief This file contains helper functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <nbnetvm.h>


//#define errsnprintf nvmsnprintf

int errsnprintf(char* buffer, int bufsize, const char *format, ...)
{
int writtenbytes;
va_list args;
va_start(args, format);

	if (buffer == NULL)
		return -1;

#ifdef WIN32
	writtenbytes= _vsnprintf(buffer, bufsize - 1, format, args);
#else
	writtenbytes= vsnprintf(buffer, bufsize - 1, format, args);
#endif
	if (bufsize >= 1)
		buffer[bufsize - 1] = 0;
	else
		buffer[0]= 0;

	return writtenbytes;
};



void *nvmAllocObject(uint32_t size, char *errbuf)
{
	void *obj = NULL;
	obj = calloc(1, size);
	if (obj == NULL)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}
	return obj;
}

void *nvmAllocNObjects(uint32_t num, uint32_t size, char *errbuf)
{
	void *obj = NULL;
	obj = calloc(num, size);
	if (obj == NULL)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}
	return obj;
}


void nvmFreeObject(void *obj)
{
	free(obj);
}


/***************** TAKEN FROM TCPDUMP ****************/

#define ASCII_LINELENGTH 300
#define HEXDUMP_BYTES_PER_LINE 16
#define HEXDUMP_SHORTS_PER_LINE (HEXDUMP_BYTES_PER_LINE / 2)
#define HEXDUMP_HEXSTUFF_PER_SHORT 5 /* 4 hex digits and a space */
#define HEXDUMP_HEXSTUFF_PER_LINE (HEXDUMP_HEXSTUFF_PER_SHORT * HEXDUMP_SHORTS_PER_LINE)

void hex_and_ascii_print_with_offset(FILE *outfile, const char *ident, register const uint8_t *cp,
									register uint32_t length, register uint32_t oset) {
	register uint32_t i;
	register int s1, s2;
	register int nshorts;
	char hexstuff[HEXDUMP_SHORTS_PER_LINE*HEXDUMP_HEXSTUFF_PER_SHORT+1], *hsp;
	char asciistuff[ASCII_LINELENGTH+1], *asp;

	nshorts = length / sizeof(unsigned short);
	i = 0;
	hsp = hexstuff; asp = asciistuff;
	while (--nshorts >= 0)
	{
		s1 = *cp++;
		s2 = *cp++;
		(void)errsnprintf(hsp, sizeof(hexstuff) - (hsp - hexstuff),
		    " %02x%02x", s1, s2);
		hsp += HEXDUMP_HEXSTUFF_PER_SHORT;
		*(asp++) = (isgraph(s1) ? s1 : '.');
		*(asp++) = (isgraph(s2) ? s2 : '.');
		i++;
		if (i >= HEXDUMP_SHORTS_PER_LINE)
		{
			*hsp = *asp = '\0';
			(void)printf("%s0x%04x: %-*s  %s",
			    ident, oset, HEXDUMP_HEXSTUFF_PER_LINE,
			    hexstuff, asciistuff);
			i = 0; hsp = hexstuff; asp = asciistuff;
			oset += HEXDUMP_BYTES_PER_LINE;
		}
	}
	if (length & 1)
	{
		s1 = *cp++;
		(void)errsnprintf(hsp, sizeof(hexstuff) - (hsp - hexstuff),
		    " %02x", s1);
		hsp += 3;
		*(asp++) = (isgraph(s1) ? s1 : '.');
		++i;
	}
	if (i > 0)
	{
		*hsp = *asp = '\0';
		(void)fprintf(outfile, "%s0x%04x: %-*s  %s",
		     ident, oset, HEXDUMP_HEXSTUFF_PER_LINE,
		     hexstuff, asciistuff);
	}
}

