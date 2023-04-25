/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include <nbee_extractedfieldreader.h>
#include <string>
#include <list>
#include <map>
#include <nbee.h>
#include <vector>

using namespace std;

/*!
	\brief Defines a map that keeps the association between field names ('proto.name') and the internal index of that field.

	It is used o allow the user to ask info about a field in the form 'proto.name', which is then translated into an
	index of that field in our internal structures.
*/
typedef map<string,int> _FieldNamesTable;



class nbeeFieldReader :public nbExtractedFieldsReader
{

friend class nbeePacketEngine;

private:
	_nbExtractedFieldsDescriptor *FieldDescriptorsVector;
	_FieldNamesTable FieldNamesTable;			//!< Keeps the mapping between protocol fields (in the form 'proto.name') and the internal index associated to that field.
	_nbExtractedFieldsNameList ProtocolFieldNames;		//!< Keeps the list of extracted fields as strings, in the form 'proto.name'.
	_nbExtractFieldInfo *ExtractedFieldsDescriptorInfo;
	_nbExtractFieldInfo *ExtractedAllFieldsInfo;
	int numExtractedFields;
	unsigned char * DataInfo;
	
	nbNetPFLCompiler *m_Compiler;
	vector<uint16_t> FieldVector;

protected:

	void FillDescriptors();

public:
	/*!
		\brief	Object constructor

		\param	fieldsVct		Reference to the _nbExtractedFieldsDescriptorVector structure
		\param	dataInfo		Reference to the Info Partition
		\param	compiler		Reference to the nbpflCompiler.

	*/
	nbeeFieldReader(_nbExtractedFieldsDescriptor *fieldsVct, _nbExtractFieldInfo *fieldsInfoVct, _nbExtractFieldInfo *AllFieldsInfo, int num_entry, unsigned char *dataInfo, nbNetPFLCompiler *compiler);

	/*!
		\brief	Object destructor
	*/
	~nbeeFieldReader(void);

	_nbExtractedFieldsDescriptor* GetField(string FieldName);

	_nbExtractedFieldsDescriptor* GetField(int Index);

	_nbExtractedFieldsDescriptor* GetFields();

	_nbExtractedFieldsNameList GetFieldNames();
	
	_nbExtractFieldInfo* GetFieldsInfo();
	
	_nbExtractFieldInfo* GetAllFieldsInfo();
	
	int getNumFields();

	int IsValid(_nbExtractedFieldsDescriptor *FieldDescriptor);

	int IsComplete();

	vector<uint16_t> GetFieldVector();
};

