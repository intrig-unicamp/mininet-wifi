/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file netil_assembler.c
 *	\brief This file contains the functions used to create the NetIL assembler bytecode
 */

#include <nbnetvm.h>
#include "../../nbee/globals/debug.h"
#include "../helpers.h"
#include "compiler.h"
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include <netvm_bytecode.h>



/* --------------- Internal functions ---------------*/


int32_t AddSection(nvmByteCode * SaveBin, char* SectName, uint32_t SectFlags, char* SectBody, uint32_t SectSize)
{
	nvmByteCodeSectionHeader *TmpSectionsTable;
	char *TmpSections;
	int i;

	// Update the Section Table
	// Reallocate the memory for the Sections Table calculating the size with the new Section
	TmpSectionsTable = (nvmByteCodeSectionHeader *) realloc(SaveBin->SectionsTable, (SaveBin->Hdr->FileHeader.NumberOfSections + 1) * sizeof(nvmByteCodeSectionHeader));

	if (TmpSectionsTable == NULL)
		return nvmFAILURE;

	SaveBin->SectionsTable = TmpSectionsTable;

	// Update the section headers already present
	for(i = 0; i < SaveBin->Hdr->FileHeader.NumberOfSections; i++)
		SaveBin->SectionsTable[i].PointerToRawData += sizeof(nvmByteCodeSectionHeader);

	// Create the new section header
	memset(SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].Name, 0, 8);
	memcpy(SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].Name, SectName, (strlen(SectName)<8)?strlen(SectName):8);
	SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].PointerToRawData = SaveBin->SizeOfSections +
		sizeof(nvmByteCodeImageHeader) +
		(SaveBin->Hdr->FileHeader.NumberOfSections + 1) * sizeof(nvmByteCodeSectionHeader);
	SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].SizeOfRawData = SectSize;
	SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].PointerToRelocations = 0;
	SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].NumberOfRelocations = 0;
	SaveBin->SectionsTable[SaveBin->Hdr->FileHeader.NumberOfSections].SectionFlag = SectFlags;

	// Update the Sections Body
	TmpSections= (char *) realloc(SaveBin->Sections, SaveBin->SizeOfSections + SectSize);
	if (TmpSections == NULL)
		return nvmFAILURE;

	SaveBin->Sections = TmpSections;

	memcpy(SaveBin->Sections + SaveBin->SizeOfSections, SectBody, SectSize);

	// Update the state
	SaveBin->Hdr->FileHeader.NumberOfSections++;
	SaveBin->SizeOfSections += SectSize;

	return nvmSUCCESS;
}



int32_t SetInitEntryPoint(nvmByteCode *SaveBin, char *SectName, uint32_t addr)
{
int i;

	for (i=0; i<SaveBin->Hdr->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(SectName,	(char *) SaveBin->SectionsTable[i].Name) == 0)
		{
			SaveBin->Hdr->FileHeader.AddressOfInitEntryPoint= SaveBin->SectionsTable[i].PointerToRawData + addr;
			return nvmSUCCESS;
		}
	}

	return nvmFAILURE;
}



int32_t SetPushEntryPoint(nvmByteCode *SaveBin, char *SectName, uint32_t addr)
{
int i;

	for (i=0; i<SaveBin->Hdr->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(SectName, (char *) SaveBin->SectionsTable[i].Name) == 0)
		{
			SaveBin->Hdr->FileHeader.AddressOfPushEntryPoint= SaveBin->SectionsTable[i].PointerToRawData + addr;
			return nvmSUCCESS;
		}
	}

	return nvmFAILURE;
}


int32_t SetPullEntryPoint(nvmByteCode *SaveBin, char *SectName, uint32_t addr)
{
	int i;

	for(i=0; i<SaveBin->Hdr->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(SectName, (char *) SaveBin->SectionsTable[i].Name) == 0)
		{
			SaveBin->Hdr->FileHeader.AddressOfPullEntryPoint= SaveBin->SectionsTable[i].PointerToRawData + addr;
			return nvmSUCCESS;
		}
	}

	return nvmFAILURE;
}

nvmByteCode *nvmCreateBytecode(uint32_t DefaultDataMemSize, uint32_t DefaultExchangeBufferSize, uint32_t DefaultStackSize)
{
nvmByteCode *SaveBin;
time_t value;

	SaveBin= malloc(sizeof(nvmByteCode));
	if (SaveBin == NULL)
	{
		return NULL;
	}

	SaveBin->SectionsTable = NULL;
	SaveBin->Sections = NULL;
	SaveBin->SizeOfSections = 0;

	SaveBin->Hdr= malloc(sizeof(nvmByteCodeImageHeader));
	if (SaveBin->Hdr == NULL)
	{
		free(SaveBin);
		return NULL;
	}

	// Init header
	SaveBin->Hdr->Signature = 0x45444F4C;	//i.e. 'EDOL'
	SaveBin->Hdr->FileHeader.NumberOfSections = 0;
	time(&value);
	SaveBin->Hdr->FileHeader.TimeDateStamp = (uint32_t) value;
	SaveBin->Hdr->FileHeader.SizeOfOptionalHeader = sizeof (nvmByteCodeImageStructOpt);
	SaveBin->Hdr->FileHeader.AddressOfInitEntryPoint = 0;
	SaveBin->Hdr->FileHeader.AddressOfPushEntryPoint = 0;
	SaveBin->Hdr->FileHeader.AddressOfPullEntryPoint = 0;

	// Init optional header
	SaveBin->Hdr->OptionalHeader.MajorLinkerVersion = 0;
	SaveBin->Hdr->OptionalHeader.MinorLinkerVersion = 1;
	SaveBin->Hdr->OptionalHeader.MajorVirtualMachineVersion = 0;
	SaveBin->Hdr->OptionalHeader.MinorVirtualMachineVersion = 1;
	SaveBin->Hdr->OptionalHeader.MajorImageVersion = 1;
	SaveBin->Hdr->OptionalHeader.MinorImageVersion = 0;
	SaveBin->Hdr->OptionalHeader.CheckSum = 0;
	SaveBin->Hdr->OptionalHeader.Compressed = 0;
	SaveBin->Hdr->OptionalHeader.DefaultDataMemSize = DefaultDataMemSize;
	SaveBin->Hdr->OptionalHeader.DefaultExchangeBufSize = DefaultExchangeBufferSize;
	SaveBin->Hdr->OptionalHeader.DefaultStackSize = DefaultStackSize;
	SaveBin->Hdr->OptionalHeader.DefaultPortTableSize = 0; // XXX set to 0 because not supported yet

	return SaveBin;
}

void nvmDestroyBytecode(nvmByteCode *bytecode)
{
	if (bytecode == NULL)
		return;

	if (bytecode->Hdr != NULL)
		free(bytecode->Hdr);
}


/* --------------- Exported functions ---------------*/

//int32_t nvmCreateBytecodeFromAsmFile(nvmByteCode *bytecode, char *FileName)
//{
//	FILE *fp;
//	long readBytes;
//	char *NetVMAsmBuffer;
//	long bufferSize = 4096;  // Initial size of the buffer that contains the assembler that will be read from file and processed
//
//	if (FileName == NULL)
//	{
//
//		return nvmFAILURE;
//	}
//
//
//	// We open the file in text mode, i.e. NO "rb"  so that fread will take care of converting the
//	//  various carriage returns into the proper format for windows
//	fp= fopen(FileName, "r");
//	if (fp == NULL)
//	{
//		printf("non apre il file con fopen\n");
//		return nvmFAILURE;
//	}
//
//	NetVMAsmBuffer = calloc(bufferSize + 1, sizeof(char));
//
//	if (NetVMAsmBuffer == NULL)
//	{
//		printf("non fa la calloc\n");
//		return nvmFAILURE;
//	}
//
//	readBytes = 0;
//
//	/* we process the file until we reach its end
//	   we do not use ftell to obtain the file size
//	   since it doesn't take care of the carriage
//	   return conversions. ftell() can return a smaller
//	   size for the buffer that we need to use to read
//	   the file if the carriage return conversion
//	   "expands" the file (e.g. "\n" --> "\r\n")
//    */
//	while (!feof(fp))
//	{	/* we compute how many bytes we can read within
//		    the current buffer
//		*/
//		size_t bytesToRead = bufferSize - readBytes;
//		if (bytesToRead == 0)
//		{  /* it's time to double the buffer */
//			char *newNetVMAsmBuffer;
//			newNetVMAsmBuffer = calloc(bufferSize * 2 + 1, sizeof(char));;
//
//			if (newNetVMAsmBuffer == NULL)
//			{
//				free(NetVMAsmBuffer);
//				fclose(fp);
//				printf("buffer null\n");
//				return nvmFAILURE;
//			}
//
//			/* copy the old buffer in the new one */
//			memcpy(newNetVMAsmBuffer, NetVMAsmBuffer, bufferSize);
//			/* delete the old one */
//			free(NetVMAsmBuffer);
//			/* move the new buffer at the address pof the old one */
//			NetVMAsmBuffer = newNetVMAsmBuffer;
//			/* double the size of the current buffer */
//			bufferSize *= 2;
//			/* the actual read in the buffer will be done in the
//			   next iteration of the loop
//			*/
//		}
//		else
//		{
//			/* we have enough space, we can copy at most bytesToRead
//			   bytes into the buffer
//			*/
//			readBytes+= fread(NetVMAsmBuffer + readBytes, 1, bytesToRead, fp);
//		}
//	}

//	fclose(fp);
//
//	// We need to add a trailing \0, since fread does not terminate the string
//	// We do not have to check for enough space in the buffer, because allocation took in mind this last char
//	NetVMAsmBuffer[readBytes] = '\0';
//
//	if (nvmCreateBytecodeFromAsmBuffer(bytecode, NetVMAsmBuffer) == nvmFAILURE)
//	{
//		printf("non funziona create bytecode dal buf\n");
//		free(NetVMAsmBuffer);
//		return nvmFAILURE;
//	}
//
//	free(NetVMAsmBuffer);
//	return nvmSUCCESS;
//}



nvmByteCode *nvmAssembleNetILFromFile(char *FileName, char *ErrBuf)
{
	FILE *fp;
	uint32_t readBytes;
	char *NetVMAsmBuffer;
	nvmByteCode *bytecode;
	uint32_t bufferSize = 4096;  // Initial size of the buffer that contains the assembler that will be read from file and processed

	NETVM_ASSERT(FileName != NULL, "FileName cannot be NULL");

	// We open the file in text mode, i.e. NO "rb"  so that fread will take care of converting the
	//  various carriage returns into the proper format for windows
	fp= fopen(FileName, "r");
	if (fp == NULL)
	{
          errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Cannot open the file %s for reading", FileName);
          return NULL;
	}

	NetVMAsmBuffer = calloc(bufferSize + 1, sizeof(char));

	if (NetVMAsmBuffer == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}

	readBytes = 0;

	/* we process the file until we reach its end
	   we do not use ftell to obtain the file size
	   since it doesn't take care of the carriage
	   return conversions. ftell() can return a smaller
	   size for the buffer that we need to use to read
	   the file if the carriage return conversion
	   "expands" the file (e.g. "\n" --> "\r\n")
    */
	while (!feof(fp))
	{	/* we compute how many bytes we can read within
		    the current buffer
		*/
		size_t bytesToRead = bufferSize - readBytes;
		if (bytesToRead == 0)
		{  /* it's time to double the buffer */
			char *newNetVMAsmBuffer;
			newNetVMAsmBuffer = calloc(bufferSize * 2 + 1, sizeof(char));;

			if (newNetVMAsmBuffer == NULL)
			{
				errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
				free(NetVMAsmBuffer);
				fclose(fp);
				return NULL;
			}

			/* copy the old buffer in the new one */
			memcpy(newNetVMAsmBuffer, NetVMAsmBuffer, bufferSize);
			/* delete the old one */
			free(NetVMAsmBuffer);
			/* move the new buffer at the address pof the old one */
			NetVMAsmBuffer = newNetVMAsmBuffer;
			/* double the size of the current buffer */
			bufferSize *= 2;
			/* the actual read in the buffer will be done in the
			   next iteration of the loop
			*/
		}
		else
		{
			/* we have enough space, we can copy at most bytesToRead
			   bytes into the buffer
			*/
			readBytes+= fread(NetVMAsmBuffer + readBytes, 1, bytesToRead, fp);
		}
	}

	fclose(fp);

	// We need to add a trailing \0, since fread does not terminate the string
	// We do not have to check for enough space in the buffer, because allocation took in mind this last char
	NetVMAsmBuffer[readBytes] = '\0';
	bytecode = nvmAssembleNetILFromBuffer(NetVMAsmBuffer, ErrBuf);
	if (bytecode == NULL)
	{
		free(NetVMAsmBuffer);
		return NULL;
	}

	free(NetVMAsmBuffer);
	return bytecode;
}

nvmByteCode *nvmAssembleNetILFromBuffer(char *asmBuffer, char *ErrBuf)
{
	pseg v;
	nvmByteCode *NetVMBinary;
	nvmByteCode *bytecode;
	char *ContiguousBinary;
	yy_set_error_buffer(ErrBuf, nvmERRBUF_SIZE);
	yy_set_warning_buffer(ErrBuf, nvmERRBUF_SIZE);

	NETVM_ASSERT(asmBuffer != NULL, "FileName cannot be NULL");

	v = yy_assemble(asmBuffer, 0);

	if (v == NULL)
	{
		return NULL;
	}

	NetVMBinary = nvmCreateBytecode(0, 0, 0);

	if (NetVMBinary == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}

	NetVMBinary->TargetFile= NULL;

	// Save the segments
	while (v != NULL)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_BYTECODE_LOAD_SAVE, "Saving section %s, len=%d, flags=%d\n", v->label, v->cur_ip, v->flags);
#endif

		AddSection(NetVMBinary, v->label, v->flags, v->istream, v->cur_ip);
		v = v->next;
	}

	// [GM] I don't like that "8"... Maybe the correct offset should be written in the bytecode itself!
	if (SetInitEntryPoint(NetVMBinary, ".init", 8) == nvmFAILURE)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Error, cannot create .init code");
		free(NetVMBinary);
		return NULL;
	}

	if (SetPushEntryPoint(NetVMBinary, ".push", 8) == nvmFAILURE)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Error, cannot create .push code");
		free(NetVMBinary);
		return NULL;
	}

	if (SetPullEntryPoint(NetVMBinary, ".pull", 8) == nvmFAILURE)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Error, cannot create .pull code");
		free(NetVMBinary);
		return NULL;
	}

	// FULVIO PER GIANLUCA: cosa diavolo vuol dire questo commento?
	// The parser doesn't create a continuos executable, but we need it if we run it without saving it
	ContiguousBinary= (char*)malloc(sizeof(nvmByteCodeImageHeader) +
											NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader) +
											NetVMBinary->SizeOfSections);

	if (ContiguousBinary == NULL)
	{
		free(NetVMBinary);
		return NULL;
	}

	memcpy(ContiguousBinary, NetVMBinary->Hdr, sizeof(nvmByteCodeImageHeader));

	memcpy(ContiguousBinary + sizeof(nvmByteCodeImageHeader),
			NetVMBinary->SectionsTable,
			NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader));

	memcpy(ContiguousBinary + sizeof(nvmByteCodeImageHeader) + NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader),
			NetVMBinary->Sections,
			NetVMBinary->SizeOfSections);

	bytecode = calloc(1, sizeof(nvmByteCode));
	if (bytecode == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}
	bytecode->TargetFile= NULL;
	bytecode->Hdr= (nvmByteCodeImageHeader *) ContiguousBinary;
	bytecode->SectionsTable= (nvmByteCodeSectionHeader*)(ContiguousBinary + sizeof(nvmByteCodeImageHeader));
	bytecode->Sections= ContiguousBinary + sizeof(nvmByteCodeImageHeader) +
										NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader);
	bytecode->SizeOfSections= NetVMBinary->SizeOfSections;

	// Free manually because we don't have a single buffer
	free(NetVMBinary->SectionsTable);
	free(NetVMBinary->Sections);
	free(NetVMBinary->Hdr);
	free(NetVMBinary);

	//printf("Bytecode Info:\n");
	//nvmDumpBytecodeInfo(stdout, bytecode);

	return bytecode;
}


nvmByteCode *nvmLoadBytecodeImage(char *FileName, char *ErrBuf)
{
nvmByteCode *bytecode;
FILE	*file;
unsigned int FileSize;
uint32_t i;

	NETVM_ASSERT(FileName != NULL, "FileName cannot be NULL");

	// Open the bytecode file
	file = fopen(FileName, "rb");

#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_BYTECODE_LOAD_SAVE, "nvmBytecode_Open: loading the bytecode %s\n", FileName);
#endif

	if (file == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Error opening the file %s for reading", FileName);
		return NULL;
	}

	// Load the bytecode file size in byte
	fseek(file, 0, SEEK_END);
	FileSize= ftell(file);

	// Move to the beginning of the file
	fseek(file, 0, SEEK_SET);

	bytecode = calloc(1, sizeof(nvmByteCode));
	if (bytecode == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}

	// Create the header of bytecode
	bytecode->Hdr = (nvmByteCodeImageHeader *) malloc(FileSize);

	if (bytecode->Hdr == NULL)
	{
		free(bytecode);
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}


	// Load bytecode header from file to nvmByteCode structure
	fread(bytecode->Hdr, FileSize, 1, file);
	fclose(file);

	// Check the bytecode validation signature
	if (bytecode->Hdr->Signature != 0x45444F4C) //i.e. 'EDOL'
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "The file %s does not appear to contain a valid NetVM bytecode image (bad signature)", FileName);
		nvmDestroyBytecode(bytecode);
		fclose(file);
		return NULL;
	}

	// First byte of section table section, after header section
	bytecode->SectionsTable = (nvmByteCodeSectionHeader*)((uint8_t *)bytecode->Hdr +
		sizeof(nvmByteCodeImageHeader) +
		bytecode->Hdr->FileHeader.SizeOfOptionalHeader -
		sizeof(nvmByteCodeImageStructOpt));

	// First byte of section table section, it's bytecode
	bytecode->Sections = (char*)&bytecode->SectionsTable[bytecode->Hdr->FileHeader.NumberOfSections];

	// Sections INIT PORTABLE PUSH PULL
	bytecode->SizeOfSections = 0;

	for (i=0; i < bytecode->Hdr->FileHeader.NumberOfSections; i++)
	{
		//compute the total size of section
		bytecode->SizeOfSections += bytecode->SectionsTable[i].SizeOfRawData;
	}

	return bytecode;
}



int32_t nvmCreateBytecodeFromAsmBuffer(nvmByteCode *bytecode, char *NetVMAsmBuffer)
{
pseg v;
nvmByteCode * NetVMBinary;
char *ContiguousBinary;
char ErrBuf[2048] = "";
yy_set_error_buffer(ErrBuf, sizeof(ErrBuf));
yy_set_warning_buffer(ErrBuf, sizeof(ErrBuf));

	if (NetVMAsmBuffer == NULL)
	{
		return nvmFAILURE;
	}

	v = yy_assemble(NetVMAsmBuffer, 0);

	if (v == NULL)
	{
		return nvmFAILURE;
	}

	if (strlen(ErrBuf) > 0)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_BYTECODE_LOAD_SAVE, "Warning, assembler warnings:\n%s", ErrBuf);
#endif
	}

	NetVMBinary = nvmCreateBytecode(0, 0, 0);

	if (NetVMBinary == NULL)
		return nvmFAILURE;

	NetVMBinary->TargetFile= NULL;

	// Save the segments
	while (v != NULL)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_BYTECODE_LOAD_SAVE, "Saving section %s, len=%d, flags=%d\n", v->label, v->cur_ip, v->flags);
#endif

		AddSection(NetVMBinary, v->label, v->flags, v->istream, v->cur_ip);
		v = v->next;
	}

	// [GM] I don't like that "8"... Maybe the correct offset should be written in the bytecode itself!
	if (SetInitEntryPoint(NetVMBinary, ".init", 8) == nvmFAILURE)
	{
		free(NetVMBinary);
		return nvmFAILURE;
	}

	if (SetPushEntryPoint(NetVMBinary, ".push", 8) == nvmFAILURE)
	{
		free(NetVMBinary);
		return nvmFAILURE;
	}

	if (SetPullEntryPoint(NetVMBinary, ".pull", 8) == nvmFAILURE)
	{
		free(NetVMBinary);
		return nvmFAILURE;
	}

	// FULVIO PER GIANLUCA: cosa diavolo vuol dire questo commento?
	// The parser doesn't create a continuos executable, but we need it if we run it without saving it
	ContiguousBinary= (char*)malloc(sizeof(nvmByteCodeImageHeader) +
											NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader) +
											NetVMBinary->SizeOfSections);

	if (ContiguousBinary == NULL)
	{
		free(NetVMBinary);
		return nvmFAILURE;
	}

	memcpy(ContiguousBinary, NetVMBinary->Hdr, sizeof(nvmByteCodeImageHeader));

	memcpy(ContiguousBinary + sizeof(nvmByteCodeImageHeader),
			NetVMBinary->SectionsTable,
			NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader));

	memcpy(ContiguousBinary + sizeof(nvmByteCodeImageHeader) + NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader),
			NetVMBinary->Sections,
			NetVMBinary->SizeOfSections);

	bytecode->TargetFile= NULL;
	bytecode->Hdr= (nvmByteCodeImageHeader *) ContiguousBinary;
	bytecode->SectionsTable= (nvmByteCodeSectionHeader*)(ContiguousBinary + sizeof(nvmByteCodeImageHeader));
	bytecode->Sections= ContiguousBinary + sizeof(nvmByteCodeImageHeader) +
										NetVMBinary->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader);
	bytecode->SizeOfSections= NetVMBinary->SizeOfSections;

	// Free manually because we don't have a single buffer
	free(NetVMBinary->SectionsTable);
	free(NetVMBinary->Sections);
	free(NetVMBinary->Hdr);
	free(NetVMBinary);

	return nvmSUCCESS;
}


int32_t nvmOpenBinaryFile(nvmByteCode *nvmBytecode, char *FileName)
{
unsigned int FileSize;
uint32_t i;

	NETVM_ASSERT(nvmBytecode != NULL, "nvmBytecode cannot be NULL");
	NETVM_ASSERT(FileName != NULL, "FileName cannot be NULL");

	// Open the bytecode file
	nvmBytecode->TargetFile= fopen(FileName, "rb");

#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_BYTECODE_LOAD_SAVE, "nvmBytecode_Open: loading the bytecode %s\n", FileName);
#endif

	if (nvmBytecode->TargetFile == NULL)
	{
		//DELETE_PTR(bytecode);
		return nvmFAILURE;
	}

	// Load the bytecode file size in byte
	fseek(nvmBytecode->TargetFile, 0, SEEK_END);
	FileSize= ftell(nvmBytecode->TargetFile);

	// Move to the beginning of the file
	fseek(nvmBytecode->TargetFile, 0, SEEK_SET);

	// Create the header of bytecode
	nvmBytecode->Hdr = (nvmByteCodeImageHeader *) malloc(FileSize);

	if (nvmBytecode->Hdr == NULL)
	{
		//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error creating the structure for keeping the NetVM bytecode.");
		//DELETE_PTR(bytecode);
		return nvmFAILURE;
	}

	// Load bytecode header from file to nvmByteCode structure
	fread(nvmBytecode->Hdr, FileSize, 1, nvmBytecode->TargetFile);
	fclose(nvmBytecode->TargetFile);

	// Check the bytecode validation signature
	if (nvmBytecode->Hdr->Signature != 0x45444F4C) //i.e. 'EDOL'
	{
		//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file '%s' does not contain a valid NetVM bytecode.", FileName);
		//DELETE_PTR(bytecode);
		fclose(nvmBytecode->TargetFile);
		return nvmFAILURE;
	}

	// First byte of section table section, after header section
	nvmBytecode->SectionsTable = (nvmByteCodeSectionHeader*)((char *)nvmBytecode->Hdr +
		sizeof(nvmByteCodeImageHeader) +
		nvmBytecode->Hdr->FileHeader.SizeOfOptionalHeader -
		sizeof(nvmByteCodeImageStructOpt));

	//First byte of section table section, it's bytecode
	nvmBytecode->Sections = (char*)&nvmBytecode->SectionsTable[nvmBytecode->Hdr->FileHeader.NumberOfSections];

	//sezioni INIT PORTABLE PUSH PULL
	nvmBytecode->SizeOfSections = 0;
	for (i=0; i<nvmBytecode->Hdr->FileHeader.NumberOfSections; i++)
		//compute the total size of section
		nvmBytecode->SizeOfSections += nvmBytecode->SectionsTable[i].SizeOfRawData;


	return nvmSUCCESS;
}



int32_t nvmSaveBinaryFile(nvmByteCode *bytecode, char *FileName, char *errbuf)
{
	NETVM_ASSERT(bytecode != NULL, "NULL bytecode");
	NETVM_ASSERT(FileName != NULL, "NULL filename");

	bytecode->TargetFile= fopen(FileName, "wb");

	if (bytecode->TargetFile == NULL)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Error writing the target file for saving the NetVM bytecode.");
		//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error opening the target file for saving the NetVM bytecode.");
		return nvmFAILURE;
	}

	// Write the whole stuff to file
	// Write the header
	if (fwrite(bytecode->Hdr, sizeof(nvmByteCodeImageHeader), 1, bytecode->TargetFile) != 1)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Error writing the target file for saving the NetVM bytecode.");

		//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error writing the target file for saving the NetVM bytecode.");
		fclose(bytecode->TargetFile);
		bytecode->TargetFile= NULL;
		return nvmFAILURE;
	}

	// Write the sections table
	if (fwrite(bytecode->SectionsTable, bytecode->Hdr->FileHeader.NumberOfSections * sizeof(nvmByteCodeSectionHeader), 1, bytecode->TargetFile) != 1)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Error writing the target file for saving the NetVM bytecode.");
		//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error opening the target file for saving the NetVM bytecode.");
		fclose(bytecode->TargetFile);
		bytecode->TargetFile= NULL;
		return nvmFAILURE;
	}

	// Write the sections
	if (fwrite(bytecode->Sections, bytecode->SizeOfSections, 1, bytecode->TargetFile) != 1)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Error writing the target file for saving the NetVM bytecode.");
		//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error opening the target file for saving the NetVM bytecode.");
		fclose(bytecode->TargetFile);
		bytecode->TargetFile= NULL;
		return nvmFAILURE;
	}

	return nvmSUCCESS;

}


