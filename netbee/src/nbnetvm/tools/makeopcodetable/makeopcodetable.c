/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



#define OPCODE_TABLE_LEN		256	//!< 1 BYTE per instruction => 256 entries

typedef struct OpCodeDescriptor
{
	char		name[128];	//!< opcode Name
	char		args[128];	//!< argument's type
	uint8_t	code;		//!< opcode
} OpCodeDescriptor;




OpCodeDescriptor nvmOpCodeTable[OPCODE_TABLE_LEN];




int main(int argc,char *argv[])
{

	FILE *ocfile;
	char linebuf [512] = "";
	char id[128] = "";
	char name[128] = "";
	char params[128] = "";
	uint32_t opcode = 0, i = 0;
	char *tbuf;


	if(argc != 2)
	{
		printf("Usage: %s DefFileName\n", argv[0]);
		return (EXIT_FAILURE);
	}

	ocfile = fopen(argv[1], "r");
	if(ocfile == NULL)
	{
		printf("Cannot find input file %s", argv[1]);
		return (EXIT_FAILURE);
	}


	for (i = 0; i < OPCODE_TABLE_LEN; i++)
	{
		strcpy(nvmOpCodeTable[i].name, "");
		strcpy(nvmOpCodeTable[i].args, "0");
		nvmOpCodeTable[i].code = 0;
		
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
		
		tbuf = strchr(tbuf + 1, ',');
		if(sscanf(tbuf,", %x", &opcode) != 1) 
			continue;


		nvmOpCodeTable[opcode].code = opcode;
		strcpy(nvmOpCodeTable[opcode].name, name);
		strcpy(nvmOpCodeTable[opcode].args, params);


	}


	for (i = 0; i < OPCODE_TABLE_LEN - 1; i++)
		printf("/*0x%02X*/\t{\"%s\", %s},\n", i, nvmOpCodeTable[i].name, nvmOpCodeTable[i].args);
	//last line without the trailing comma:
	printf("/*0x%02X*/\t{\"%s\", %s}\n", i, nvmOpCodeTable[i].name, nvmOpCodeTable[i].args);

	return (EXIT_SUCCESS);
}


