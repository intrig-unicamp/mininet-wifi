/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file vm_management.c
 *	\brief This file contains the functions that implement the core of the NetVM framework.
 */

#include <nbnetvm.h>
#include <netvm_bytecode.h>
#include <string.h>
#include "int_structs.h"
#include "helpers.h"
#include "coprocessor.h"
#include "../nbee/globals/debug.h"




/*!
  \brief  Fills the PE structure  with information gathered from a bytecode image

  \param  PE		pointer to a NetPE object

  \param  bytecode	pointer to a nvmByteCode image

  \param  ErrBuf	buffer that handles the last error

  \return nvmSUCCESS if it succeeds, nvmFAILURE if it fails
*/

nvmRESULT nvmFillPEInfo(nvmNetPE *PE, nvmByteCode *bytecode, char *ErrBuf)
{
	uint32_t i = 0, p = 0, port_table_len = 0, k, j,len;
	char name[MAX_COPRO_NAME];
	uint8_t *copro_section_element;
	uint8_t *segmentsByteCode = NULL, *ptr;
	uint32_t *port_table = NULL;

	uint8_t *push_ILTable = NULL, *pull_ILTable = NULL, *init_ILTable = NULL;
	uint32_t push_ILTlen = 0, pull_ILTlen = 0, init_ILTlen = 0;

	NETVM_ASSERT(PE != NULL && bytecode != NULL && ErrBuf, " NULL arguments");

	segmentsByteCode = (uint8_t *)bytecode->Hdr;

	for (i = 0; i < bytecode->Hdr->FileHeader.NumberOfSections; i++)
	{
		switch (bytecode->SectionsTable[i].SectionFlag)
		{
			case BC_CODE_SCN|BC_PUSH_SCN:
				PE->PushHandler = nvmAllocObject(sizeof(nvmPEHandler), ErrBuf);
				if (PE->PushHandler == NULL)
				{
					errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
					return nvmFAILURE;
				}

				PE->PushHandler->NumLocals = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + LOCALS_SIZE_OFFS]);
				PE->PushHandler->MaxStackSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + MAX_STACK_SIZE_OFFS]);
				PE->PushHandler->ByteCode = (uint8_t *)&segmentsByteCode[bytecode->Hdr->FileHeader.AddressOfPushEntryPoint];
				PE->PushHandler->CodeSize = bytecode->SectionsTable[i].SizeOfRawData - SEGMENT_HEADER_LEN;
				PE->PushHandler->Name = (char*)bytecode->SectionsTable[i].Name;
				PE->PushHandler->HandlerType = PUSH_HANDLER;
				PE->PushHandler->OwnerPE = PE;
				PE->PushHandler->Insn2LineTable = NULL;
				PE->PushHandler->Insn2LineTLen = 0;
				break;
			case BC_INSN_LINES_SCN|BC_PUSH_SCN:
				push_ILTable = (uint8_t *)&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData];
				push_ILTlen = bytecode->SectionsTable[i].SizeOfRawData;
				break;

			case BC_CODE_SCN|BC_PULL_SCN:
				PE->PullHandler = nvmAllocObject(sizeof(nvmPEHandler), ErrBuf);
				if (PE->PullHandler == NULL)
				{
					errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
					return nvmFAILURE;
				}

				PE->PullHandler->NumLocals = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + LOCALS_SIZE_OFFS]);
				PE->PullHandler->MaxStackSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + MAX_STACK_SIZE_OFFS]);
				PE->PullHandler->ByteCode = (uint8_t *)&segmentsByteCode[bytecode->Hdr->FileHeader.AddressOfPullEntryPoint];
				PE->PullHandler->CodeSize = bytecode->SectionsTable[i].SizeOfRawData - SEGMENT_HEADER_LEN;
				PE->PullHandler->Name = (char*)bytecode->SectionsTable[i].Name;
				PE->PullHandler->HandlerType = PULL_HANDLER;
				PE->PullHandler->OwnerPE = PE;
				PE->PullHandler->Insn2LineTable = NULL;
				PE->PullHandler->Insn2LineTLen = 0;
				break;
			case BC_INSN_LINES_SCN|BC_PULL_SCN:
				pull_ILTable = (uint8_t *)&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData];
				pull_ILTlen = bytecode->SectionsTable[i].SizeOfRawData;
				break;

			case BC_CODE_SCN|BC_INIT_SCN:
				PE->InitHandler = nvmAllocObject(sizeof(nvmPEHandler), ErrBuf);
				if (PE->InitHandler == NULL)
				{
					errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
					return nvmFAILURE;
				}

				PE->InitHandler->NumLocals = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + LOCALS_SIZE_OFFS]);
				PE->InitHandler->MaxStackSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + MAX_STACK_SIZE_OFFS]);
				PE->InitHandler->ByteCode = (uint8_t *)&segmentsByteCode[bytecode->Hdr->FileHeader.AddressOfInitEntryPoint];
				PE->InitHandler->CodeSize = bytecode->SectionsTable[i].SizeOfRawData - SEGMENT_HEADER_LEN;
				PE->InitHandler->Name = (char*)bytecode->SectionsTable[i].Name;
				PE->InitHandler->HandlerType = INIT_HANDLER;
				PE->InitHandler->OwnerPE = PE;
				PE->InitHandler->Insn2LineTable = NULL;
				PE->InitHandler->Insn2LineTLen = 0;
				break;

			case BC_INSN_LINES_SCN|BC_INIT_SCN:
				init_ILTable = (uint8_t *)&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData];
				init_ILTlen = bytecode->SectionsTable[i].SizeOfRawData;
				break;

			case BC_PORT_SCN:
				port_table_len = *(uint32_t *) ((int8_t*)bytecode->Hdr + bytecode->SectionsTable[i].PointerToRawData);
				port_table = (uint32_t *) ((int8_t*)bytecode->Hdr + bytecode->SectionsTable[i].PointerToRawData + PORT_TABLE_OFFS);
				PE->PortTable = calloc(port_table_len, sizeof(nvmPEPort));
				if (PE->PortTable == NULL)
				{
					errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
					return nvmFAILURE;
				}

				for (p = 0; p < port_table_len; p++)
				{
					if (port_table[p] & nvmPORT_COLLECTOR)
						PORT_SET_TYPE_COLLECTOR(PE->PortTable[p].PortFlags);
					else
						PORT_SET_TYPE_EXPORTER(PE->PortTable[ p].PortFlags);

					if (port_table[p] & nvmCONNECTION_PULL)
						PORT_SET_DIR_PULL(PE->PortTable[p].PortFlags);
					else
						PORT_SET_DIR_PUSH(PE->PortTable[p].PortFlags);
				}
				PE->NPorts = port_table_len;
				break;

			case BC_INITIALIZED_DATA_SCN:
				PE->InitedMem = (uint8_t *) bytecode->Hdr + bytecode->SectionsTable[i].PointerToRawData;
				PE->InitedMemSize = bytecode->SectionsTable[i].SizeOfRawData;
				break;

			case BC_METADATA_SCN:
				/* 32bit for name length */
				k = *(uint32_t *) ((int8_t*)bytecode->Hdr + bytecode->SectionsTable[i].PointerToRawData);

				/* k characters forming PE name */
				ptr = (uint8_t *) ((int8_t*)bytecode->Hdr + bytecode->SectionsTable[i].PointerToRawData + 4);
				strncpy (PE->Name, (char*)ptr, k);
				PE->Name[k < MAX_NETPE_NAME ? k : MAX_NETPE_NAME] = '\0';

				/* 32bit for data memory size */
				PE->DataMemSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + 4 + k]);

				/* 32bit for info partition size */
				PE->InfoPartitionSize = PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + 4 + k + 4]);

				/* 32bit for coprocessor number */
				PE->NCopros =  PTR_TO_LONG(&segmentsByteCode[bytecode->SectionsTable[i].PointerToRawData + 4 + k + 4 + 4]);
				if (PE->NCopros > 0) {
					PE->Copros = calloc(PE->NCopros,4);

					/* For each coprocessor, 32 bit for coprocessor name length and thqt many characters for the actual name */
					copro_section_element = (uint8_t *) ((int8_t*)bytecode->Hdr + bytecode->SectionsTable[i].PointerToRawData + 4 + k + 4 + 4 + 4);
					for (k = 0; k < PE->NCopros; k++)
					{
						len = *(uint32_t *)copro_section_element;
						copro_section_element += 4;
						memcpy (name, copro_section_element, len);
						name[len] = '\0';
						copro_section_element += len;

						/* See if the requested coprocessor is available */
						for (j = 0; j < AVAILABLE_COPROS_NO; j++) {
							if (strcmp (name, nvm_copro_map[j].copro_name) == 0) {
								PE->Copros[k]=j;
								break;
							}
						}
						if (j >= AVAILABLE_COPROS_NO) {
							errorprintf(__FILE__, __FUNCTION__, __LINE__, "Coprocessor '%s' is not available\n", name);
							return (nvmFAILURE);
						}
					}
				}

				break;

			default:
				errorprintf(__FILE__, __FUNCTION__, __LINE__, "Unsupported section in NetIL code\n");
				return (nvmFAILURE);
				break;
		}

		for (p = 0; p < PE->NPorts; p++)
		{
			if (PORT_DIR(PE->PortTable[p].PortFlags) == PORT_DIR_PUSH\
				&& PORT_TYPE(PE->PortTable[p].PortFlags) == PORT_TYPE_EXPORTER)
			{
				if (PE->PushHandler == NULL)
				{
					errsnprintf(ErrBuf, nvmERRBUF_SIZE, "No valid handler available for PUSH COLLECTOR port");
					return nvmFAILURE;
				}
				PE->PortTable[p].Handler = PE->PushHandler;
			}
			else if (PORT_DIR(PE->PortTable[p].PortFlags) == PORT_DIR_PULL\
				&& PORT_TYPE(PE->PortTable[p].PortFlags) == PORT_TYPE_EXPORTER)
			{
				if (PE->PullHandler == NULL)
				{
					errsnprintf(ErrBuf, nvmERRBUF_SIZE, "No valid handler available for PULL EXPORTER port");
					return nvmFAILURE;
				}
				PE->PortTable[p].Handler = PE->PullHandler;
			}
		}
	}

	PE->InitHandler->Insn2LineTable = init_ILTable;
	PE->InitHandler->Insn2LineTLen = init_ILTlen;
	PE->PushHandler->Insn2LineTable = push_ILTable;
	PE->PushHandler->Insn2LineTLen = push_ILTlen;
	PE->PullHandler->Insn2LineTable = pull_ILTable;
	PE->PullHandler->Insn2LineTLen = pull_ILTlen;

	return nvmSUCCESS;
}


nvmNetPE *nvmAllocPE(char* ErrBuf)
{
	nvmNetPE *netPE = nvmAllocObject(sizeof(nvmNetPE), ErrBuf);
	if (netPE == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}
	return netPE;

}

void nvmDestroyPE(nvmNetPE *PE)
{
	free(PE->InitHandler);
	free(PE->PushHandler);
	free(PE->PullHandler);
	free(PE->PortTable);
	free(PE);
}


void nvmRegisterPE(nvmNetVM *VMObj, nvmNetPE *PE)
{
	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(PE != NULL, "PE cannot be NULL");
	SLLst_Add_Item_Tail(VMObj->NetPEs, PE);
	PE->OwnerVM = VMObj;
}

void nvmRegisterSocket(nvmNetVM *VMObj, nvmSocket *Socket)
{
	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(Socket != NULL, "Socket cannot be NULL");
	SLLst_Add_Item_Tail(VMObj->Sockets, Socket);
	Socket->OwnerVM = VMObj;
}

nvmNetVM *nvmCreateVM(nvmNETVMFLAGS flags, char* ErrBuf)
{
	nvmNetVM *netVM = nvmAllocObject(sizeof(nvmNetVM), ErrBuf);
	if (netVM == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}

	netVM->NetPEs= New_SingLinkedList(NOT_SORTED_LST);
	netVM->Sockets= New_SingLinkedList(NOT_SORTED_LST);
	return netVM;
}




void nvmDestroyVM(nvmNetVM *VMObj)
{
	if (VMObj->NetPEs != NULL)
		Free_SingLinkedList(VMObj->NetPEs, (nvmFreeFunct *)nvmDestroyPE);
	if (VMObj->Sockets != NULL)
		Free_SingLinkedList(VMObj->Sockets, free);

	free(VMObj);
}


//FIXME OM: this function is useless!
#if 0
nvmRESULT nvmInitVM(nvmNetVM *VMObj, char* ErrBuf)
{
	return nvmSUCCESS;
}

#endif


nvmNetPE *nvmLoadPE(nvmNetVM *VMObj, char *filename, char* ErrBuf)
{
	nvmByteCode *bytecode = NULL;
	nvmNetPE *netPE = NULL;
	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(filename != NULL, "filename cannot be NULL");
	NETVM_ASSERT(ErrBuf != NULL, "ErrBuf cannot be NULL");


	bytecode = nvmLoadBytecodeImage(filename,ErrBuf);
	if (bytecode == NULL)
		return NULL;

	netPE = nvmCreatePE(VMObj, bytecode, ErrBuf);
	nvmSetPEFileName(netPE, filename);
	return netPE;
}


nvmNetPE *nvmCreatePE(nvmNetVM *VMObj, nvmByteCode *bytecode, char* ErrBuf)
{
	nvmNetPE *netPE = NULL;

	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(bytecode != NULL, "bytecode cannot be NULL");
	NETVM_ASSERT(ErrBuf != NULL, "ErrBuf cannot be NULL");
	netPE = nvmAllocPE(ErrBuf);
	if (netPE == NULL)
		return NULL;
	if (nvmFillPEInfo(netPE, bytecode, ErrBuf) == nvmFAILURE)
		return NULL;
	nvmRegisterPE(VMObj, netPE);
	netPE->FileName[0] = '\0';
	return netPE;
}


void nvmSetPEFileName(nvmNetPE *netPE, char *filename)
{
	errsnprintf(netPE->FileName, MAX_NETPE_NAME, "%s", filename);
}


nvmSocket *nvmCreateSocket(nvmNetVM *VMObj, char* ErrBuf)
{
	nvmSocket *socket = NULL;

	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(ErrBuf != NULL, "ErrBuf cannot be NULL");

	socket = nvmAllocObject(sizeof(nvmSocket), ErrBuf);
	if (socket == NULL)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}

	nvmRegisterSocket(VMObj, socket);

	return socket;
}


nvmRESULT nvmConnectSocket2PE(nvmNetVM *VMObj, nvmSocket *Socket, nvmNetPE *PE, uint32_t port, char *ErrBuf)
{
	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(Socket != NULL, "Socket cannot be NULL");
	NETVM_ASSERT(PE != NULL, "PE cannot be NULL");
	NETVM_ASSERT(ErrBuf != NULL, "ErrBuf cannot be NULL");
	NETVM_ASSERT(PE->OwnerVM == VMObj, "PE does not belong to VM");
	NETVM_ASSERT(Socket->OwnerVM == VMObj, "Socket does not belong to VM");
	NETVM_ASSERT(Socket->CtdPE == NULL, "Socket already connected to a NetPE");
	Socket->CtdPE = PE;
	if (port >= PE->NPorts)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Port %d is out of range for PE %s", port, PE->Name);
		return nvmFAILURE;
	}
	Socket->CtdPort = port;
	PE->PortTable[port].CtdSocket = Socket;
	PORT_SET_CONN_SOCK(PE->PortTable[port].PortFlags);
	return nvmSUCCESS;
}


nvmBOOL nvmCheckPEPorts(nvmPEPort *port1, nvmPEPort *port2, char *ErrBuf)
{
	if (PORT_DIR(port1->PortFlags) != PORT_DIR(port2->PortFlags))
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Port1 and Port2 must be in the same direction");
		return nvmFALSE;
	}
	if (PORT_TYPE(port1->PortFlags) == PORT_TYPE(port2->PortFlags))
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Port1 and Port2 must be of different type");
		return nvmFALSE;
	}
	if (PORT_IS_EXPORTER(port1->PortFlags) && PORT_IS_COLLECTOR(port2->PortFlags))
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Port1 must be an exporter and Port2 must be a collector");
		return nvmFALSE;
	}

	return nvmTRUE;
}

nvmRESULT nvmConnectPE2PE(nvmNetVM *VMObj, nvmNetPE *PE1, uint32_t port1, nvmNetPE *PE2, uint32_t port2, char *ErrBuf)
{
	NETVM_ASSERT(VMObj != NULL, "VMObj cannot be NULL");
	NETVM_ASSERT(PE1 != NULL, "PE1 cannot be NULL");
	NETVM_ASSERT(PE2 != NULL, "PE2 cannot be NULL");
	NETVM_ASSERT(ErrBuf != NULL, "ErrBuf cannot be NULL");
	NETVM_ASSERT(PE1->OwnerVM == VMObj, "PE1 does not belong to VM");
	NETVM_ASSERT(PE2->OwnerVM == VMObj, "PE2 does not belong to VM");

	if (port1 >= PE1->NPorts)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Port %d is out of range for PE %s", port1, PE1->Name);
		return nvmFAILURE;
	}

	if (port2 >= PE2->NPorts)
	{
		errsnprintf(ErrBuf, nvmERRBUF_SIZE, "Port %d is out of range for PE %s", port2, PE2->Name);
		return nvmFAILURE;
	}


	if (!nvmCheckPEPorts(&PE1->PortTable[port1], &PE2->PortTable[port2], ErrBuf))
		return nvmFAILURE;

	PE1->PortTable[port1].CtdPE = PE2;
	PE2->PortTable[port2].CtdPE = PE1;
	PE1->PortTable[port1].CtdPort = port2;
	PE2->PortTable[port2].CtdPort = port1;
	PORT_SET_CONN_PE(PE1->PortTable[port1].PortFlags);
	PORT_SET_CONN_PE(PE2->PortTable[port2].PortFlags);
	return nvmSUCCESS;
}
