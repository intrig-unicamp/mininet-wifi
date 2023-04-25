/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file dump.c
 * \brief This file contains NetIL bytecode dump functions
 *
 */

#include <nbnetvm.h>
#include <netvm_bytecode.h>
//#include <netvm_structs.h>
#include <assembler/netvm_dump.h>
#include "nvm_gramm.tab.h"
//#include "../netvm_internals.h"
#include "../opcode_consts.h"
#include <assembler/compiler.h>
#include <helpers.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#undef snprintf
#define snprintf _snprintf
#endif


OpCodeInfo OpCodeTable[256]=
{
	#include "../opcodestable.txt"
};



#define EXTRACT_BC_SIGNATURE(dst,src)	do{strncpy( (dst) , (src) , 4 ); (dst)[4]='\0';}while(0)


char *nvmDumpInsn(char *instrBuf, uint32_t bufLen, uint8_t **bytecode, uint32_t *lineNum)
{

	char	*op;
	char	args[2046]="";
	char    pair_match[64] = "";
	uint8_t opcode;

	uint8_t *instr = *bytecode;

	uint32_t i = 0, n_pairs = 0, key = 0;
	int32_t target = 0, default_target = 0;

	opcode = *instr;
	(*bytecode)++;


	op = OpCodeTable[opcode].Name;

	memset(args, 0, sizeof args);

	switch(OpCodeTable[opcode].Args)
	{
		case T_NO_ARG_INST:
			strcpy(args, "");
			break;
		case T_1BYTE_ARG_INST:
			snprintf(args, sizeof args, "%u", **bytecode);
			(*bytecode)++;
			break;
		case T_1WORD_ARG_INST:
			snprintf(args, sizeof args, "%u", *(uint16_t*)*bytecode);
			(*bytecode) += 2;
			break;
		case T_1INT_ARG_INST:
			snprintf(args, sizeof args, "%u", *(uint32_t*)*bytecode);
			(*bytecode) += 4;
			break;
		case T_2INT_ARG_INST:
			snprintf(args, sizeof args, "%u, %u", *(uint32_t*)*bytecode, *(uint32_t*)(*bytecode + 4));
			(*bytecode) += 8;
			break;
		case T_1LABEL_INST:
			snprintf(args, sizeof args, "%u", (*((int32_t*)(*bytecode))) + (*lineNum) + 5);
			(*bytecode) += 4;
			break;
		case T_1_SHORT_LABEL_INST:
			snprintf(args, sizeof args, "%u", (*((int16_t*)(*bytecode))) + (*lineNum) + 2);
			(*bytecode) += 1;
			break;

		case T_JMP_TBL_LKUP_INST:
			default_target = *(int32_t*)*bytecode;
			(*bytecode) += 4;
			n_pairs = *(uint32_t*)*bytecode;
			(*bytecode) += 4;
			snprintf(args, sizeof args, "%u:\n", n_pairs); /*number of match pairs */
			default_target += *lineNum + 1 + 4 + 4 + 8 * n_pairs; /*op + npairs + def_targ + npairs * (match + target)*/
			for (i = 0; i < n_pairs; i++)
			{
				key = *(uint32_t*)*bytecode;
				(*bytecode) += 4;
				target = *(uint32_t*)*bytecode;
				(*bytecode) += 4;
				snprintf(pair_match, sizeof pair_match, "\t    %u: %u\n", key, target + *lineNum + 1 + 4 + 4 + 8 * n_pairs);
				strncat(args, pair_match, sizeof args - strlen(args));
			}
			snprintf(pair_match, sizeof pair_match, "\t    default: %u\n", default_target);
			strncat(args, pair_match, sizeof args - strlen(args));
			break;

		case T_1_PUSH_PORT_ARG_INST:
		case T_1_PULL_PORT_ARG_INST:
			snprintf(args, sizeof args, "%u", *(int32_t*)*bytecode);
			(*bytecode) += 4;
			break;

		case T_2_COPRO_IO_ARG_INST:
			snprintf(args, sizeof args, "%u, %u", *(int32_t*)*bytecode, *(int32_t*)(*bytecode+4));
			(*bytecode) += 8;
	}

	snprintf(instrBuf, bufLen, "%u:\t%-15s%20s", (*lineNum), op, args);
	(*lineNum) += (*bytecode) - instr;
	return instrBuf;

}


void nvmDumpBytecode(FILE *outFile, uint8_t *bytecode, uint32_t bytecodeLen)
{
	uint8_t *sentinel;
	uint32_t line = 0;

	char instruction[256];

	sentinel = bytecode + bytecodeLen;
	while (bytecode < sentinel)
	{
		fprintf(outFile, "%s\n", nvmDumpInsn(instruction, sizeof instruction, &bytecode, &line));
	}
}



#define hex_and_ascii_print(file, ident, cp, length) hex_and_ascii_print_with_offset(file, ident, cp, length, 0)

/**************************************************************/


void nvmDumpInitialisedDataSection (FILE *outFile, uint8_t *bytecode, uint32_t bytecodeLen) {
	uint32_t endianness;

	endianness = (uint32_t) *bytecode;
	switch (endianness) {
		case SEGMENT_BIG_ENDIAN:
			fprintf (outFile, ";\tEndianness: Big endian\n");
			break;
		case SEGMENT_LITTLE_ENDIAN:
			fprintf (outFile, ";\tEndianness: Little endian\n");
			break;
		case SEGMENT_UNDEFINED_ENDIANNESS:
		default:
			fprintf (outFile, ";\tEndianness: Unknown\n");
			break;
	}
	bytecode += sizeof (uint32_t);

	hex_and_ascii_print (outFile, "\n; ", bytecode, bytecodeLen);

	return;
}


void nvmDumpIpToLineMappings (FILE *outfile, uint8_t *bytecode, uint32_t bytecodeLen)
{
	uint32_t i = 0;
	uint32_t ip, line;
	fprintf(outfile, ";\tBytecode to source line mappings:\n");
	while(i < bytecodeLen)
	{
		ip = *(uint32_t*)&bytecode[i];
		i+=4;
		line = *(uint32_t*)&bytecode[i];
		i+=4;
		fprintf(outfile, ";\t  Bytecode %u -> Source Line %u\n", ip, line);
	}
}


// TODO Add stuff to dump the metadata section
void nvmDumpBytecodeInfo(FILE *file,nvmByteCode *binary)
{

	nvmByteCodeImageHeader *Hdr;
	nvmByteCodeSectionHeader *pSectionsTable;
	uint32_t NumberOfSections,section;
	char signature[5];


	Hdr = binary->Hdr;
	NumberOfSections = Hdr->FileHeader.NumberOfSections;


	EXTRACT_BC_SIGNATURE(signature,(char*) &Hdr->Signature); //extract signature info "LODE"


	fprintf(file,";----------------- BINARY INFO: -----------------\n\n");

	fprintf(file,";IMAGE HEADER:\n");
	fprintf(file,";Signature: %s\n\n",signature);
	fprintf(file,";\tFILE HEADER:\n");
	fprintf(file,";\tNumber Of Sections: %d\n",NumberOfSections);
	fprintf(file,";\tTime and Date Stamp: %d\n",Hdr->FileHeader.TimeDateStamp);
	fprintf(file,";\tSize of Optional Header: %d\n",Hdr->FileHeader.SizeOfOptionalHeader );
	fprintf(file,";\tAddress Of Init Entry Point: 0x%X\n",Hdr->FileHeader.AddressOfInitEntryPoint);
	fprintf(file,";\tAddress Of Pull Entry Point: 0x%X\n",Hdr->FileHeader.AddressOfPullEntryPoint );
	fprintf(file,";\tAddress Of Push Entry Point: 0x%X\n\n",Hdr->FileHeader.AddressOfPushEntryPoint );

	if (Hdr->FileHeader.SizeOfOptionalHeader != 0){
		fprintf(file,";\tOPTIONAL HEADER:\n");
		fprintf(file,";\tMajor Linker Version: %d\n",Hdr->OptionalHeader.MajorLinkerVersion);
		fprintf(file,";\tMinor Linker Version: %d\n",Hdr->OptionalHeader.MinorLinkerVersion);
		fprintf(file,";\tMajor Virtual Machine Version: %d\n",Hdr->OptionalHeader.MajorVirtualMachineVersion);
		fprintf(file,";\tMinor Virtual Machine Version: %d\n",Hdr->OptionalHeader.MinorVirtualMachineVersion);
		fprintf(file,";\tMajor Image Version: %d\n",Hdr->OptionalHeader.MajorImageVersion);
		fprintf(file,";\tMinor Image Version: %d\n",Hdr->OptionalHeader.MinorImageVersion);
		fprintf(file,";\tChecksum: 0x%X\n",Hdr->OptionalHeader.CheckSum);
		fprintf(file,";\tDefault Data Mem Size: %d\n",Hdr->OptionalHeader.DefaultDataMemSize);
		fprintf(file,";\tDefault Exchange Buf Size: %d\n",Hdr->OptionalHeader.DefaultExchangeBufSize);
		fprintf(file,";\tDefault Stack Size: %d\n",Hdr->OptionalHeader.DefaultStackSize);
		fprintf(file,";\tDefault Port Table Size: %d\n\n",Hdr->OptionalHeader.DefaultPortTableSize);
	}

	if (NumberOfSections!=0){

		fprintf(file,";SECTIONS:\n\n");

		pSectionsTable=binary->SectionsTable;
		for (section=0;section<NumberOfSections;section++){

			fprintf(file,";Section %d:\n",section);
			fprintf(file,";\tName: %s\n",pSectionsTable[section].Name);
			fprintf(file,";\tPointerToRawData: 0x%X\n",pSectionsTable[section].PointerToRawData);
			fprintf(file,";\tSizeOfRawData: %d\n",pSectionsTable[section].SizeOfRawData);
			fprintf(file,";\tSectionFlag: 0x%X\n",pSectionsTable[section].SectionFlag);
			if (pSectionsTable[section].SectionFlag & BC_CODE_SCN)
			{
				fprintf(file,";\tMax Stack Size: %u\n", *((uint32_t*)((uint8_t*)Hdr + pSectionsTable[section].PointerToRawData)));
				fprintf(file,";\tLocals: %u\n", *((uint32_t*)((uint8_t*)Hdr + pSectionsTable[section].PointerToRawData + 4)));
				nvmDumpBytecode(file, (uint8_t*)Hdr + pSectionsTable[section].PointerToRawData + 8, pSectionsTable[section].SizeOfRawData - 8);
			}
			else if (pSectionsTable[section].SectionFlag == BC_INITIALIZED_DATA_SCN)
			{
				nvmDumpInitialisedDataSection (file, (uint8_t *) Hdr + pSectionsTable[section].PointerToRawData, pSectionsTable[section].SizeOfRawData);
			}
			else if (pSectionsTable[section].SectionFlag & BC_INSN_LINES_SCN)
			{
				nvmDumpIpToLineMappings(file, (uint8_t *) Hdr + pSectionsTable[section].PointerToRawData, pSectionsTable[section].SizeOfRawData);
			}
		}

		fprintf(file,"\n\n");

	}

	else {
		fprintf(file,"\n;*** WARNING! The bytecode contains no valid sections!\n");
	}

	fprintf(file,";-------------- END OF BINARY INFO --------------\n\n");
}

