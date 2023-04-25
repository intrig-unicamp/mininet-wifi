/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char *argv[])
{

	FILE *ocfile, *sfilei;
	char linebuf [512];
	char id[128];
	char name[128];
	char params[128];
	char *tbuf;
	unsigned int i;

	if(argc != 3)
	{
		printf("Usage: %s defname scannername\n", argv[0]);
		return (EXIT_FAILURE);
	}

	ocfile = fopen(argv[1], "r");
	if(ocfile == NULL)
	{
		printf("Cannot find input file %s", argv[1]);
		return (EXIT_FAILURE);
	}

	sfilei = fopen(argv[2], "r");
	if(sfilei == NULL)
	{
		printf("Cannot find input file %s", argv[2]);
		return (EXIT_FAILURE);
	}

	// Scan the first portion of the scanner
	while(fgets(linebuf, 511, sfilei) != NULL)
	{
		if(strncmp(linebuf, "!!opcodes", sizeof("!!opcodes")-1) == 0)
			break;
		else
			printf("%s", linebuf);
	}

	// scan the .def file
	while(fgets(linebuf, 511, ocfile) != NULL)
	{
		// Extract opcode id
		if(sscanf(linebuf," nvmOPCODE ( %[^,]s", id) != 1)
			continue;

		// Extract the mnemonic
		tbuf = strchr(linebuf, ',');

		if(sscanf(tbuf,", \"%[^\"][^,]s", name) != 1)
//		if(sscanf(tbuf,", %[^,]s", name) != 1)
			continue;

		// Extract the arguments
		tbuf = strchr(tbuf + 1, ',');

		if(sscanf(tbuf,", %[^,]s", params) != 1)
			continue;

		//
		// Print the output
		//
		printf("\"%s\"", name);

		// Some formatting
#define TABSIZE 8
		for(i = (strlen(name) + 2) / TABSIZE; i < 3; i++)
			printf("\t");

		printf("{nvmparser_lval.number =  %s; return %s;}\n", id, params);
	}


	// Append the rest of the scanner
	while(fgets(linebuf, 511, sfilei) != NULL)
	{
		if(strncmp(linebuf, "!!opcodes", sizeof("!!opcodes")-1) == 0)
			break;
		else
			printf("%s", linebuf);
	}

	return (EXIT_SUCCESS);
}
