/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file bytecode_segments.c
 * \brief This file contains the main functions for the creation of Bytecode Segments Information
 *
 */


#include <netvm_bytecode.h>
#include "bytecode_segments.h"
#include "helpers.h"

#include "../../nbee/globals/debug.h"


int32_t nvmJitFill_Segments_Info(nvmByteCodeSegmentsInfo *segmentsInfo, nvmByteCode *bytecode, char* errbuf)
{
	uint32_t i = 0, j = 0;

	uint8_t *segmentsByteCode = NULL;
	uint8_t *byte_stream = NULL;
	NETVM_ASSERT(segmentsInfo != NULL && bytecode != NULL, " NULL arguments");

	segmentsByteCode = (uint8_t *)bytecode->Hdr;

	for (i = 0; i < bytecode->Hdr->FileHeader.NumberOfSections; i++)
	{
		switch (bytecode->SectionsTable[i].SectionFlag)
		{
			case BC_CODE_SCN|BC_PUSH_SCN:
				segmentsInfo->PushSegment.LocalsSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + LOCALS_SIZE_OFFS]);
				segmentsInfo->PushSegment.MaxStackSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + MAX_STACK_SIZE_OFFS]);
				segmentsInfo->PushSegment.ByteCode = (uint8_t *)&segmentsByteCode[bytecode->Hdr->FileHeader.AddressOfPushEntryPoint];
				segmentsInfo->PushSegment.SegmentSize = bytecode->SectionsTable[i].SizeOfRawData - SEGMENT_HEADER_LEN;
				segmentsInfo->PushSegment.Name = (char*)bytecode->SectionsTable[i].Name;
				segmentsInfo->PushSegment.SegmentType = PUSH_SEGMENT;
				break;
			case BC_CODE_SCN|BC_PULL_SCN:
				segmentsInfo->PullSegment.LocalsSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + LOCALS_SIZE_OFFS]);
				segmentsInfo->PullSegment.MaxStackSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + MAX_STACK_SIZE_OFFS]);
				segmentsInfo->PullSegment.ByteCode = (uint8_t *)&segmentsByteCode[bytecode->Hdr->FileHeader.AddressOfPullEntryPoint];
				segmentsInfo->PullSegment.SegmentSize = bytecode->SectionsTable[i].SizeOfRawData - SEGMENT_HEADER_LEN;
				segmentsInfo->PullSegment.Name = (char*)bytecode->SectionsTable[i].Name;
				segmentsInfo->PullSegment.SegmentType = PULL_SEGMENT;
				break;
			case BC_CODE_SCN|BC_INIT_SCN:
				segmentsInfo->InitSegment.LocalsSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + LOCALS_SIZE_OFFS]);
				segmentsInfo->InitSegment.MaxStackSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + MAX_STACK_SIZE_OFFS]);
				segmentsInfo->InitSegment.ByteCode = (uint8_t *)&segmentsByteCode[bytecode->Hdr->FileHeader.AddressOfInitEntryPoint];
				segmentsInfo->InitSegment.SegmentSize = bytecode->SectionsTable[i].SizeOfRawData - SEGMENT_HEADER_LEN;
				segmentsInfo->InitSegment.Name = (char*)bytecode->SectionsTable[i].Name;
				segmentsInfo->InitSegment.SegmentType = INIT_SEGMENT;
				break;
			case BC_PORT_SCN:
				segmentsInfo->InitSegment.LocalsSize = 0;
				segmentsInfo->InitSegment.MaxStackSize = 0;
				segmentsInfo->InitSegment.ByteCode = (uint8_t *)&bytecode->Hdr[bytecode->SectionsTable[i].PointerToRawData];
				segmentsInfo->InitSegment.SegmentSize = bytecode->SectionsTable[i].SizeOfRawData;
				segmentsInfo->InitSegment.Name = (char*)bytecode->SectionsTable[i].Name;
				segmentsInfo->InitSegment.SegmentType = PORT_SEGMENT;
				break;
			case BC_INSN_LINES_SCN|BC_PUSH_SCN:
				j = 0;
				byte_stream = (uint8_t *)&bytecode->Hdr[bytecode->SectionsTable[i].PointerToRawData];
				while(j < bytecode->SectionsTable[i].SizeOfRawData)
				{
					uint32_t ip, line;
					ip = *(uint32_t*)&byte_stream[j];
					line = *(uint32_t*)&byte_stream[j];
					segmentsInfo->PushSegment.Insn2LineMap.insert(std::pair<uint32_t,uint32_t>(ip, line));
				}
				break;
			case BC_INSN_LINES_SCN|BC_PULL_SCN:
				j = 0;
				byte_stream = (uint8_t *)&bytecode->Hdr[bytecode->SectionsTable[i].PointerToRawData];
				while(j < bytecode->SectionsTable[i].SizeOfRawData)
				{
					uint32_t ip, line;
					ip = *(uint32_t*)&byte_stream[j];
					line = *(uint32_t*)&byte_stream[j];
					segmentsInfo->PullSegment.Insn2LineMap.insert(std::pair<uint32_t,uint32_t>(ip, line));
				}
				break;
			case BC_INSN_LINES_SCN|BC_INIT_SCN:
				j = 0;
				byte_stream = (uint8_t *)&bytecode->Hdr[bytecode->SectionsTable[i].PointerToRawData];
				while(j < bytecode->SectionsTable[i].SizeOfRawData)
				{
					uint32_t ip, line;
					ip = *(uint32_t*)&byte_stream[j];
					line = *(uint32_t*)&byte_stream[j];
					segmentsInfo->InitSegment.Insn2LineMap.insert(std::pair<uint32_t,uint32_t>(ip, line));
				}
				break;
		}


	}
	return nvmSUCCESS;

}
