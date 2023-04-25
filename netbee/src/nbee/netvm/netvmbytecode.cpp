/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdlib.h>
#include <time.h>

#include <nbee_netvm.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"

#ifndef WIN32
#include <string.h>
#endif


nbNetVmByteCode::nbNetVmByteCode()
{
	//open the bytecode of the ip filter
	ByteCode = (nvmByteCode*) malloc(sizeof(nvmByteCode));
	if (ByteCode == NULL)
		printf("Cannot create the structure used to load the bytecode in the NetPE\n");
};

nbNetVmByteCode::~nbNetVmByteCode()
{
	
};


void nbNetVmByteCode::DumpBytecode(FILE *file)
{
	nvmDumpBytecodeInfo(file, GetNetVmByteCodePointer());
}

int nbNetVmByteCode::CreateBytecodeFromAsmFile(char *FileName)
{
	//open the bytecode file and fillup the ByteCode structure
	if ( nvmCreateBytecodeFromAsmFile(this->GetNetVmByteCodePointer(), FileName) != nvmSUCCESS )
	{
		printf("Cannot open the bytecode file\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}


int nbNetVmByteCode::CreateBytecodeFromAsmBuffer(char *NetVMAsmBuffer)
{
	//open the bytecode file and fillup the ByteCode structure
	if ( nvmCreateBytecodeFromAsmBuffer(this->GetNetVmByteCodePointer(), NetVMAsmBuffer) != nvmSUCCESS )
	{
		printf("Cannot open the bytecode buffer\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}


int nbNetVmByteCode::SaveBinaryFile(char *FileName)
{
	
	if( nvmSaveBinaryFile(this->GetNetVmByteCodePointer(), FileName)!= nvmSUCCESS )	
	{
		printf("Cannot Save Binary file\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
	
}


int nbNetVmByteCode::OpenBinaryFile(char *FileName)
{
	if( nvmOpenBinaryFile(this->GetNetVmByteCodePointer(), FileName)!= nvmSUCCESS )	
	{
		printf("Cannot open the binary file\n");
		return nbFAILURE;
	}
	return nbSUCCESS;	
}

void nbNetVmByteCode::close(void)
{
		
}

/*nvmByteCode * nbNetVmByteCode::Create(u_int32_t DefaultDataMemSize, u_int32_t DefaultExchangeBufferSize, u_int32_t DefaultStackSize)
{
nvmByteCode *SaveBin;

	SaveBin= new nvmByteCode;
	if (SaveBin == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error creating the structure for keeping the NetVM bytecode.");
		return NULL;
	}

	SaveBin->SectionsTable = NULL;
	SaveBin->Sections = NULL;
	SaveBin->SizeOfSections = 0;

	SaveBin->Hdr= new nvmByteCodeImageHeader;
	if (SaveBin->Hdr == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error creating the structure for keeping the NetVM bytecode.");
		delete SaveBin;
		return NULL;
	}

	// Init header
	SaveBin->Hdr->Signature = 'EDOL';
	SaveBin->Hdr->FileHeader.NumberOfSections = 0;
	time( (time_t *) &(SaveBin->Hdr->FileHeader.TimeDateStamp) );
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
}*/

/*u_int32_t nbNetVmByteCode::AddSection(nvmByteCode * SaveBin, char* SectName, u_int32_t SectFlags, char* SectBody, u_int32_t SectSize)
{
	nvmByteCodeSectionHeader *TmpSectionsTable;
	char *TmpSections;
	int i;

	// Update the Section Table
	// Reallocate the memory for the Sections Table calculating the size with the new Section
	TmpSectionsTable = (nvmByteCodeSectionHeader *) realloc(SaveBin->SectionsTable, (SaveBin->Hdr->FileHeader.NumberOfSections + 1) * sizeof(nvmByteCodeSectionHeader));

	if (TmpSectionsTable == NULL)
		return nbFAILURE;

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
		return nbFAILURE;

	SaveBin->Sections = TmpSections;

	memcpy(SaveBin->Sections + SaveBin->SizeOfSections, SectBody, SectSize);

	// Update the state
	SaveBin->Hdr->FileHeader.NumberOfSections++;
	SaveBin->SizeOfSections += SectSize;

	return nbSUCCESS;
}*/



/*u_int32_t nbNetVmByteCode::SetInitEntryPoint(nvmByteCode *SaveBin, char *SectName, u_int32_t addr)
{
int i;
	
	for (i=0; i<SaveBin->Hdr->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(SectName,	(char *) SaveBin->SectionsTable[i].Name) == 0)
		{
			SaveBin->Hdr->FileHeader.AddressOfInitEntryPoint= SaveBin->SectionsTable[i].PointerToRawData + addr;
			return nbSUCCESS;
		}
	}

	return nbFAILURE;
}*/



/*u_int32_t nbNetVmByteCode::SetPushEntryPoint(nvmByteCode *SaveBin, char *SectName, u_int32_t addr)
{
int i;
	
	for (i=0; i<SaveBin->Hdr->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(SectName, (char *) SaveBin->SectionsTable[i].Name) == 0)
		{
			SaveBin->Hdr->FileHeader.AddressOfPushEntryPoint= SaveBin->SectionsTable[i].PointerToRawData + addr;
			return nbSUCCESS;
		}
	}

	return nbFAILURE;
}*/


/*u_int32_t nbNetVmByteCode::SetPullEntryPoint(nvmByteCode *SaveBin, char *SectName, u_int32_t addr)
{
	int i;
	
	for(i=0; i<SaveBin->Hdr->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(SectName, (char *) SaveBin->SectionsTable[i].Name) == 0)
		{
			SaveBin->Hdr->FileHeader.AddressOfPullEntryPoint= SaveBin->SectionsTable[i].PointerToRawData + addr;
			return nbSUCCESS;
		}
	}

	return nbFAILURE;
}*/