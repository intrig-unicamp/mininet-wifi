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


#include <stdio.h>
#include "fieldprinter.h"
#include "../utils/utils.h"

#ifdef PROFILING
extern nbProfiler* ProfilerFormatFields;
#endif

void CFieldPrinter::Initialize(nbNetPDLUtils* NetPDLUtils, PrintingMode_t PrintingMode, AnonymizationMapTable_t AnonymizationIPTable, AnonymizationArgumentList_t AnonymizationIPArgumentList, FILE *OutputFile)
{
	m_NetPDLUtils= NetPDLUtils;
	m_PrintingMode= PrintingMode;
	m_OutputFile= OutputFile;
	m_AnonymizationIPArgumentList= AnonymizationIPArgumentList;
	m_AnonymizationIPTable= AnonymizationIPTable;
	m_FormattedField[0]= 0;
}


void CFieldPrinter::PrintField(_nbExtractedFieldsDescriptor &FieldDescriptor, _nbExtractFieldInfo &FieldInfo, _nbExtractFieldInfo AllFieldsInfo[], int FieldNumber, const unsigned char *PktData)
{
_nbExtractedFieldsDescriptor *FieldsDescriptorVector= FieldDescriptor.DVct;
int RetVal;

	switch (FieldInfo.FieldType)
	{
	case PDL_FIELD_TYPE_ALLFIELDS:
		{
			if (FieldDescriptor.Valid)
			{
				for (int i=0; i < FieldDescriptor.num_entries; i++)
					PrintField(FieldsDescriptorVector[i], AllFieldsInfo[i], AllFieldsInfo, FieldNumber, PktData);
			}
		};
		break;

	case PDL_FIELD_TYPE_BIT:
		{
			if (FieldDescriptor.Valid)
			{
				if (FieldsDescriptorVector != NULL)
				{
					for (int i=0; i < FieldDescriptor.num_entries; i++)
						PrintField(FieldsDescriptorVector[i], FieldInfo, AllFieldsInfo, FieldNumber, PktData);
					return;
				}

				switch (m_PrintingMode)
				{
					case DEFAULT:
						fprintf(m_OutputFile, "\t%s.%s : value=%u\n", FieldInfo.Proto, FieldInfo.Name, (unsigned int) FieldDescriptor.BitField_Value);
						break;

#ifdef ENABLE_REDIS
					case REDIS:
						ssnprintf(m_FormattedField, sizeof(m_FormattedField), "%u, ", (unsigned int) FieldDescriptor.BitField_Value);
						break;
#endif
#ifdef ENABLE_SQLITE3
					case SQLITE3:
						ssnprintf(m_FormattedField, sizeof(m_FormattedField), "%u, ", (unsigned int) FieldDescriptor.BitField_Value);
						break;
#endif
					default:
						fprintf(m_OutputFile, "%u, ", (unsigned int) FieldDescriptor.BitField_Value);
						break;
				}
			}
			else
			{
				switch (m_PrintingMode)
				{
					case DEFAULT:
						fprintf(m_OutputFile, "\t%s.%s : value=*\n", FieldInfo.Proto, FieldInfo.Name);
						break;

#ifdef ENABLE_REDIS
					case REDIS:
						ssnprintf(m_FormattedField, sizeof(m_FormattedField), "-");
						break;
#endif
#ifdef ENABLE_SQLITE3
					case SQLITE3:
						ssnprintf(m_FormattedField, sizeof(m_FormattedField), "-");
						break;
#endif
					default:
						fprintf(m_OutputFile, " , ");
						break;
				}
			}
		};
		break;

	default:
		{
			if (FieldDescriptor.Valid)
			{
				if (FieldsDescriptorVector != NULL)
				{
					for (int i=0; i < FieldDescriptor.num_entries; i++)
						PrintField(FieldsDescriptorVector[i], FieldInfo, AllFieldsInfo, FieldNumber, PktData);

					return;
				}
				if (FieldDescriptor.Length > MAX_FIELD_SIZE)
				{
					fprintf(stderr, "The size of field '%s' is larger than the max value allowed in this program. ",FieldInfo.Name);
					fprintf(stderr, "Max allowed: %d - Current: %d\n", MAX_FIELD_SIZE, FieldDescriptor.Length);
					return;
				}

#ifdef PROFILING
				int64_t StartTime, EndTime;

				StartTime= nbProfilerGetTime();
#endif
				// Here we use the 'UserExtension' member in order to store the 'fast printing' function code, if available.
				if (FieldDescriptor.UserExtension)
				{
					RetVal= m_NetPDLUtils->FormatNetPDLField((long) FieldDescriptor.UserExtension, PktData + FieldDescriptor.Offset,
						FieldDescriptor.Length, m_FormattedField, sizeof(m_FormattedField));
				}
				else
				{
					// If the raw packet dump is enough for you, you can get rid of this function
					RetVal= m_NetPDLUtils->FormatNetPDLField(FieldInfo.Proto, FieldInfo.Name,
						PktData + FieldDescriptor.Offset, FieldDescriptor.Length, m_FormattedField,
						sizeof(m_FormattedField));
				}

#ifdef PROFILING
				EndTime= nbProfilerGetTime();
				ProfilerFormatFields->StoreSample(StartTime, EndTime);
#endif

				if (RetVal == nbSUCCESS)
				{

					// Perform anonymization if needed
					// If this fields is found in the anonymization table, let's replace it with the correspondent anonimized value
					if (m_AnonymizationIPArgumentList.count(FieldNumber+1) > 0)
					{
						if (!m_AnonymizationIPTable[m_FormattedField].empty())
							// There may be a bug here... if the field contains '0' in it, the strncpy may fail to copy the entire data
							strncpy(m_FormattedField, m_AnonymizationIPTable[m_FormattedField].c_str(), sizeof(m_FormattedField));
					}
					
					switch (m_PrintingMode)
					{
						case DEFAULT:
							fprintf(m_OutputFile, "\t%s.%s: offset=%d len=%d value=%s\n", FieldInfo.Proto,FieldInfo.Name,
									FieldDescriptor.Offset, FieldDescriptor.Length, m_FormattedField);
							break;
#ifdef ENABLE_REDIS
						case REDIS:
							// Do nothing; the field is already in the m_FormattedField field
							break;
#endif

#ifdef ENABLE_SQLITE3
						case SQLITE3:
							// Do nothing; the field is already in the m_FormattedField field
							break;
#endif
						default:
							fprintf(m_OutputFile, "%s, ", m_FormattedField);
							break;
					}
				}
			}
			else
			{
				switch (m_PrintingMode)
				{
					case DEFAULT:
						fprintf(m_OutputFile, "\t%s.%s: offset= * len= * value= *\n", FieldInfo.Proto, FieldInfo.Name);
						break;

#ifdef ENABLE_REDIS
					case REDIS:
						ssnprintf(m_FormattedField, sizeof(m_FormattedField), "\0");
						break;
#endif
#ifdef ENABLE_SQLITE3
					case SQLITE3:
						ssnprintf(m_FormattedField, sizeof(m_FormattedField), "\0");
						break;
#endif
					default:
						fprintf(m_OutputFile, " , ");
						break;
				}
			}
		}
	}
}


char* CFieldPrinter::GetFormattedField()
{
	return m_FormattedField;
}
