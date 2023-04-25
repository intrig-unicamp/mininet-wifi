/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "op_size.h"
#include "netvmjitglobals.h"
#include "basicblock.h"

#include "irnode.h"
#include "mirnode.h"
#include "taggable.h"
#include "../opcodes.h"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <map>
#include <list>
using namespace std;

/* FIXME: this code does not work correctly because it does not propagate correctly the register size
 * when a (smaller) register is used in a regload and then it is written to memory as a bigger value
 * than previously thought. In this case it is not immediatly clear what to do with the value.
 * I suppose we could just resize the whole live range size to the bigger value, but this requires
 * an intermediate step while rewriting that modifies the opcodes to match the new register size.
 */
 
namespace jit {

 
Infer_Size::Infer_Size(Infer_Size::CFG &c) : cfg(c)
{
	// Nop
	return;
}

Infer_Size::~Infer_Size()
{
	// Nop
	return;
}

void Infer_Size::run()
{
	annotate();
	adapt();
	return;
}
 
void Infer_Size::adapt()
{		
	// Make the modification pass
	list<BasicBlock *> *bblist(cfg.getBBList());
	list<BasicBlock *>::iterator i;
	
	for(i = bblist->begin(); i != bblist->end(); ++i) {
		Touch_Constants(*i);
	}
	
	delete bblist;

	return;
}

void Infer_Size::annotate()
{
//	regsize_map_t map;

	// Make the first pass for flagging
	visitor_find_live_range_size visitor_flag(cfg, *this);
	cfg.PreOrder(*cfg.getEntryNode(), visitor_flag);

	{
		cout << "After first pass\n";
		regsize_map_t::iterator i;
		for(i = map.begin(); i != map.end(); ++i) {
			cout << i->first << ":\t" << i->second << '\n';
		}
	}

	{
		// Second pass for propagation
		visitor_propagate vp(cfg, *this);
		cfg.PreOrder(*cfg.getEntryNode(), vp);
	}

	{
		cout << "After second pass\n";
		regsize_map_t::iterator i;
		for(i = map.begin(); i != map.end(); ++i) {
			cout << i->first << ":\t" << i->second << '\n';
		}
	}
	
	{
		// Third pass (fixes some constants)
		visitor_propagate vp(cfg, *this);
		cfg.PreOrder(*cfg.getEntryNode(), vp);
	}
	
	{
		cout << "After last pass\n";
		regsize_map_t::iterator i;
		for(i = map.begin(); i != map.end(); ++i) {
			cout << i->first << ":\t" << i->second << '\n';
		}
	}

	// Make the last pass for rewriting
	visitor_set_live_range_size visitor_set(cfg, *this);
	cfg.PreOrder(*cfg.getEntryNode(), visitor_set);

	return;
}

bool Infer_Size::visitor_find_live_range_size::operator()(Infer_Size::BasicBlock *bb)
{
	is.Find_Live_Range_Size(bb);
	return true;
}

bool Infer_Size::visitor_set_live_range_size::operator()(Infer_Size::BasicBlock *bb)
{
	is.Set_Live_Range_Size(bb);
	return true;
}

bool Infer_Size::visitor_propagate::operator()(Infer_Size::BasicBlock *bb)
{
	is.Propagate(bb);
	return true;
}

void Infer_Size::Propagate(Infer_Size::BasicBlock *BB)
{
	if (!BB)
		return;
	
	list<CFG::IRType *> &code_list(BB->getCode());
	list<CFG::IRType *>::iterator i;
	
	
	for(i = code_list.begin(); i != code_list.end(); ++i) {
		/* If there are sons that do not have a set size (as an example, constants), we
		 * must propagate the size from here. In practice the kids get to know that their result
		 * will be used in a maxSize operation. */
		operand_size_t maxSize((*i)->getProperty<operand_size_t>("size"));
		
		if((*i)->getOwnReg()) {
			maxSize = max(maxSize, map[*((*i)->getOwnReg())]);
		}
		
		// Do not propagate to stores, the address must be left untouched
		// Do not propagate to loads for the same reason
		switch(nvmOpCodeTable[(*i)->getOpcode()].Consts & OPCODE_MASK) {
			case OP_LSTORE:
			case OP_STORE:
				break;

			case OP_LLOAD:
			case OP_LOAD:
				break;
				
			default:
				propagate_size((*i)->getKid(0), maxSize);
		}
	
		propagate_size((*i)->getKid(1), maxSize);

	}
	

	return;
}


void Infer_Size::Touch_Constants(Infer_Size::BasicBlock *BB)
{
	if (!BB)
		return;
		
	list<CFG::IRType *> &code_list(BB->getCode());
	list<CFG::IRType *>::iterator i;
	
	for(i = code_list.begin(); i != code_list.end(); ++i) {
		Touch_Insn_Constants(*i);
	}
	
	return;
}

void Infer_Size::Touch_Insn_Constants(MIRNode *insn)
{
	if(!insn)
		return;

	operand_size_t t = insn->getProperty<operand_size_t>("size");
	insn->set_preferred_size(t);
	
	Touch_Insn_Constants(insn->getKid(0));
	Touch_Insn_Constants(insn->getKid(1));
	
	return;
}


void Infer_Size::Find_Live_Range_Size(Infer_Size::BasicBlock *BB)
{
	if (!BB)
		return;
	
	list<CFG::IRType *> &code_list(BB->getCode());
	list<CFG::IRType *>::iterator i;
	
	for(i = code_list.begin(); i != code_list.end(); ++i) {
		Find_Insn_Live_Range_Size(*i);
	}
	
	return;
}

void Infer_Size::Set_Live_Range_Size(Infer_Size::BasicBlock *BB)
{
	if (!BB)
		return;

	list<CFG::IRType *> &code_list(BB->getCode());
	list<CFG::IRType *>::iterator i;
	
	for(i = code_list.begin(); i != code_list.end(); ++i) {
		Set_Insn_Live_Range_Size(*i);
	}
	
	return;
}

void Infer_Size::Set_Insn_Live_Range_Size(MIRNode *insn)
{
	regsize_map_t::iterator entry;
        // unused
	// operand_size_t maxSize = insn->getProperty<operand_size_t>("size");
	
	// Rewrites the kids
	for(unsigned i = 0; i < 2; ++i) {
		if(insn->getKid(i)) {
			Set_Insn_Live_Range_Size(insn->getKid(i));
		}
	}
	
	cout << "Esamino l'operatore " << nvmOpCodeTable[insn->getOpcode()].CodeName << '\n';
	
	// Rewrites the father
	if(insn->getOwnReg()) {	
		if((entry = map.find(insn->getDefReg())) == map.end()) {
			assert(1 == 0 && "Operand not found, internal error");
			return;
		}

		cout << "Riscrivo l'operatore " << nvmOpCodeTable[insn->getOpcode()].CodeName << " con "
			<< entry->second << '\n';

		insn->setProperty<operand_size_t>("size", entry->second);
		insn->set_preferred_size(entry->second);
		// maxSize = entry->second;
	}

	return;
}

Infer_Size::regsize_map_t *Infer_Size::getMap()
{
	return &map;
}

void Infer_Size::Find_Insn_Live_Range_Size(MIRNode* insn)
{
	MIRNode::RegType *e(0);
	regsize_map_t::iterator entry(map.end());
//	operand_size_t maxSize = no_size;
	operand_size_t maxSize = no_size;
	
	if(!insn)
		return;
		
//	cout << "Esamino l'operatore\n";
//	insn->printNode(cout, true);
//	cout << '\n';
	
	// Call itself on the kids, so they have their sizes defined
	for(unsigned i = 0; i < 2; ++i) {
		if(insn->getKid(i)) {
			Find_Insn_Live_Range_Size(insn->getKid(i));
		}
	}
	
	if(insn->getOwnReg()) {
		/* Here we need to choose the size for our result */
		
//		cout << "L'opcode " << nvmOpCodeTable[insn->getOpcode()].CodeName << " definisce 1 operatore.\n";
		
		/* Look if per chance the operand size has already been defined */
		entry = map.find(*(insn->getOwnReg()));
		e = insn->getOwnReg();
	} else {
		// Handle stores the same way.
	
		switch(insn->getOpcode()) {
			case LSTORE8: case PBSTR:
//				cout << "L'opcode " << nvmOpCodeTable[insn->getOpcode()].CodeName << " e' una store.\n";
				assert(insn->getKid(1));
				e = insn->getKid(1)->getOwnReg();
				
				maxSize = byte8;
				
				// HACK: offsets must be at least 16 bits wide...
				if(insn->getKid(0)->getOwnReg()) {
					map[*(insn->getKid(0)->getOwnReg())] = max(word16, map[*(insn->getKid(0)->getOwnReg())]);
				}
				
				break;

			case LSTORE16: case PSSTR: case ISSTR:
//				cout << "L'opcode " << nvmOpCodeTable[insn->getOpcode()].CodeName << " e' una store.\n";
				assert(insn->getKid(1));
				e = insn->getKid(1)->getOwnReg();
				maxSize = word16;
				
				// HACK: offsets must be at least 16 bits wide...
				if(insn->getKid(0)->getOwnReg()) {
					map[*(insn->getKid(0)->getOwnReg())] = max(word16, map[*(insn->getKid(0)->getOwnReg())]);
				}
				
				break;
						
			case LSTORE32: case PISTR: case IISTR:
//				cout << "L'opcode " << nvmOpCodeTable[insn->getOpcode()].CodeName << " e' una store.\n";
				assert(insn->getKid(1));
				e = insn->getKid(1)->getOwnReg();
				maxSize = dword32;
				
				// HACK: offsets must be at least 16 bits wide...
				if(insn->getKid(0)->getOwnReg()) {
					map[*(insn->getKid(0)->getOwnReg())] = max(word16, map[*(insn->getKid(0)->getOwnReg())]);
				}
				
				break;
						
			default:
//				maxSize = size_unknown;
				;
		}
		
		if(e)
			entry = map.find(*e);
	}

	//			hashEntry = Int_HTbl_LookUp(hashTable, insn->Value, 0, HASH_TEST_ONLY);
		
	/* Detect the most appropriate operand size for this instruction */
	switch(nvmOpCodeTable[insn->getOpcode()].Consts & OPCODE_MASK) {
		case OP_LLOAD:
		case OP_LOAD:
			switch(insn->getOpcode()) {
				case LLOAD8:
				case PBLDU: case PBLDS:
					maxSize = byte8;
					
					// HACK: offsets must be at least 16 bits wide...
					if(insn->getKid(0)->getOwnReg()) {
						map[*(insn->getKid(0)->getOwnReg())] = max(word16, map[*(insn->getKid(0)->getOwnReg())]);
					}

					break;	
				case LLOAD16:
				case PSLDU: case PSLDS:
				case ISSSLD:
					maxSize = word16;
					
					// HACK: offsets must be at least 16 bits wide...
					if(insn->getKid(0)->getOwnReg()) {
						map[*(insn->getKid(0)->getOwnReg())] = max(word16, map[*(insn->getKid(0)->getOwnReg())]);
					}
					
					break;
				case LLOAD32:
				case PILD:
				case ISSILD:
					maxSize = dword32;
					
					// HACK: offsets must be at least 16 bits wide...
					if(insn->getKid(0)->getOwnReg()) {
						map[*(insn->getKid(0)->getOwnReg())] = max(word16, map[*(insn->getKid(0)->getOwnReg())]);
					}
					
					break;
				default:
					maxSize = size_unknown;
			}
			break;
	
#if 0
		/* FIXME: LSTORE does not define anything, its code does NOT belong here */				
		case OP_LSTORE:
		case OP_STORE:
			//					cout << "Riconosco LSTORE" << '\n';
			switch(insn->getOpcode()) {
				case LSTORE8:
				case PBSTR:
					maxSize = byte8;
					break;
						
					case LSTORE16:
					case PSSTR:
					case ISSTR:
						maxSize = word16;
						break;
						
					case LSTORE32:
					case PISTR:
					case IISTR:
						maxSize = dword32;
						break;
						
					default:
						maxSize = size_unknown;
				}
				break;
#endif
		case OP_ARITH:
			//					cout << "Ho un operatore aritmetico\n";
			maxSize = insn->getKid(0)->getProperty<operand_size_t>("size");
			switch(insn->getOpcode()) {
				/* These are operators which operands really 'matter', because the result
				 * width depends on them, and they have 2 operands */
				case ADD:  case ADDSOV:  case ADDUOV:
				case SUB:  case SUBSOV:  case SUBUOV:
				case MOD:  case XOR: case AND: case OR:
					maxSize = max(maxSize, insn->getKid(1)->getProperty<operand_size_t>("size"));
					//							cout << "L'operatore aritmetico ha 2 figli!\n";
					break;
				case IMUL: case IMULSOV: case IMULUOV:
					maxSize = (operand_size_t)(maxSize + maxSize);
					maxSize = (operand_size_t)(maxSize - !!(maxSize > 3));
					{
						operand_size_t other_son(insn->getKid(1)->getProperty<operand_size_t>(
							"size"));
						other_son = (operand_size_t)(other_son + other_son);
						other_son = (operand_size_t)(other_son - !!(other_son > 3));
						maxSize = max(maxSize, other_son);
					}
					break;
			}
			break;
				
		case OP_LDPKTB:
			/* the offset is always 32 bits wide */
			//					cout << "giving load.packet.base " << SZ_DWORD << '\n';
			maxSize = dword32;
			break;
				
		case OP_STREG:
			/* regstore does what its son does */
			maxSize = insn->getKid(0)->getProperty<operand_size_t>("size");
			//					cout << "streg ha figlio di dimensione " << maxSize << '\n';
			break;
				
		case OP_LDREG:
		case OP_PHI:
			/* does not do anything, will be propagated */
			break;
		
		case OP_STACK:
			switch(insn->getOpcode()) {
				case PBL:
//					cout << "HANDLING PBL\n";
					maxSize = word16;
					break;
				default:
					;
			}
			break;

		case OP_COP:
			// HACK: each coprocessor register is 32 bits by default
			maxSize = dword32;
				
		default:
//			cout << "I forgot about something here: " << nvmOpCodeTable[insn->getOpcode()].CodeName
//			<< '\n';
//			maxSize = size_unknown;
			;
	} /* switch(... & OPCODE_MASK) */

	if(entry != map.end()) {
//		cout << "ho gia` un valore per l'operando di " /* << nvmOpCodeTable[insn->Code].CodeName */
//			<< ": " << entry->second << '\n';
		maxSize = max(maxSize, entry->second);
	}
				
//	cout << "Proposed dimension for " << nvmOpCodeTable[insn->getOpcode()].CodeName << ": " <<
//		maxSize << '\n';
				
	/* Overwrite the entry with the newly calculated size */
	if(e) {
//		cout << '\t' << *e << "\t<- "
//			<< maxSize << ' ' << nvmOpCodeTable[insn->getOpcode()].CodeName << '\n';
		map[*e] = maxSize;
	}

	/* If this is a constant, put an appropriate value here */
	if(insn->getOpcode() == CNST) {
		ConstNode *n(dynamic_cast<ConstNode *>(insn));
		insn->setProperty<operand_size_t>("size", get_constant_size(n->getValue()));
	} else
		insn->setProperty<operand_size_t>("size", maxSize);
		
//	cout << "Real dimension for " << nvmOpCodeTable[insn->getOpcode()].CodeName << ": " <<
//		insn->getProperty<operand_size_t>("size") << '\n';

	return;
}

void Infer_Size::propagate_size(MIRNode *insn, operand_size_t father_suggestion)
{
	if(!insn)
		return;
		
//	cout << "propagate_size per\n";
//	insn->printNode(cout, true);
//	cout << '\n';
	
	switch(nvmOpCodeTable[insn->getOpcode()].Consts & OPCODE_MASK) {
		case OP_LOAD:
		case OP_LLOAD:
			/* do NOT change the size of the produced register! */
			return;
	}


	switch(insn->getOpcode()) {
		case CNST:
			/* Here we get out chance to have our size correctly defined.
			 * If it is already, we do not do a thing - we might want to sign extend if
			 * we know we are shorter than the required.
			 * Otherwise, we use the parent's suggestion to set our size */
			if(father_suggestion != size_unknown) {
//				cout << "father had a good idea: " << father_suggestion << "\n";
//			if(insn->getProperty<operand_size_t>("size") != size_unknown)
				insn->setProperty<operand_size_t>("size", max(
						insn->getProperty<operand_size_t>("size"), father_suggestion
					)
				);
			}
				
			/* If after all the size is still not defined, we try to deduce it from the numerical
			 * value of this constants */
			if(!insn->getProperty<operand_size_t>("size")) {
//				cout << "overwriting with my own size\n";
				ConstNode *n(dynamic_cast<ConstNode *>(insn));
				insn->setProperty<operand_size_t>("size", get_constant_size(n->getValue()));
			}
			break;
			
		default:
			/* I really can't remember what I had in mind when I wrote this. */
#if 0
			/* Then there is a bunch of arithmetic opcodes that use a 'meta-operand' that need special
			 * treatment. As an example, the left shift operand may as well work on 8 bit values, but
			 * often the result is needed on many more bits. */
			switch(nvmOpCodeTable[insn->getOpcode()].Consts & OPCODE_MASK) {
				case OP_ARITH:
					switch(insn->getOpcode()) {
						case SHL: case SHR: case USHR:
						case ROTL: case ROTR:
							insn->setProperty<operand_size_t>("size", 
								max(insn->getProperty<operand_size_t>("size"), father_suggestion));
							/* Note that we do not insert this size value in any hashtable. This is
							 * because here we treat subtrees, not statements, and whatever happens to
							 * be in a subtree node Value is most certainly to be discarded after this
							 * statement, and never needed again.
							 * THIS WILL BE FALSE as soon as we implement common subexpression culling.
							 * - Pierluigi
							 */
							break;
						default:
							/* do nothing */
							;
					}
					break;
				default:
					/* I still have to think about other ops need special treatment - Pierluigi */
					;
			}
#endif
			if(insn->getProperty<operand_size_t>("size") < father_suggestion) {
				insn->setProperty<operand_size_t>("size", father_suggestion);
				
				// This will need to be fixed, sooner or later
				if(insn->getOwnReg()) {
					map[*(insn->getOwnReg())] = max(map[*(insn->getOwnReg())], father_suggestion);
//					cout << "Dopo propagate " << *(insn->getOwnReg()) << "  " << map[*(insn->getOwnReg())]
//						<< '\n';
				}
			}

	}
	
	father_suggestion = insn->getProperty<operand_size_t>("size");
	
//	cout << "final size: " << father_suggestion << '\n';
	
	// Do not propagate to stores, the address must be left untouched
	// Do not propagate to loads for the same reason
	switch(nvmOpCodeTable[insn->getOpcode()].Consts & OPCODE_MASK) {
		case OP_LSTORE:
		case OP_STORE:
			break;
			
		case OP_LLOAD:
		case OP_LOAD:
			break;
			
		case OP_ARITH:
			switch(insn->getOpcode()) {
				case IMUL:
					break;
				default:
					propagate_size(insn->getKid(0), father_suggestion);					
			}
			break;

		default:
			propagate_size(insn->getKid(0), father_suggestion);
	}


	switch(nvmOpCodeTable[insn->getOpcode()].Consts & OPCODE_MASK) {
		case OP_ARITH:
			switch(insn->getOpcode()) {
				case IMUL:
					break;
				default:
					propagate_size(insn->getKid(1), father_suggestion);					
			}
			break;

		default:
			propagate_size(insn->getKid(1), father_suggestion);
	}

	return;
}

operand_size_t Infer_Size::get_constant_size(uint32_t cnst, bool has_sign)
{
	/* This is actually impossible to do correctly because signed values fill the whole upper part of
	 * the dword with ones. At the same time we cannot assume that a constant with many ones in the
	 * higher part is negative for sure (think of what would happen with 255.255.255.0 netmasks),
	 * so we have the has_sign parameter which will save us from destruction if and when we decide to
	 * keep track of the signedness as well (that will have to be done during parsing, I suppose).
	 * - Pierluigi */
	 
	 // HACK: if the highest bit is 1, then we assume it is signed.
	 int32_t _cnst((int32_t)cnst);
	 
	 if (_cnst < 0)
		has_sign = true;
	 
	 if (has_sign)
		cnst = -cnst;
	
	// Find the first word that has a non-zero bit.
	int i(3);
	for(; i > 0 && !(cnst & (0xFF << (i*8))); --i)
		;
		
	// Anything bigger than 1 becomes 2 because we do not distinguish between 32 and 24 bits (yet?)
	// Plus one because register_size_t 0 is SZ_NOSIZE
	i = (i > 1 ? 2 : i) + 1;
	
//	cout << cnst << " ha dimensione " << i << '\n';
	
	return (operand_size_t)i;
}

} /* namespace jit */
