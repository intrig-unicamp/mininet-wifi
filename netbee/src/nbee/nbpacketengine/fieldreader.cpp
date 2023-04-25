/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "nbeefieldreader.h"
#include <iostream>


nbeeFieldReader::nbeeFieldReader(_nbExtractedFieldsDescriptor* ndescriptorVct, _nbExtractFieldInfo *fieldsInfoVct, _nbExtractFieldInfo *AllFieldsInfo, int num_entry, unsigned char* dataInfo, nbNetPFLCompiler *compiler):
FieldDescriptorsVector(ndescriptorVct), ExtractedFieldsDescriptorInfo(fieldsInfoVct), ExtractedAllFieldsInfo(AllFieldsInfo), numExtractedFields(num_entry), DataInfo(dataInfo), m_Compiler(compiler), FieldVector(num_entry, 0)
{
string FieldName;

	for (int i=0; i < num_entry; i++)
	{
		FieldName= string(ExtractedFieldsDescriptorInfo[i].Proto)+"."+ string(ExtractedFieldsDescriptorInfo[i].Name);
		FieldNamesTable[FieldName]= i;
		ProtocolFieldNames.push_back(FieldName);
		FieldName.erase(0, FieldName.length());


		FieldVector[i] = 1 << ExtractedFieldsDescriptorInfo[i].DataFormatType;

		if (ExtractedFieldsDescriptorInfo[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO)
			FieldVector[i] = FieldVector[i] | nbNETPFLCOMPILER_MAX_PROTO_INSTANCES << 12;

		else if(ExtractedFieldsDescriptorInfo[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
			FieldVector[i] = FieldVector[i] | nbNETPFLCOMPILER_MAX_FIELD_INSTANCES << 8;
	}
}


nbeeFieldReader::~nbeeFieldReader()
{
	//delete FieldDescriptorsVector;
}


_nbExtractedFieldsDescriptor* nbeeFieldReader::GetField(string FieldName)
{
int Index;

	Index= FieldNamesTable.count(FieldName);

	if (Index == 1)
		return &(FieldDescriptorsVector[FieldNamesTable[FieldName]]);

	return NULL;
}


_nbExtractedFieldsDescriptor* nbeeFieldReader::GetField(int Index)
{
	if ((Index >= 0) && (Index < numExtractedFields))
		return &(FieldDescriptorsVector[Index]);

	return NULL;
}


_nbExtractedFieldsDescriptor* nbeeFieldReader::GetFields()
{
	return FieldDescriptorsVector;
}

int nbeeFieldReader::getNumFields()
{
	return numExtractedFields;
}

_nbExtractFieldInfo* nbeeFieldReader::GetFieldsInfo()
{
	return ExtractedFieldsDescriptorInfo;
}

_nbExtractFieldInfo* nbeeFieldReader::GetAllFieldsInfo()
{
	return ExtractedAllFieldsInfo;
}

void nbeeFieldReader::FillDescriptors()
{
	int index = 0, n = 0;
	for(int i=0; i < numExtractedFields; i++)
	{
		if(ExtractedFieldsDescriptorInfo[i].FieldType != PDL_FIELD_TYPE_ALLFIELDS
			&& ExtractedFieldsDescriptorInfo[i].FieldType != PDL_FIELD_TYPE_BIT)
		{
			if(FieldDescriptorsVector[i].num_entries == 0)
			{
				FieldDescriptorsVector[i].Offset = *(uint16_t *) &DataInfo[index];
				index += 2;
				FieldDescriptorsVector[i].Length = *(uint16_t *) &DataInfo[index];
				index += 2;
				if(FieldDescriptorsVector[i].Length > 0)
					FieldDescriptorsVector[i].Valid= true;
				else
					FieldDescriptorsVector[i].Valid= false;
			}else
			{
				FieldDescriptorsVector[i].Valid= true;
				n = *(uint32_t *) &DataInfo[index];
				index += 4;
				if(n > 0 && FieldDescriptorsVector[i].DVct != NULL)
				{
					int max_instance;
					if(ExtractedFieldsDescriptorInfo[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
						max_instance = nbNETPFLCOMPILER_MAX_FIELD_INSTANCES;
					else
						max_instance = nbNETPFLCOMPILER_MAX_PROTO_INSTANCES;
					FieldDescriptorsVector[i].num_entries = n;
					for(int j = 0; j < n; j++)
					{
						FieldDescriptorsVector[i].DVct[j].Offset = *(uint16_t *) &DataInfo[index];
						index += 2;
						FieldDescriptorsVector[i].DVct[j].Length = *(uint16_t *) &DataInfo[index];
						index += 2;
						if(FieldDescriptorsVector[i].DVct[j].Length > 0)
							FieldDescriptorsVector[i].DVct[j].Valid= true;
						else
							FieldDescriptorsVector[i].DVct[j].Valid= false;
					}
					index += 4*(max_instance - n);
				}else
				{
					FieldDescriptorsVector[i].Valid= false;
					int max_instance;
					if(ExtractedFieldsDescriptorInfo[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
						max_instance = nbNETPFLCOMPILER_MAX_FIELD_INSTANCES;
					else
						max_instance = nbNETPFLCOMPILER_MAX_PROTO_INSTANCES;
					index += 4*max_instance;
				}
			}
		}else if(ExtractedFieldsDescriptorInfo[i].FieldType == PDL_FIELD_TYPE_BIT)
		{
			if(FieldDescriptorsVector[i].num_entries == 0)
			{
				FieldDescriptorsVector[i].BitField_Value = (*(uint32_t *) &DataInfo[index]) - 0x80000000;
				if(((*(uint32_t *) &DataInfo[index]) & 0x80000000) == 0x80000000)
					FieldDescriptorsVector[i].Valid= true;
				else
					FieldDescriptorsVector[i].Valid= false;
				index += 4;
			}else
			{
				FieldDescriptorsVector[i].Valid= true;
				n = *(uint32_t *) &DataInfo[index];
				index += 4;
				if(n > 0 && FieldDescriptorsVector[i].DVct != NULL)
				{
					int max_instance;
					if(ExtractedFieldsDescriptorInfo[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
						max_instance = nbNETPFLCOMPILER_MAX_FIELD_INSTANCES;
					else
						max_instance = nbNETPFLCOMPILER_MAX_PROTO_INSTANCES;
					FieldDescriptorsVector[i].num_entries = n;
					for(int j = 0; j < n; j++)
					{
						FieldDescriptorsVector[i].DVct[j].BitField_Value = (*(uint32_t *) &DataInfo[index]) - 0x80000000;
						if(((*(uint32_t *) &DataInfo[index]) & 0x80000000) == 0x80000000)
							FieldDescriptorsVector[i].DVct[j].Valid= true;
						else
							FieldDescriptorsVector[i].DVct[j].Valid= false;
						index += 4;
					}
					index += 4*(max_instance - n);
				}else
				{
					FieldDescriptorsVector[i].Valid= false;
					int max_instance;
					if(ExtractedFieldsDescriptorInfo[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
						max_instance = nbNETPFLCOMPILER_MAX_FIELD_INSTANCES;
					else
						max_instance = nbNETPFLCOMPILER_MAX_PROTO_INSTANCES;
					index += 4*max_instance;
				}
			}
		}else
		{
			FieldDescriptorsVector[i].Valid= true;
			n = *(uint16_t *) &DataInfo[index];
			index += 2;
			if(n > 0 && FieldDescriptorsVector[i].DVct != NULL && i == numExtractedFields - 1)
			{
				FieldDescriptorsVector[i].num_entries = n;
				for(int j = 0; j < n; j++)
				{
					int id = *(uint16_t *) &DataInfo[index];
					index += 2;
					int res = m_Compiler->GetFieldInfo(ExtractedFieldsDescriptorInfo[i].Proto, id, &ExtractedAllFieldsInfo[j]);
					if(res == nbSUCCESS)
					{
						if(ExtractedAllFieldsInfo[j].FieldType == PDL_FIELD_TYPE_BIT)
						{
							FieldDescriptorsVector[i].DVct[j].BitField_Value = (*(uint32_t *) &DataInfo[index]) - 0x80000000;
							if(((*(uint32_t *) &DataInfo[index]) & 0x80000000) == 0x80000000)
								FieldDescriptorsVector[i].DVct[j].Valid= true;
							else
								FieldDescriptorsVector[i].DVct[j].Valid= false;
							index += 4;
						}else
						{
							FieldDescriptorsVector[i].DVct[j].Offset = *(uint16_t *) &DataInfo[index];
							index += 2;
							FieldDescriptorsVector[i].DVct[j].Length = *(uint16_t *) &DataInfo[index];
							index += 2;
							if(FieldDescriptorsVector[i].DVct[j].Length > 0)
								FieldDescriptorsVector[i].DVct[j].Valid= true;
							else
								FieldDescriptorsVector[i].DVct[j].Valid= false;
						}
					}else
					{
						FieldDescriptorsVector[i].DVct[j].Valid= false;
						index += 4;
					}
				}
			}else
			{
				FieldDescriptorsVector[i].Valid= false;
				for(int j = i+1; j < numExtractedFields; j++)
					FieldDescriptorsVector[j].Valid= false;
				break;
			}
		}
	}
}


_nbExtractedFieldsNameList nbeeFieldReader::GetFieldNames()
{
		return ProtocolFieldNames;
}


int nbeeFieldReader::IsValid(_nbExtractedFieldsDescriptor *FieldDescriptor)
{
	if (FieldDescriptor->Valid)
			return nbSUCCESS;

	return nbFAILURE;
}


int nbeeFieldReader::IsComplete()
{
	for (int i=0; i < numExtractedFields; i++)
	{
	 if (IsValid(&FieldDescriptorsVector[i]) == nbFAILURE)
		 return nbFAILURE;
	}
	return nbSUCCESS;
}


vector<uint16_t> nbeeFieldReader::GetFieldVector()
{
	return FieldVector;
}

