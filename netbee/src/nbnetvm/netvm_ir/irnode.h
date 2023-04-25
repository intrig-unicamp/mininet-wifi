/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

/*!
 * \file irnode.h
 * \brief this file contains the definition of the base class of each instruction representation and the register manager definition
 */

#include <set>
#include <map>
#include <stdint.h>
#include "taggable.h"

namespace jit {

	typedef enum {
		no_size = 0,	/* leave this as the _first_ one */
		byte8,
		word16,
		dword32,
		qword64,
		size_unknown,	/* leave this as the _last_ one */
	} operand_size_t;


	/*!
	 * \brief an abstract class for every instruction representation
	 *
	 * This class is a template of the register type and the opcode type.<br>
	 * It contains the opcode of this instruction, the basic block Id and some methods that should be implemented
	 * by every derived class
	 */
	template<class _RegType /* , class _OpCodeType */>
		class IRNode: public Taggable
	{
		public:
			typedef _RegType RegType; //!< the register type of this instruction

			//!constructor
			IRNode( /* OpCodeType opcode, */ uint32_t BBId = 0);
			//!return the BB this instruction belongs to
			uint32_t belongsTo() const;
			virtual void setBBId(uint16_t bbId);
			/*!
			 * \brief get the set of register defined by this node
			 * \return the set of register defined by this node
			 */
			virtual std::set<RegType> getDefs() = 0;
			/*!
			 * \brief get the set of register used by this node
			 * \return the set of register used by this node
			 */
			virtual std::set<RegType> getUses() = 0;
			
			/*!
			 * \brief get the set of registers used by this node and its sons
			 * \return the set of registers used by this node and its sons
			 */
			virtual std::set<RegType> getAllUses() {return getUses();};

			/*!
			 * \brief get the set of registers defined by this node and its sons
			 * \return the set of registers defined by this node and its sons
			 */
			virtual std::set<RegType> getAllDefs() {return getDefs();};
		
			//!destructor
			virtual ~IRNode() {}
			
			virtual uint32_t get_defined_count() {
				return getDefs().size();
			}
			
			/*!
			 * \brief Provides a way to specify the most appropriate dimension for this node's register
			 * \param size the preferred size for this node's defined / used register(s)
			 *
			 * This is used to notify a node of the results of the operands size inference algorithm.
			 */
			virtual void set_preferred_size(operand_size_t size) {
				// Nop
				return;
			}

			//virtual std::set<uint32_t> getTargets() = 0;
			
			/*!
			 * \brief provides a way to specify if an instruction has side_effects
			 *
			 * Needed from dead code elimination. If the method returns true then the instruction
			 * is not removed even if its defined reg is not used
			 */
			virtual bool has_side_effects() {return false;}

			virtual bool isPFLStmt() { return false;};

			//virtual IRNode* getTree() { return this; }

		protected:
			uint32_t BBid; //!<the basic block id this instruction belongs to

	};

	template <class _RegType, class _OpCodeType>
	class TableIRNode : public IRNode<_RegType>
	{
		public:
			
			typedef _OpCodeType OpCodeType; //!< the opcode type of this instruction 

			TableIRNode(OpCodeType opcode, uint32_t BBId = 0);

			//!return the opcode of this instruction
			virtual OpCodeType getOpcode() const; 
			//!set the opcode of this instruction
			void setOpcode(OpCodeType opcode); 

			OpCodeType OpCode; //FIXME
		protected:
	};

	/*!
	 * \param opcode the opcode of this instruction
	 * \param BBId the is of the BB we belong to
	 */
	template<class RegType /*, class OpCodeType */>
	IRNode<RegType /*,OpCodeType */>::IRNode(/* OpCodeType opcode, */ uint32_t BBId) : Taggable(), /* OpCode(opcode), */ BBid(BBId) 
	{}

	template<class RegType , class OpCodeType >
	OpCodeType TableIRNode<RegType,OpCodeType>::getOpcode() const
	{
		return OpCode;
	}

	template<class RegType , class OpCodeType >
	void TableIRNode<RegType,OpCodeType>::setOpcode(OpCodeType opcode)
	{
		OpCode = opcode;
	}

	template<class RegType, class OpCodeType>
	TableIRNode<RegType, OpCodeType>::TableIRNode(OpCodeType opcode, uint32_t BBId) : IRNode<RegType>(BBId), OpCode(opcode)
	{}

	template<class RegType /*, class OpCodeType */>
	uint32_t IRNode<RegType /*,OpCodeType */>::belongsTo() const
	{
		return BBid;
	}
	
	template<class RegType>
	void IRNode<RegType>::setBBId(uint16_t bbId)
	{
		BBid = bbId;
		return;
	}
}

