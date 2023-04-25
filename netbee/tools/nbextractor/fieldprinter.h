/*
 * Copyright (c) 2002 - 2011
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition
 * is met:
 *
 * Neither the name of the Politecnico di Torino nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#pragma once


#include <nbee.h>
#include "configparams.h"
#include "anonimize-ip.h"

#define MAX_FIELD_SIZE 128000 

class CFieldPrinter
{
	PrintingMode_t m_PrintingMode;
	nbNetPDLUtils* m_NetPDLUtils;
	FILE *m_OutputFile;
	AnonymizationMapTable_t m_AnonymizationIPTable;
	AnonymizationArgumentList_t m_AnonymizationIPArgumentList;
	char m_FormattedField[MAX_FIELD_SIZE];


public:
	void Initialize(nbNetPDLUtils* NetPDLUtils, PrintingMode_t PrintingMode, AnonymizationMapTable_t AnonymizationIPTable, AnonymizationArgumentList_t AnonymizationIPArgumentList, FILE *OutputFile);
	void PrintField(_nbExtractedFieldsDescriptor &FieldDescriptor, _nbExtractFieldInfo &FieldInfo, _nbExtractFieldInfo AllFieldsInfo[], int FieldNumber, const unsigned char *PktData);
	char* GetFormattedField();
};
