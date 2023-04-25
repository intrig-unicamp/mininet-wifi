/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "tree.h"
#include "symbols.h"
#include "dump.h"
#include "mironode.h"
#include <iomanip>
#include <list>
#include <string.h>



#define MAX_SYM_STR_SZ 256
#define TREE_NODE	"node%u_%u"
#define STMT_NODE	"stmt%u"
#define TAB_CHAR	"  "
/*

int dumpstr(char* Buffer, int BufSize, char *Format, ...)
{
int WrittenBytes;
va_list Args;
va_start(Args, Format);

if (Buffer == NULL)
return -1;

WrittenBytes= vsnprintf(Buffer, BufSize - 1, Format, Args);

if (BufSize >= 1)
Buffer[BufSize - 1] = 0;
else
Buffer[0]= 0;

return WrittenBytes;
};
*/


void CodeWriter::DumpSymbol(Symbol *sym)
{
	nbASSERT(sym!=NULL,"DumpSymbol: Symbol cannot be NULL!");	

	switch (sym->SymKind)
	{
	case SYM_PROTOCOL:
		m_Stream << ((SymbolProto*)sym)->Name;
		break;
	case SYM_FIELD:
		m_Stream << ((SymbolField*)sym)->Name;
		break;
	case SYM_RT_VAR:
		m_Stream << ((SymbolVariable*)sym)->Name;
		break;
	case SYM_TEMP:
		if (GenNetIL)
			m_Stream << ((SymbolTemp*)sym)->LocalReg.get_model()->get_name();
		else
			m_Stream << ((SymbolTemp*)sym)->Name << "[" << ((SymbolTemp*)sym)->LocalReg << "]";
		break;
	case SYM_LABEL:
	{	
		m_Stream << ((SymbolLabel*)sym)->Name;
		}break;
	case SYM_INT_CONST:
		m_Stream << ((SymbolIntConst*)sym)->Value;
		break;
	case SYM_STR_CONST:
		m_Stream << "\"" << ((SymbolStrConst*)sym)->Name << "\"";
		break;
	case SYM_RT_LOOKUP_ENTRY:
		{
			SymbolLookupTableEntry *entry=(SymbolLookupTableEntry *)sym;
			switch (entry->Validity)
			{
			case LOOKUPTABLE_ENTRY_VALIDITY_ADDONHIT:
				m_Stream << "AddOnHit";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_KEEPFOREVER:
				m_Stream << "KeepForever";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_KEEPMAXTIME:
				m_Stream << "KeepMaxTime";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_REPLACEONHIT:
				m_Stream << "ReplaceOnHit";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_UPDATEONHIT:
				m_Stream << "UpdateOnHit";
				break;
			default:
				break;
			}
		}break;
	case SYM_RT_LOOKUP_KEYS:
		{
			SymbolLookupTableKeysList *entry=(SymbolLookupTableKeysList *)sym;

			m_Stream << "<";
			NodesList_t::iterator i=entry->Values.begin();
			while ( i!=entry->Values.end() )
			{
				if (i!=entry->Values.begin())
					m_Stream << ", ";
				if (*i == NULL)
					m_Stream << " ERROR ";
				else
					DumpTree(*i);
				i++;
			}
			m_Stream << ">";
		}break;
	case SYM_RT_LOOKUP_VALUES:
		{
			SymbolLookupTableValuesList *entry=(SymbolLookupTableValuesList *)sym;

			m_Stream << "<";
			NodesList_t::iterator i=entry->Values.begin();
			while ( i!=entry->Values.end() )
			{
				if (i!=entry->Values.begin())
					m_Stream << ", ";
				if (*i == NULL)
					m_Stream << " ERROR ";
				else
					DumpTree(*i);
				i++;
			}
			m_Stream << ">";
		}break;
	case SYM_RT_LOOKUP_ITEM:
		{
			SymbolLookupTableItem *item=(SymbolLookupTableItem *)sym;
			m_Stream << item->Table->Name << "." << item->Name;
		}break;
	case SYM_RT_LOOKUP: //[icerrato]
		{
			SymbolLookupTable *lookuptable=(SymbolLookupTable *)sym;
			m_Stream << lookuptable->Name << ", " << lookuptable->MaxExactEntriesNr << ", " << lookuptable->MaxMaskedEntriesNr;
			if(lookuptable->Validity == TABLE_VALIDITY_STATIC)
				m_Stream << ", static";
			else if(lookuptable->Validity == TABLE_VALIDITY_DYNAMIC)
				m_Stream << ", dynamic";
			else
				nbASSERT(false, "validy of a lookup table must be TABLE_VALIDITY_STATIC or TABLE_VALIDITY_DYNAMIC");		
		}break;
	default:
		nbASSERT(false, "DUMP SYMBOL: CANNOT BE HERE");
		break;
	}
}


void CodeWriter::DumpOpcode(uint16 opcode)
{
	if (opcode == IR_LABEL)
		return;

	if (opcode > mir_first_op)
	{	
		m_Stream << NvmOps[opcode - mir_first_op -1].Name; 
		return;
	}

	m_Stream << OpDescriptions[GET_OP(opcode)].Name;
	if (GET_OP_TYPE(opcode) > 0)
		m_Stream << "." << IRTypeNames[GET_OP_TYPE(opcode)];
}


void CodeWriter::DumpTabLevel(uint32 level)
{
	if (GenNetIL)
	{
		m_Stream << TAB_CHAR;
	}
	else
	{
		uint32 i = 0;
		for (i = 0; i < level; i++)
			m_Stream << TAB_CHAR;
	}
}


void CodeWriter::DumpBitField(SymbolFieldBitField *bitField, uint32 size)
{
	nbASSERT((size == 1)||(size == 2)||(size == 4), "size can be only 1, 2, or 4!");
	m_Stream << "BITFIELD(" << bitField->Name;
	m_Stream << hex << "[mask: 0x" << setw(size)<< bitField->Mask << "]" << dec;
}


void CodeWriter::DumpInnerFields(SymbolFieldContainer *parent, uint32 level)
{
	if (!parent->InnerFields.empty())
	{
		// parse inner fields
		m_Stream << endLine;
		FieldsList_t::iterator i;
		for (i = parent->InnerFields.begin(); i != parent->InnerFields.end(); i++)
		{
			//nbASSERT( ((SymbolField *)*i)->FieldType==PDL_FIELD_BITFIELD, "A child field cannot be different from a bit field");

			DumpTabLevel(level + 1);
			SymbolFieldContainer *childField = (SymbolFieldContainer*) *i;
			SymbolFieldContainer *container=parent;


			switch (childField->FieldType)
			{
			case PDL_FIELD_BITFIELD:
				{
					SymbolFieldBitField *bitField = (SymbolFieldBitField *)childField;
					uint32 mask=bitField->Mask;
					// search the fixed field root to get the size
					while (container->FieldType!=PDL_FIELD_FIXED)
					{
						nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

						// and the mask with the parent mask
						mask=mask & ((SymbolFieldBitField *)container)->Mask;

						container=((SymbolFieldBitField *)container)->DependsOn;
					}
					SymbolFieldFixed *fixed=(SymbolFieldFixed *)container;
					nbASSERT((fixed->Size == 1)||(fixed->Size == 2)||(fixed->Size == 4), "size can be only 1, 2, or 4!");

					m_Stream << "DEFFLD( ";
					m_Stream << bitField->Name << "{";
					m_Stream << "type: bit, ";
					m_Stream << hex << "mask: 0x" << setw(fixed->Size)<< mask << dec;
				}break;
			case PDL_FIELD_FIXED:
				{
					// a fixed field is defined by a statement... it will be dumped there...
					//SymbolFieldFixed *field = (SymbolFieldFixed *)childField;
					//m_Stream << field->Name << "{";
					//SymbolFieldFixed *fixed = (SymbolFieldFixed*)field;
					//m_Stream << "type: fixed, size: " << fixed->Size;
				}break;
			default:
				nbASSERT(false, "An inner field should be either a bit field or a fixed field.");
				break;
			}

			this->DumpInnerFields(childField, level+1);

			if (i != parent->InnerFields.end())
			{
				m_Stream << "} )";
				m_Stream << endLine;
			}
		}
		DumpTabLevel(level);
	}
}

void CodeWriter::DumpFieldDetails(Node *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(GET_OP(node->Op) == OP_FIELD, "node must be a OP_FIELD");
	nbASSERT(node->Sym->SymKind == SYM_FIELD, "terminal node symbol must be a SYM_FIELD");

	SymbolField *field = (SymbolField*)node->Sym;
	m_Stream << field->Name << "{";
	
	switch(field->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixed = (SymbolFieldFixed*)field;
			m_Stream << "type: fixed, size: " << fixed->Size;

			this->DumpInnerFields(fixed, level);

			//if (!fixed->InnerFields.empty())
			//{
			//	// parse inner fields
			//	m_Stream << endLine;
			//	FieldsList_t::iterator i;
			//	for (i = fixed->InnerFields.begin(); i != fixed->InnerFields.end(); i++)
			//	{
			//		DumpTabLevel(level + 1);
			//		SymbolFieldBitField *bitField = (SymbolFieldBitField*) *i;
			//		nbASSERT((fixed->Size == 1)||(fixed->Size == 2)||(fixed->Size == 4), "size can be only 1, 2, or 4!");
			//		m_Stream << "BITFIELD(" << bitField->Name;
			//		m_Stream << hex << "[mask: 0x" << setw(fixed->Size)<< bitField->Mask << "]" << dec;
			//		if (i != fixed->InnerFields.end())
			//			m_Stream << endLine;
			//	}
			//	DumpTabLevel(level);
			//}
		}break;

	case PDL_FIELD_VARLEN:
		{
			SymbolFieldVarLen *varLen = (SymbolFieldVarLen*)field;
			// [ds] added check to going on with dump...
			if (varLen!=NULL && varLen->LenExpr!=NULL)
			{
				m_Stream << "type: varlen, length: ";
				DumpTree(varLen->LenExpr, level);
			}
		}break;

	case PDL_FIELD_PADDING:
		{
			SymbolFieldPadding *pad = (SymbolFieldPadding*)field;
			m_Stream << "type: padding, alignment: " << pad->Align;
		}break;

	case PDL_FIELD_TOKEND:
		{
			SymbolFieldTokEnd *tokEnded = (SymbolFieldTokEnd*)field;

			if (tokEnded!=NULL)
			{
			 if(tokEnded->EndTok!=NULL)
				m_Stream << "type: tokended, token: "<< tokEnded->EndTok;
			 else if(tokEnded->EndRegEx!=NULL)
				m_Stream << "type: tokended, regEx: "<< tokEnded->EndRegEx;
			}
		}break;


	case PDL_FIELD_TOKWRAP:
		{
			SymbolFieldTokWrap *tokWrap = (SymbolFieldTokWrap*)field;

			if (tokWrap!=NULL)
			{
			 if(tokWrap->BeginTok!=NULL)
				m_Stream << "type: tokwrap, begin_token: "<< tokWrap->BeginTok;
			 else if(tokWrap->BeginRegEx!=NULL)
				m_Stream << "type: tokwrap, begin_regEx: "<< tokWrap->BeginRegEx;

			 if(tokWrap->EndTok!=NULL)
				m_Stream << "type: tokwrap, end_token: "<< tokWrap->EndTok;
			 else if(tokWrap->EndRegEx!=NULL)
				m_Stream << "type: tokwrap, end_regEx: "<< tokWrap->EndRegEx;
			}
		}break;


	case PDL_FIELD_LINE:
		{
			SymbolFieldLine *line = (SymbolFieldLine*)field;

				m_Stream << "type: line, line: " << line;

		}break;

	case PDL_FIELD_PATTERN:
		{
			SymbolFieldPattern *pattern = (SymbolFieldPattern*)field;

			if (pattern!=NULL && pattern->Pattern!=NULL)
			{
				m_Stream << "type: pattern, pattern: "<< pattern->Pattern;
			}
		}break;

	case PDL_FIELD_EATALL:
		{
			SymbolFieldEatAll *eatall = (SymbolFieldEatAll*)field;

				m_Stream << "type: eatall, eatall:" << eatall;

		}break;

	default:
		break;
	}
	m_Stream << "}";
}

//TODO: [icerrato]
void CodeWriter::DumpFieldDetails(MIRONode *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(GET_OP(node->OpCode) == OP_FIELD, "node must be a OP_FIELD");
	nbASSERT(node->Sym->SymKind == SYM_FIELD, "terminal node symbol must be a SYM_FIELD");
	
	SymbolField *field = (SymbolField*)node->Sym;
	m_Stream << field->Name << "{";
	
	switch(field->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixed = (SymbolFieldFixed*)field;
			m_Stream << "type: fixed, size: " << fixed->Size;

			this->DumpInnerFields(fixed, level);

			//if (!fixed->InnerFields.empty())
			//{
			//	// parse inner fields
			//	m_Stream << endLine;
			//	FieldsList_t::iterator i;
			//	for (i = fixed->InnerFields.begin(); i != fixed->InnerFields.end(); i++)
			//	{
			//		DumpTabLevel(level + 1);
			//		SymbolFieldBitField *bitField = (SymbolFieldBitField*) *i;
			//		nbASSERT((fixed->Size == 1)||(fixed->Size == 2)||(fixed->Size == 4), "size can be only 1, 2, or 4!");
			//		m_Stream << "BITFIELD(" << bitField->Name;
			//		m_Stream << hex << "[mask: 0x" << setw(fixed->Size)<< bitField->Mask << "]" << dec;
			//		if (i != fixed->InnerFields.end())
			//			m_Stream << endLine;
			//	}
			//	DumpTabLevel(level);
			//}
		}break;

	case PDL_FIELD_VARLEN:
		{
			SymbolFieldVarLen *varLen = (SymbolFieldVarLen*)field;
			// [ds] added check to going on with dump...
			if (varLen!=NULL && varLen->LenExpr!=NULL)
			{
				m_Stream << "type: varlen, length: ";
				DumpTree(varLen->LenExpr, level);
			}
		}break;

	case PDL_FIELD_PADDING:
		{
			SymbolFieldPadding *pad = (SymbolFieldPadding*)field;
			m_Stream << "type: padding, alignment: " << pad->Align;
		}break;

	case PDL_FIELD_TOKEND:
		{
			SymbolFieldTokEnd *tokEnded = (SymbolFieldTokEnd*)field;

			if (tokEnded!=NULL)
			{
			 if(tokEnded->EndTok!=NULL)
				m_Stream << "type: tokended, token: "<< tokEnded->EndTok;
			 else if(tokEnded->EndRegEx!=NULL)
				m_Stream << "type: tokended, regEx: "<< tokEnded->EndRegEx;
			}			
		}break;


	case PDL_FIELD_TOKWRAP:
		{
			SymbolFieldTokWrap *tokWrap = (SymbolFieldTokWrap*)field;

			if (tokWrap!=NULL)
			{
			 if(tokWrap->BeginTok!=NULL)
				m_Stream << "type: tokwrap, begin_token: "<< tokWrap->BeginTok;
			 else if(tokWrap->BeginRegEx!=NULL)
				m_Stream << "type: tokwrap, begin_regEx: "<< tokWrap->BeginRegEx;

			 if(tokWrap->EndTok!=NULL)
				m_Stream << "type: tokwrap, end_token: "<< tokWrap->EndTok;
			 else if(tokWrap->EndRegEx!=NULL)
				m_Stream << "type: tokwrap, end_regEx: "<< tokWrap->EndRegEx;
			}
		}break;


	case PDL_FIELD_LINE:
		{
			SymbolFieldLine *line = (SymbolFieldLine*)field;

				m_Stream << "type: line, line: " << line;

		}break;

	default:
		break;
	}
	m_Stream << "}";
}

void CodeWriter::DumpVarDetails(Node *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(GET_OP(node->Op) == OP_VAR, "node must be a OP_VAR");
	nbASSERT((node->Sym->SymKind == SYM_RT_VAR) || (node->Sym->SymKind == SYM_TEMP) || (node->Sym->SymKind == SYM_RT_LOOKUP_ITEM), "terminal node symbol must be a SYM_RTVAR, a SYM_TEMP or a SYM_RT_LOOKUP_ITEM");//[icerrato]

	SymbolVariable *var = (SymbolVariable*)node->Sym;
	m_Stream << var->Name;
}

//TODO: [icerrato]
//! Methos to dump Vardetails for MIRONode
void CodeWriter::DumpVarDetails(MIRONode *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(GET_OP(node->OpCode) == OP_VAR, "node must be a OP_VAR");
	nbASSERT((node->Sym->SymKind == SYM_RT_VAR) || (node->Sym->SymKind == SYM_TEMP) || (node->Sym->SymKind == SYM_RT_LOOKUP_ITEM), "terminal node symbol must be a SYM_RTVAR, a SYM_TEMP or a SYM_RT_LOOKUP_ITEM");//[icerrato]

	SymbolVariable *var = (SymbolVariable*)node->Sym;
	m_Stream << var->Name;
}

void CodeWriter::DumpTree(Node *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	//DumpTabLevel(level);
	if (GenNetIL)
	{
		DumpTreeNetIL(node, level);
		return;
	}
	if (node->Op > mir_first_op)
	{
		DumpOpcode(node->Op);
		m_Stream << "( ";

		switch(node->Op)
		{
		case LOCLD:
			DumpSymbol((SymbolTemp*)node->Sym);
			break;
		case LOCST:
			DumpSymbol((SymbolTemp*)node->Sym);
			m_Stream << ", ";
			DumpTree(node->Kids[0], level);
			break;
		case PUSH:
			m_Stream << node->Value;
			break;
		case SNDPKT:
			m_Stream << node->Value;
		default:
			if (node->Kids[0])
			{
				DumpTree(node->Kids[0], level);
			}
			if (node->Kids[1])
			{
				m_Stream << ", ";
				DumpTree(node->Kids[1], level);
			}
			if (node->IsTerOp() && node->Kids[2])
			{
				m_Stream << ", ";
				DumpTree(node->Kids[2], level);
			}
			break;
		}
		m_Stream << " )";
		return;
	}
	if (node->IsTerm())
	{
		nbASSERT(node->Sym != NULL, "terminal node must contain a valid symbol");
		DumpSymbol(node->Sym);
		if (node->Op == IR_SVAR)
		{
			if (node->Kids[0])
			{
				nbASSERT(node->Kids[1], "invalid range specification");
				m_Stream << "[";
				DumpTree(node->Kids[0], level);
				m_Stream << ":";
				DumpTree(node->Kids[1], level);
				m_Stream << "]";
			}
		}
		return;
	}

	if (GET_OP(node->Op) == OP_LABEL)
	{
		DumpSymbol(node->Sym);
		return;
	}


	DumpOpcode(node->Op);
	m_Stream << "( ";
	switch(node->Op)
	{
	case IR_DEFFLD:
		DumpFieldDetails(node->Kids[0], level);
		break;
	case IR_DEFVARS:
	case IR_DEFVARI:
		DumpVarDetails(node->Kids[0], level);
		if (node->Kids[1])
		{
			m_Stream << " = ";
			DumpTree(node->Kids[1], level);
		}
		break;
	case IR_LKADD:
		m_Stream << ((SymbolLookupTableEntry *)node->Sym)->Table->Name;
		m_Stream << ", ";
		DumpTree(node->Kids[0], level);
		m_Stream << ", ";
		DumpTree(node->Kids[1], level);
		m_Stream << ", ";
		DumpSymbol(node->Sym);
		break;
	case IR_LKDEL:
		m_Stream << ((SymbolLookupTable *)node->Sym)->Name;
		break;
	case IR_LKHIT:
	case IR_LKSEL:
		m_Stream << ((SymbolLookupTableEntry *)node->Sym)->Table->Name;
		m_Stream << ", ";
		DumpTree(node->Kids[0], level);
		break;
	case IR_REGEXFND:
		{
			nbASSERT(node->Sym != NULL, "regexp statement must contain a valid symbol");
			SymbolRegEx *genericRegexp=(SymbolRegEx *)node->Sym;
			nbASSERT(genericRegexp->MyType()==NETPDL,"Only netpdl regexp can be dumped!");
			SymbolRegExPDL *regexp=(SymbolRegExPDL *)genericRegexp;
			
			DumpTree(regexp->Offset);
			m_Stream << ", '" << regexp->Pattern->Name << "', ";
			if (regexp->CaseSensitive)
				m_Stream << "ci";
			else
				m_Stream << "cs";
		}break;
	case IR_REGEXXTR:
		{
			nbASSERT(node->Sym != NULL, "regexp statement must contain a valid symbol");
			SymbolRegEx *genericRegexp=(SymbolRegEx *)node->Sym;
			nbASSERT(genericRegexp->MyType()==NETPDL,"Only netpdl regexp can be dumped!");
			SymbolRegExPDL *regexp=(SymbolRegExPDL *)genericRegexp;
			
			DumpTree(regexp->Offset);
			m_Stream << ", '" << regexp->Pattern->Name << "', ";
			if (regexp->CaseSensitive)
				m_Stream << "ci, ";
			else
				m_Stream << "cs, ";
			if(regexp->MatchToReturn == 0)
				m_Stream << "complete, ";
			else
				m_Stream << "partial, ";
			nbASSERT(regexp->MatchedSize != NULL,"There is a bug!");
			m_Stream << regexp->MatchedSize->Name;
		}break;
	case IR_LKUPDS: case IR_LKUPDI:
	case IR_ASGNS:
	case IR_ASGNI:
	default:
		if (node->Kids[0])
			DumpTree(node->Kids[0], level);

		if (node->Kids[1])
		{
			m_Stream << ", ";
			DumpTree(node->Kids[1], level);
		}
		break;
	}
	m_Stream << " )";
}

//TODO: [icerrato]
void CodeWriter::DumpTree(MIRONode *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	//DumpTabLevel(level);
	if (GenNetIL)
	{
		DumpTreeNetIL(node, level);
		return;
	}
	if (node->OpCode > mir_first_op)
	{
		DumpOpcode(node->OpCode);
		if(node->getDefReg() != MIRONode::RegType::invalid)
			m_Stream << "[" << node->getDefReg() << "] ";
		m_Stream << "( ";

		switch(node->OpCode)
		{
		case LOCLD:
			DumpSymbol((SymbolTemp*)node->Sym);
			break;
		case LOCST:
			DumpSymbol((SymbolTemp*)node->Sym);
			m_Stream << ", ";
			DumpTree(node->kids[0], level);
			break;
		case PUSH:
			m_Stream << node->Value;
			break;
		case SNDPKT:
			m_Stream << node->Value;
		default:
			if (node->kids[0])
			{
				DumpTree(node->kids[0], level);
			}
			if (node->kids[1])
			{
				m_Stream << ", ";
				DumpTree(node->kids[1], level);
			}
			if (node->IsTerOp() && node->kids[2])
			{
				m_Stream << ", ";
				DumpTree(node->kids[2], level);
			}
			break;
		}
		m_Stream << " )";
		return;
	}
	if (node->IsTerm())
	{
		nbASSERT(node->Sym != NULL, "terminal node must contain a valid symbol");
		DumpSymbol(node->Sym);
		if (node->OpCode == IR_SVAR)
		{
			if (node->kids[0])
			{
				nbASSERT(node->kids[1], "invalid range specification");
				m_Stream << "[";
				DumpTree(node->kids[0], level);
				m_Stream << ":";
				DumpTree(node->kids[1], level);
				m_Stream << "]";
			}
		}
		return;
	}

	if (GET_OP(node->OpCode) == OP_LABEL)
	{
		DumpSymbol(node->Sym);
		return;
	}


	DumpOpcode(node->OpCode);
	m_Stream << "( ";
	switch(node->OpCode)
	{
	case IR_DEFFLD:	
		DumpFieldDetails(node->kids[0], level);
		break;
	case IR_DEFVARS:
	case IR_DEFVARI:
		DumpVarDetails(node->kids[0], level);
		if (node->kids[1])
		{
			m_Stream << " = ";
			DumpTree(node->kids[1], level);
		}
		break;
	case IR_LKADD:
		m_Stream << ((SymbolLookupTableEntry *)node->Sym)->Table->Name;
		m_Stream << ", ";
		DumpTree(node->kids[0], level);
		m_Stream << ", ";
		DumpTree(node->kids[1], level);
		m_Stream << ", ";
		DumpSymbol(node->Sym);
		break;
	case IR_LKDEL:
		m_Stream << ((SymbolLookupTable *)node->Sym)->Name;
		break;
	case IR_LKHIT:
	case IR_LKSEL:
		m_Stream << ((SymbolLookupTableEntry *)node->Sym)->Table->Name;
		m_Stream << ", ";
		DumpTree(node->kids[0], level);
		break;
	case IR_LKUPDS: case IR_LKUPDI:
	case IR_ASGNS:
	case IR_ASGNI:
	default:
		if (node->kids[0])
			DumpTree(node->kids[0], level);

		if (node->kids[1])
		{
			m_Stream << ", ";
			DumpTree(node->kids[1], level);
		}
		break;
	}
	m_Stream << " )";
}

void CodeWriter::DumpTreeNetIL(Node *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");

	//nbASSERT(node->Op > mir_first_op, "the opcode is not a NetIL operator");
	if (node->Kids[0])
		DumpTree(node->Kids[0], level);

	if (node->Kids[1])
		DumpTree(node->Kids[1], level);

	if (node->IsTerOp() && node->Kids[2])
		DumpTree(node->Kids[2], level);

	if (node->Op == IR_LABEL)
		return;

	if (node->Op == IR_DATA)
	{
		SymbolDataItem *data=(SymbolDataItem *)node->Sym;
		char type[3];
		if (data->Type==DATA_TYPE_BYTE)
			strncpy(type, "db", 3);
		else
			strncpy(type, "dw", 3);

		m_Stream << "  " << data->Name << "  " << type << "  " << data->Value;

		return;
	}

	DumpTabLevel(level);
	DumpOpcode(node->Op);
	switch(node->Op)
	{
	case LOCLD:
		m_Stream << " ";
		DumpSymbol((SymbolTemp*)node->Sym);
		m_Stream << TAB_CHAR << "; " << ((SymbolTemp*)node->Sym)->Name;
		break;
	case LOCST:
		m_Stream << " ";
		DumpTree(node->Kids[0], level);
		DumpSymbol((SymbolTemp*)node->Sym);
		m_Stream << TAB_CHAR << "; " << ((SymbolTemp*)node->Sym)->Name;
		break;
	case PUSH:
		m_Stream << " " << node->Value;
		break;
	case SNDPKT:
		m_Stream << " out" << node->Value;
		break;
	case JCMPEQ: case JCMPNEQ: case JCMPG:
	case JCMPGE: case JCMPL: case JCMPLE:
	case JCMPG_S: case JCMPGE_S:
	case JCMPL_S: case JCMPLE_S:
		return;
		break;
	case JFLDEQ: case JFLDNEQ:
	case JFLDGT: case JFLDLT:
		return;
		break;
	case COPIN: case COPOUT:
		nbASSERT(node->Kids[0]!=NULL && node->Kids[0]->Sym!=NULL && node->Kids[0]->Sym->SymKind==SYM_LABEL, "copro.in and copro.out should specify two labels");
		nbASSERT(node->Kids[1]!=NULL && node->Kids[1]->Sym!=NULL && node->Kids[1]->Sym->SymKind==SYM_LABEL, "copro.in and copro.out should specify two labels");
		m_Stream << " " << ((SymbolLabel *)node->Kids[0]->Sym)->Name << ", " << ((SymbolLabel *)node->Kids[1]->Sym)->Name;
		break;
	case COPINIT:
		nbASSERT(node->Kids[0]!=NULL && node->Kids[0]->Sym!=NULL && node->Kids[0]->Sym->SymKind==SYM_LABEL, "copro.init and copro.out should specify two labels");
		nbASSERT(node->Kids[1]!=NULL && node->Kids[1]->Sym!=NULL && node->Kids[1]->Sym->SymKind==SYM_LABEL, "copro.in and copro.out should specify two labels");
		m_Stream << " " << ((SymbolLabel *)node->Kids[0]->Sym)->Name << ", " << ((SymbolLabel *)node->Kids[1]->Sym)->Name;
		break;
	case COPRUN:
		nbASSERT(node->Kids[0]!=NULL && node->Kids[0]->Sym!=NULL && node->Kids[0]->Sym->SymKind==SYM_LABEL, "copro.invoke should specify two labels");
		nbASSERT(node->Kids[1]!=NULL && node->Kids[1]->Sym!=NULL && node->Kids[1]->Sym->SymKind==SYM_LABEL, "copro.invoke should specify two labels");
		m_Stream << " " << ((SymbolLabel *)node->Kids[0]->Sym)->Name << ", " << ((SymbolLabel *)node->Kids[1]->Sym)->Name;
		break;
	}

	m_Stream << endLine;
}

//TODO: [icerrato]
//Code to dump treeNetil from MIRONode
void CodeWriter::DumpTreeNetIL(MIRONode *node, uint32 level)
{
	nbASSERT(node != NULL, "node cannot be NULL");

	//nbASSERT(node->Op > mir_first_op, "the opcode is not a NetIL operator");
	if (node->kids[0])
		DumpTree(node->kids[0], level);

	if (node->kids[1])
		DumpTree(node->kids[1], level);

	if (node->IsTerOp() && node->kids[2])
		DumpTree(node->kids[2], level);

	if (node->getOpcode() == IR_LABEL)
		return;

	DumpTabLevel(level);
	DumpOpcode(node->getOpcode());
	switch(node->getOpcode())
	{
	case LOCLD:
		m_Stream << " ";
		DumpSymbol((SymbolTemp*)node->Sym);
		m_Stream << TAB_CHAR << "; " << ((SymbolTemp*)node->Sym)->Name;
		break;
	case LOCST:
		m_Stream << " ";
		//DumpTree(node->kids[0], level);
		DumpSymbol((SymbolTemp*)node->Sym);
		m_Stream << TAB_CHAR << "; " << ((SymbolTemp*)node->Sym)->Name;
		break;
	case PUSH:
		m_Stream << " " << node->Value;
		break;
	case SNDPKT:
		m_Stream << " out" << node->Value;
		break;
	case JCMPEQ: case JCMPNEQ: case JCMPG:
	case JCMPGE: case JCMPL: case JCMPLE:
	case JCMPG_S: case JCMPGE_S:
	case JCMPL_S: case JCMPLE_S:
		return;
		break;
	case JFLDEQ: case JFLDNEQ:
	case JFLDGT: case JFLDLT:
		return;
		break;
	case COPIN:
		nbASSERT(node->Sym->SymKind==SYM_LABEL, "copro.in and copro.out should specify the copro name as label symbol");
		m_Stream << " " << ((SymbolLabel *)node->Sym)->Name << ", " << node->Value;
		break;
	case COPOUT:
		nbASSERT(node->kids[0]!=NULL, "copro.out should specify a node argument");
		nbASSERT(node->Sym->SymKind==SYM_LABEL, "copro.out should specify the copro name as label symbol");
		nbASSERT(node->Value>=0, "copro.out should specify the register ID as value");
		m_Stream << " " << ((SymbolLabel *)node->Sym)->Name << ", " << node->Value;
		break;
	case COPINIT:
		nbASSERT(node->Sym->SymKind==SYM_LABEL, "copro.init should specify the copro name as label symbol");
		nbASSERT(node->SymEx==NULL || node->SymEx->SymKind==SYM_LABEL, "copro.init can specify an extra label symbol");
		nbASSERT(node->Value>=0, "copro.init can specify a value");
		if (node->SymEx)

			m_Stream << " " << ((SymbolLabel *)node->Sym)->Name << ", " << ((SymbolLabel *)node->SymEx)->Name;
		else
			m_Stream << " " << ((SymbolLabel *)node->Sym)->Name << ", " << node->Value;
		break;
	case COPRUN:
		nbASSERT(node->Sym->SymKind==SYM_LABEL, "copro.run should specify the copro name as label symbol");
		nbASSERT(node->Value>=0, "copro.run should specify the operation ID as value");
		m_Stream << " " << ((SymbolLabel *)node->Sym)->Name << ", " << node->Value;
		break;
	}

	m_Stream << endLine;
}

void CodeWriter::DumpJump(StmtJump *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->TrueBranch != NULL, "TrueBranch cannot be NULL");
	if (GenNetIL)
	{
		DumpJumpNetIL(stmt, level);
		return;
	}
	DumpTabLevel(level);

	if (stmt->FalseBranch != NULL)
	{
		nbASSERT(stmt->Forest != NULL, "Forest cannot be NULL");
		if (stmt->Forest->Op < HIR_LAST_OP)
			m_Stream << "JCOND(";
		DumpTree(stmt->Forest, level);
		if (stmt->Forest->Op < HIR_LAST_OP)
			m_Stream << ") ";
		m_Stream << " t:";
		DumpSymbol(stmt->TrueBranch);
		m_Stream << ", f:";
		DumpSymbol(stmt->FalseBranch);
		return;
	}

	if (stmt->Opcode < HIR_LAST_OP)
		m_Stream << "JUMP";
	else
		DumpOpcode(stmt->Opcode);
	m_Stream << " ";
	DumpSymbol(stmt->TrueBranch);

}

//TODO: [icerrato]
//! method to print a PFLNODE jump
void CodeWriter::DumpJump(JumpMIRONode *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->TrueBranch != NULL, "TrueBranch cannot be NULL");
	if (GenNetIL)
	{
		DumpJumpNetIL(stmt, level);
		return;
	}
	DumpTabLevel(level);

	if (stmt->FalseBranch != NULL)
	{
		nbASSERT(stmt->getKid(0) != NULL, "Forest cannot be NULL");
		if (stmt->getKid(0)->OpCode < HIR_LAST_OP)
			m_Stream << "JCOND(";
		DumpTree(stmt->getKid(0), level);
		if (stmt->getKid(0)->OpCode < HIR_LAST_OP)
			m_Stream << ") ";
		m_Stream << " t:";
		DumpSymbol(stmt->TrueBranch);
		m_Stream << ", f:";
		DumpSymbol(stmt->FalseBranch);
		return;
	}

	if (stmt->getOpcode() < HIR_LAST_OP)
			m_Stream << "JUMP";
	else
		DumpOpcode(stmt->getOpcode());
	m_Stream << " ";
	DumpSymbol(stmt->TrueBranch);


}

void CodeWriter::DumpJumpNetIL(StmtJump *stmt, uint32 level)
{
	if (stmt->FalseBranch == NULL)
	{
		DumpTabLevel(level);
		DumpOpcode(stmt->Opcode);
		m_Stream << " ";
		DumpSymbol(stmt->TrueBranch);
	}
	else
	{
		nbASSERT(stmt->Forest != NULL, "Forest cannot be NULL");
		DumpTreeNetIL(stmt->Forest, level);
		m_Stream << " ";
		DumpSymbol(stmt->TrueBranch);
	}
}

//method to dump netilcode for JumpMIRONode
void CodeWriter::DumpJumpNetIL(JumpMIRONode *stmt, uint32 level)
{
	//std::cerr << "Sto per stampare il jump:" << std::endl;
	//stmt->printNode(std::cerr, true);
	//std::cerr << std::endl;

	if (stmt->FalseBranch == NULL)
	{
		DumpTabLevel(level);
		DumpOpcode(stmt->getOpcode());
		m_Stream << " ";
		DumpSymbol(stmt->TrueBranch);
	}
	else
	{
		nbASSERT(stmt->getKid(0) != NULL, "Forest cannot be NULL");
		DumpTreeNetIL(stmt->getKid(0), level);
		m_Stream << " ";
		DumpSymbol(stmt->TrueBranch);
	}
}

void CodeWriter::DumpCase(StmtCase *stmt, bool isDefault, uint32 level)
{
	DumpTabLevel(level);
	if (isDefault)
	{
		m_Stream << "default: ";

	}
	else
	{
		if (!GenNetIL)
		{
			m_Stream << "case ";
			DumpTree(stmt->Forest, level);
		}
		else
		{
			m_Stream << stmt->Forest->Value;
		}
		/*
		nbASSERT(stmt->Forest != NULL, "Forest should not be NULL");
		nbASSERT(stmt->Forest->Sym != NULL, "symbol should not be NULL");

		*/
		m_Stream << ": ";
	}
	if (stmt->Target)
	{
		nbASSERT(stmt->Code->Code->Empty(), "when target is defined code must be empty");
		m_Stream << stmt->Target->Name;
		return;  //HERE TMP
	}

	m_Stream << endLine;
	DumpCode(stmt->Code->Code, level + 1);

}


//!Code to dump a cse PFLNODE
void CodeWriter::DumpCase(CaseMIRONode *stmt, bool isDefault, uint32 level)
{
	DumpTabLevel(level);
	if (isDefault)
	{
		m_Stream << "default: ";

	}
	else
	{
		if (!GenNetIL)
		{
			m_Stream << "case ";
			DumpTree(stmt->getKid(0), level);
		}
		else
		{
			m_Stream << stmt->getKid(0)->Value;
		}
		/*
		nbASSERT(stmt->Forest != NULL, "Forest should not be NULL");
		nbASSERT(stmt->Forest->Sym != NULL, "symbol should not be NULL");

		*/
		m_Stream << ": ";
	}
	if (stmt->Target)
	{
		//nbASSERT(stmt->Code.Code.Empty(), "when target is defined code must be empty"); //FIXME
		m_Stream << stmt->Target->Name;
		return;
	}

	m_Stream << endLine;
	//DumpCode(&stmt->Code.Code, level + 1); //FIXME

}
void CodeWriter::DumpIf(StmtIf *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->Kind == STMT_IF, "stmt must be a STMT_IF");
	DumpTabLevel(level);
	m_Stream << "if (";
	DumpTree(stmt->Forest, level);
	m_Stream << " )" << endLine;
	DumpTabLevel(level);
	m_Stream << "{" << endLine;
	DumpCode(stmt->TrueBlock->Code, level + 1);
	DumpTabLevel(level);
	m_Stream << "}";
	if (!stmt->FalseBlock->Code->Empty())
	{
		m_Stream << endLine;
		DumpTabLevel(level);
		m_Stream << "else" << endLine;
		DumpTabLevel(level);
		m_Stream << "{" << endLine;
		DumpCode(stmt->FalseBlock->Code, level + 1);
		DumpTabLevel(level);
		m_Stream << "}";
	}
}

void CodeWriter::DumpSwitch(StmtBase *stmt, uint32 level)
{

	nbASSERT(stmt->Kind == STMT_SWITCH, "stmt must be of STMT_SWITCH type");
	StmtSwitch *swStmt = (StmtSwitch*)stmt;
	if (GenNetIL)
	{
		DumpSwitchNetIL(swStmt, level);
		return;
	}
	DumpTabLevel(level);
	m_Stream << "switch( ";
	DumpTree(swStmt->Forest, level);
	m_Stream << " )" << endLine;
	DumpTabLevel(level);
	m_Stream << "{" << endLine;
	DumpCode(swStmt->Cases, level + 1);
	if (swStmt->Default){
		DumpCase(swStmt->Default, true, level + 1);
	}
	DumpTabLevel(level);
	m_Stream << "}";
}

//!code to dump a switch MIRONode
void CodeWriter::DumpSwitch(SwitchMIRONode *stmt, uint32 level)
{

	nbASSERT(stmt->Kind == STMT_SWITCH, "stmt must be of STMT_SWITCH type");
	SwitchMIRONode *swStmt = dynamic_cast<SwitchMIRONode*>(stmt);
	if (GenNetIL)
	{
		DumpSwitchNetIL(swStmt, level);
		return;
	}
	DumpTabLevel(level);
	m_Stream << "switch( ";
	DumpTree(swStmt->getKid(0), level);
	m_Stream << " )" << endLine;
	DumpTabLevel(level);
	m_Stream << "{" << endLine;
	DumpCode(&swStmt->Cases, level + 1);
	if (swStmt->Default)
		DumpCase(swStmt->Default, true, level + 1);
	DumpTabLevel(level);
	m_Stream << "}";
}

void CodeWriter::DumpSwitchNetIL(StmtSwitch *swStmt, uint32 level)
{
	DumpTabLevel(level);
	DumpTree(swStmt->Forest, level);
	m_Stream << "switch " << swStmt->NumCases << ":" << endLine;
	DumpCode(swStmt->Cases, level + 1);
	if (swStmt->Default)
		DumpCase(swStmt->Default, true, level + 1);
	DumpTabLevel(level);
}

//! method to dump SwitchNetil from MIRONode
void CodeWriter::DumpSwitchNetIL(SwitchMIRONode *swStmt, uint32 level)
{
	DumpTabLevel(level);
	DumpTree(swStmt->getKid(0), level);
	m_Stream << "switch " << swStmt->NumCases << ":" << endLine;
	DumpCode(&swStmt->Cases, level + 1);
	if (swStmt->Default)
		DumpCase(swStmt->Default, true, level + 1);
	DumpTabLevel(level);
}

void CodeWriter::DumpBlock(StmtBlock *stmt, uint32 level)
{
	/*
	string comment;
	if (stmt->Comment.size() > 0)
	comment = stmt->Comment + " ";
	m_Stream << "--- BLOCK " << comment << "START ---" << endLine;
	*/
	DumpCode(stmt->Code, level + 1);
	/*
	m_Stream << "--- BLOCK " << comment << "END ---" << endLine;
	*/

}


//!code to dump a block MIRONode
void CodeWriter::DumpBlock(BlockMIRONode *stmt, uint32 level)
{
	/*
	string comment;
	if (stmt->Comment.size() > 0)
	comment = stmt->Comment + " ";
	m_Stream << "--- BLOCK " << comment << "START ---" << endLine;
	*/
	DumpCode(stmt->Code, level + 1);
	/*
	m_Stream << "--- BLOCK " << comment << "END ---" << endLine;
	*/

}

void CodeWriter::DumpLoop(StmtLoop *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->Kind == STMT_LOOP, "stmt must be a STMT_LOOP");
	DumpTabLevel(level);
	m_Stream << "for (";
	DumpTree(stmt->Forest, level);
	m_Stream << " = ";
	DumpTree(stmt->InitVal, level);
	m_Stream << "; ";
	DumpTree(stmt->TermCond, level);
	m_Stream << "; ";
	DumpStatement(stmt->IncStmt);
	m_Stream << " )" << endLine;
	DumpTabLevel(level);
	m_Stream << "{" << endLine;
	DumpCode(stmt->Code->Code, level + 1);
	DumpTabLevel(level);
	m_Stream << "}";
}


void CodeWriter::DumpDoWhile(StmtWhile *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");

	nbASSERT(stmt->Kind == STMT_DO_WHILE, "stmt must be a STMT_DO_WHILE");
	DumpTabLevel(level);
	m_Stream << "do" << endLine;
	DumpTabLevel(level);
	m_Stream << "{" << endLine;
	DumpTabLevel(level);
	DumpCode(stmt->Code->Code, level + 1);
	m_Stream << "}" << endLine;
	DumpTabLevel(level);
	m_Stream << "while (";
	DumpTree(stmt->Forest);
	m_Stream << " )";

}

void CodeWriter::DumpWhileDo(StmtWhile *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->Kind == STMT_WHILE, "stmt must be a STMT_WHILE");
	DumpTabLevel(level);
	m_Stream << "while(";
	DumpTree(stmt->Forest);
	m_Stream << ")"	<< endLine;
	DumpTabLevel(level);
	m_Stream << "{" << endLine;
	DumpCode(stmt->Code->Code, level + 1);
	DumpTabLevel(level);
	m_Stream << "}";

}

void CodeWriter::DumpBreak(StmtCtrl *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->Kind == STMT_BREAK, "stmt must be a STMT_BREAK");
	DumpTabLevel(level);
	m_Stream << "break";
}

void CodeWriter::DumpContinue(StmtCtrl *stmt, uint32 level)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->Kind == STMT_CONTINUE, "stmt must be a STMT_CONTINUE");
	DumpTabLevel(level);
	m_Stream << "continue";
}

void CodeWriter::DumpComment(StmtComment *stmt, uint32 level)
{
	DumpTabLevel(level);
	if (strcmp((const char *)stmt->Comment.c_str(), "")==0)
		m_Stream << "";
	else
	{
		if (GenNetIL)
			m_Stream << "; " << stmt->Comment;
		else
			m_Stream << "/*** " << stmt->Comment << " ***/";
	}
}

//! Comment dump for PFLNode NEW
void CodeWriter::DumpComment(CommentMIRONode *stmt, uint32 level)
{
	DumpTabLevel(level);
	if (strcmp((const char *)stmt->Comment.c_str(), "")==0)
		m_Stream << "";
	else
	{
		if (GenNetIL)
			m_Stream << "; " << stmt->Comment;
		else
			m_Stream << "/*** " << stmt->Comment << " ***/";
	}
}


void CodeWriter::DumpStatement(StmtBase *stmt, uint32 level)
{
	//uint32 nodeCount = 0;
	//char *stmtKind = NULL;
	nbASSERT(stmt != NULL , "stmt cannot be NULL");
	
	switch(stmt->Kind)
	{
	case STMT_LABEL:
	{
		DumpSymbol(stmt->Forest->Sym);
		//DumpTree(stmt->Forest, level);
		m_Stream << ":";
		}break;
	case STMT_COMMENT:
		DumpComment((StmtComment*)stmt, level);
		break;
	case STMT_GEN:
	{
		if (!GenNetIL)
			DumpTabLevel(level + 1);	
		DumpTree(stmt->Forest, level + 1);
		}break;
	case STMT_JUMP:
	case STMT_JUMP_FIELD:
		DumpJump((StmtJump*)stmt, level + 1);
		break;
	case STMT_CASE:
		DumpCase((StmtCase*)stmt, false, level);
		break;
	case STMT_SWITCH:
		DumpSwitch(stmt, level + 1);
		break;
	case STMT_IF:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		DumpIf((StmtIf*)stmt, level+1);
		break;
	case STMT_LOOP:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		DumpLoop((StmtLoop*)stmt, level + 1);
		break;
	case STMT_BREAK:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		DumpBreak((StmtCtrl*)stmt, level + 1);
		break;
	case STMT_CONTINUE:
		DumpContinue((StmtCtrl*)stmt, level + 1);
		break;
	case STMT_WHILE:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		DumpWhileDo((StmtWhile*)stmt, level + 1);
		break;
	case STMT_DO_WHILE:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		DumpDoWhile((StmtWhile*)stmt, level + 1);
		break;
	case STMT_BLOCK:
		DumpBlock((StmtBlock*)stmt, level + 1);
		break;
	case STMT_FINFOST:
		break;
	default:
		nbASSERT(false, "DUMP_STMT: CANNOT BE HERE");
		break;
	}
	
	if (stmt->Comment.size() > 0 && stmt->Kind != STMT_COMMENT)
	{
		m_Stream << TAB_CHAR << "// " << stmt->Comment;
	}

}

void CodeWriter::DumpStatement(StmtMIRONode *stmt, uint32 level)
{
	//uint32 nodeCount = 0;
	//char *stmtKind = NULL;

	switch(stmt->Kind)
	{
	case STMT_LABEL:
	{
		DumpSymbol(stmt->getKid(0)->Sym);
		//DumpTree(stmt->Forest, level);
		m_Stream << ":";
		}break;
	case STMT_COMMENT:
		DumpComment((CommentMIRONode*)stmt, level);
		break;
	case STMT_GEN:
	{
		if (!GenNetIL)
			DumpTabLevel(level + 1);
		DumpTree(stmt->getKid(0), level + 1);
		}break;
	case STMT_JUMP:
	case STMT_JUMP_FIELD:
		DumpJump((JumpMIRONode*)stmt, level + 1);
		break;
	case STMT_CASE:
		DumpCase((CaseMIRONode*)stmt, false, level);
		break;
	case STMT_SWITCH:
		DumpSwitch((SwitchMIRONode*)stmt, level + 1);
		break;
	case STMT_IF:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		assert(1 == 0);
		//DumpIf((StmtIf*)stmt, level+1);
		break;
	case STMT_LOOP:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		assert(1 == 0);
		//DumpLoop((StmtLoop*)stmt, level + 1);
		break;
	case STMT_BREAK:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		assert(1 == 0);
		//DumpBreak((StmtCtrl*)stmt, level + 1);
		break;
	case STMT_CONTINUE:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		assert(1 == 0);
		//DumpContinue((StmtCtrl*)stmt, level + 1);
		break;
	case STMT_WHILE:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		assert(1 == 0);
		//DumpWhileDo((StmtWhile*)stmt, level + 1);
		break;
	case STMT_DO_WHILE:
		nbASSERT(!GenNetIL, "cannot generate NetIL directly from this kind of statement");
		assert(1 == 0);
		//DumpDoWhile((StmtWhile*)stmt, level + 1);
		break;
	case STMT_BLOCK:
		DumpBlock((StmtBlock*)stmt, level + 1);
		break;
	case STMT_PHI:
		DumpPhi((PhiMIRONode*)stmt);
		break;
	case STMT_FINFOST:
		break;
	default:
		nbASSERT(false, "DUMP_STMT_PFL: CANNOT BE HERE");
		break;
	}
	if (stmt->Comment.size() > 0 && stmt->Kind != STMT_COMMENT)
	{
		m_Stream << TAB_CHAR << "// " << stmt->Comment;
	}

}

void CodeWriter::DumpCode(CodeList *code, uint32 level)
{
	StmtBase *next = code->Front();
	while(next)
	{
		if (next->Flags & STMT_FL_DEAD)
		{
			next = next->Next;
			continue;
		}
/*
		if(next->Kind==STMT_SWITCH)
		   cout<<"SWITCH eiii"<<endl;
		else
		   cout<<" X";
*/
		DumpStatement(next, level);
		//if (next->Kind != STMT_GEN && GenNetIL)
		m_Stream << endLine;
		next = next->Next;
	}
	
//	cout<<"end"<<endl;
	m_Stream.flush();
	//m_Stream << endLine;
}

//!Overloaded method to work with PFLNODE
void CodeWriter::DumpCode(std::list<MIRONode*> *code, uint32 level)
{
	typedef std::list<MIRONode*>::iterator it_t;
	
	for(it_t i = code->begin(); i != code->end(); i++)
	{
		StmtMIRONode *actual = dynamic_cast<StmtMIRONode*>(*i);

		//if (actual->Flags & STMT_FL_DEAD)
		//{
		//	continue;
		//}
		DumpStatement(actual, level);
#if _ENABLE_PFLC_PROFILING
		if(_num_Statements != NULL)
			*_num_Statements++;
#endif
		//if (next->Kind != STMT_GEN && GenNetIL)
		m_Stream << endLine;
	}
	m_Stream.flush();
	//m_Stream << endLine;
}


void CodeWriter::DumpNetIL(CodeList *code, uint32 level)
{
	GenNetIL = true;
	DumpCode(code, level);
	GenNetIL = false;
}

//!Method to dump netilcode from a list of MIRONode
void CodeWriter::DumpNetIL(std::list<MIRONode*> *code, uint32 level)
{

	GenNetIL = true;
	DumpCode(code, level);
	GenNetIL = false;
}

void CodeWriter::DumpSymbol(Symbol *sym, ostream &m_Stream)
{
	bool GenNetIL = false;

	switch (sym->SymKind)
	{
	case SYM_PROTOCOL:
		m_Stream << ((SymbolProto*)sym)->Name;
		break;
	case SYM_FIELD:
		m_Stream << ((SymbolField*)sym)->Name;
		break;
	case SYM_RT_VAR:
		m_Stream << ((SymbolVariable*)sym)->Name;
		break;
	case SYM_TEMP:
		if (GenNetIL)
			m_Stream << ((SymbolTemp*)sym)->Temp;
		else
			m_Stream << ((SymbolTemp*)sym)->Name;
		break;
	case SYM_LABEL:
		m_Stream << ((SymbolLabel*)sym)->Name;
		break;
	case SYM_INT_CONST:
		m_Stream << ((SymbolIntConst*)sym)->Value;
		break;
	case SYM_STR_CONST:
		m_Stream << "\"" << ((SymbolStrConst*)sym)->Name << "\"";
		break;
	case SYM_RT_LOOKUP_ENTRY:
		{
			SymbolLookupTableEntry *entry=(SymbolLookupTableEntry *)sym;
			switch (entry->Validity)
			{
			case LOOKUPTABLE_ENTRY_VALIDITY_ADDONHIT:
				m_Stream << "AddOnHit";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_KEEPFOREVER:
				m_Stream << "KeepForever";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_KEEPMAXTIME:
				m_Stream << "KeepMaxTime";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_REPLACEONHIT:
				m_Stream << "ReplaceOnHit";
				break;
			case LOOKUPTABLE_ENTRY_VALIDITY_UPDATEONHIT:
				m_Stream << "UpdateOnHit";
				break;
			default:
				break;
			}
		}break;
	case SYM_RT_LOOKUP_KEYS:
		{
			SymbolLookupTableKeysList *entry=(SymbolLookupTableKeysList *)sym;

			m_Stream << "<";
			NodesList_t::iterator i=entry->Values.begin();
			while ( i!=entry->Values.end() )
			{
				if (i!=entry->Values.begin())
					m_Stream << ", ";
				if (*i == NULL)
					m_Stream << " ERROR ";
				else
					//DumpTree(*i);
					i++;
			}
			m_Stream << ">";
		}break;
	case SYM_RT_LOOKUP_VALUES:
		{
			SymbolLookupTableValuesList *entry=(SymbolLookupTableValuesList *)sym;

			m_Stream << "<";
			NodesList_t::iterator i=entry->Values.begin();
			while ( i!=entry->Values.end() )
			{
				if (i!=entry->Values.begin())
					m_Stream << ", ";
				if (*i == NULL)
					m_Stream << " ERROR ";
				else
					//DumpTree(*i);
					i++;
			}
			m_Stream << ">";
		}break;
	case SYM_RT_LOOKUP_ITEM:
		{
			SymbolLookupTableItem *item=(SymbolLookupTableItem *)sym;
			m_Stream << item->Table->Name << "." << item->Name;
		}break;
	default:
		nbASSERT(false, "DUMP SYMBOL: CANNOT BE HERE");
		break;
	}
}

void CodeWriter::DumpOpCode_s(uint16_t opcode, ostream& m_Stream)
{
	if (opcode == IR_LABEL)
		return;

	if (opcode > mir_first_op)
	{
		m_Stream << NvmOps[opcode - mir_first_op - 1].Name;
		return;
	}

	m_Stream << OpDescriptions[GET_OP(opcode)].Name;
	if (GET_OP_TYPE(opcode) > 0)
		m_Stream << "." << IRTypeNames[GET_OP_TYPE(opcode)];
}

void CodeWriter::DumpPhi(PhiMIRONode *stmt, uint32 level)
{
	stmt->printNode(m_Stream, true);
}
