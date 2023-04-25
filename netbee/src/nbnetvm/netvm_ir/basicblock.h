/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file basicblock.h
 * \brief definition of the template class basic block
 *
 * This file contains the definition of the class basic block
 */
#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "digraph.h"
#include "irnode.h"
#include <iostream>
#include <set>
#include <cassert>

namespace jit {

	/*!
	 * \brief a struct to keep all liveness info together
	 * It contains all the sets used by liveness analysis algorithms implemented using nbBitVector
	 */
//	template<typename _Content>
//	struct _liveness_info {
//		typedef _Content ContentType;
//	
//		std::set<_Content> UEVarSet;             //!< Upward-Exposed Variables Set, i.e. variables that are used in a Basic Block X before any redefinition in X
//		std::set<_Content> VarKillSet;           //!< Variables that are defined in the Basic Block
//		std::set<_Content> LiveOutSet;           //!< Variables that are Live at the end of the Basic Block
//		std::set<_Content> LiveInSet;            //!< Variables that are Live at the beginning of the Basic Block
//
//		/*!
//		 * \brief constructor
//		 * \param size The size of the sets
//		 * Initialize every set to an empty bit vector
//		 */
//		_liveness_info() {}
//
//		template<class T>
//		friend std::ostream& operator<<(std::ostream& os, const struct _liveness_info<T>& linfo);
//	}; 

//	typedef struct _liveness_info Liveness_Info;

	template <class T> class BasicBlock;

	template <class T> class CFGBuilder;

	/*!
	 * \brief class representing a basic block of code
	 *
	 * A basic block is a sequence of instructions which you can enter only from the start and exit only from the end
	 * They are the building blocks of the CFG.
	 * They mainly contain a list of code and information about liveness.
	 * They also contain a reference to the node of the CFG digraph
	 */
	template <class IR>
		class BasicBlock: public Taggable
		{
			friend class CFGBuilder<IR>;

			public:

			typedef IR IRNode;											 //!<instruction type
			typedef typename IRNode::RegType RegType;						 //!<instruction register type
//			typedef RegisterManager<RegType> manager_t;					 //!<Register manager for this instruction type
			typedef typename std::list<IRNode*>::iterator IRStmtNodeIterator; //!<iterator on instruction list
			typedef BasicBlock ThisType;

			typedef typename DiGraph<ThisType*>::GraphNode node_t; //!<digraph node type

			static const uint16_t EXIT_BB = 0xFFFF; //!< the id for the exit BB
			static const uint16_t ENTRY_BB = 0; //!< the id for the entry BB

			/*!
			 * \brief add an instruction to this BB
			 * \param node the instruction to add
			 */
			void addNewInstruction(IR *node);

			void addNewInstructionAtPosition(IR *node, typename std::list<IRNode*>::iterator iterator);

			//! Return a pointer to the node in the graph
			virtual node_t * getNodePtr() {return nodePtr; }

			//typedef struct _liveness_info<typename IR::RegType> Liveness_Info;

			private:
			std::list<IR*> mlCode; //!<list of pointers to the instruction of this BB
			uint16_t Id; //!<Basic Block Id

			uint32_t startpc; //!<starting program counter in the bytecode
			uint32_t endpc;   //!<end program counter in the bytecode
			uint16_t inOrder; //!<in order number of the BB

			node_t *nodePtr; //!<pointer to the node in the digraph

			//liveness info
			//Liveness_Info *liveness_info; //!< pointer to the liveness info struct

			/*!
			 * \brief reset liveness info of this BB
			 * \param size size of the new bitvectors
			 *
			 * It frees old liveness information if any and then reinitializes them with specified size
			 */
			public:
		//	void reset_Liveness()
		//	{
		//		if(liveness_info)
		//			delete liveness_info;

		//		liveness_info = new Liveness_Info();
		//	}


			~BasicBlock();
			
			node_t *getNode() {return nodePtr;};
			
			/*!
			 * \brief set the associate node in the DiGraph
			 *
			 * \param node the node pointer in the DiGraph
			 */
			void setNode(node_t *node) {nodePtr = node; }
			
			/*!
			 * \brief add a copy operation at the beginning of the basic block, but after every phi function
			 * \param source the source register
			 * \param dest the destination register
			 */
			void add_copy_head(RegType source, RegType dest);
			
			/*!
			 * \brief add a copy operation at the end of the basic block, but before any jump operation
			 * \param source the source register
			 * \param dest the destination register
			 */
			void add_copy_tail(RegType source, RegType dest);

			/*!
			 * \brief return the number of instruction of this basic block
			 * \return number of instruction of this basic block
			 */
			uint32_t get_insn_num() const;


			uint32_t getCodeSize() { return get_insn_num(); }

			/*!
			 * \brief sets the in order number
			 * \param value number to set
			 */
			void setInOrder(uint16_t value) {inOrder = value;}
			/*!
			 * \brief sets the start program counter
			 * \param value number to set
			 */
			void setStartpc(uint32_t value) {startpc = value;}
			/*!
			 * \brief sets the end program counter
			 * \param value number to set
			 */
			void setEndpc(uint32_t value) {endpc = value;}

			/*!
			 * \brief return the liveness info
			 * \return a pointer to the liveness info of this BB
			 */
			//Liveness_Info* get_LivenessInfo() const { return liveness_info; }

			/*!
			 * \brief get an iterator for the code list
			 * \return an iterator at the start of the code list
			 */
			IRStmtNodeIterator codeBegin() { return mlCode.begin(); }

			/*!
			 * \brief get an iterator for the code list
			 * \return an iterator at the end of the code list
			 */
			IRStmtNodeIterator codeEnd() { return mlCode.end(); }

			/*!
			 * \brief get the code list
			 * \return the code list of this BB
			 */
			std::list<IR*>& getCode() { return mlCode; }

			/*!
			 * \brief deletes code from this BB list
			 * \param start iterator to the first element to delete
			 * \return iterator to the element after the last element deleted
			 */
			IRStmtNodeIterator eraseCode(IRStmtNodeIterator start) { return mlCode.erase(start); }

			/*!
			 * \brief deletes code from this BB list
			 * \param start iterator to the point from which to start deleting
			 * \param end iterator to the last element to delete
			 * \return iterator to the element after the last element deleted
			 */
			IRStmtNodeIterator eraseCode(IRStmtNodeIterator start, IRStmtNodeIterator end) { return mlCode.erase(start, end); }

			/*!
			 * \brief return the start program counter in the bytecode
			 * \return the pc of the starting instruction of this basic block in the bytecode
			 */
			uint32_t getStartPc() { return startpc; }

			/*!
			 * \brief return the end program counter in the bytecode
			 * \return the pc of the last instruction of this basic block in the bytecode
			 */
			uint32_t getEndPc() { return endpc; }

			/*!
			 * \brief insert an instruction at the start of this BB
			 * \param node the node to insert
			 */
			void addHeadInstruction(IR *node)
			{
				//NETVM_ASSERT(node != NULL, __FUNCTION__ "Adding a NULL Node to a BasicBlock");
				assert(node != NULL && "Adding a NULL Node to a BasicBlock");
				mlCode.push_front(node);
			}

			/*!
			 * \brief insert an instruction at the end of this BB
			 * \param node the node to insert
			 */
			void addTailInstruction(IR *node)
			{
				//NETVM_ASSERT(node != NULL, __FUNCTION__ "Adding a NULL Node to a BasicBlock");
				assert(node != NULL && "Adding a NULL Node to a BasicBlock");
				mlCode.push_back(node);
			}

			IR * getLastStatement();


			/*!
			 * \brief an iterator class for the successor and predecessor of this BB
			 *
			 */
			//forward declaration
			class BBIterator;

			/*!
			 * \brief constructo
			 * \param Id the id of the new BB
			 */
			BasicBlock(uint16_t Id);

			//BasicBlock(const BasicBlock& bb) : Taggable(), Id(bb.Id), startpc(0xffffffff), endpc(0), liveness_info(NULL) {}
			//~BasicBlock();

			/*!
			 * \brief get the Id of the BB
			 * \return the Id of the BB
			 */
			uint16_t getId() const { return Id; }

			/*!
			 * \brief get this node successors
			 * \return a list of this node's successor
			 */
			std::list<node_t *>& getSuccessors() { return nodePtr->GetSuccessors(); }

			/*!
			 * \brief get this node predecessors
			 * \return a list of this node's predecessors
			 */
			std::list<node_t *>& getPredecessors() { return nodePtr->GetPredecessors(); }

			/*!
			 * \brief remove BasicBlock from this node successors
			 * \param BBId of the basicblock to be removed
			 */
			void removeFromSuccessors(uint16_t BBId);

			/*!
			 * \brief remove BasicBlock from this node predecessors
			 * \param BBId of the basicblock to be removed
			 */
			void removeFromPredecessors(uint16_t BBId);

			/*!
			 * \brief get an iterator for the successors list
			 * \return an iterator to the start of the successors list
			 */
			BBIterator succBegin() { return BBIterator(nodePtr->GetSuccessors().begin()); }

			/*!
			 * \brief get an iterator for the successors list
			 * \return an iterator to the end of the successors list
			 */
			BBIterator succEnd() { return BBIterator(nodePtr->GetSuccessors().end()); }

			/*!
			 * \brief get an iterator for the predecessors list
			 * \return an iterator to the start of the predecessors list
			 */
			BBIterator precBegin() { return BBIterator(nodePtr->GetPredecessors().begin()); }

			/*!
			 * \brief get an iterator for the predecessors list
			 * \return an iterator to the end of the predecessors list
			 */
			BBIterator precEnd() { return BBIterator(nodePtr->GetPredecessors().end()); }

			template<class T>
				friend std::ostream& operator<<(std::ostream& os, BasicBlock<T>& BB);

			//! compute new upward exposed killed var sets
			void compute_UEVarSets(/* manager_t& manager */);
			//! perform liveness analysis algorithm on successors
			bool compute_VarSets();

			struct cmpGraphNodeById;

			bool operator==(const BasicBlock<IR> &bb) const;
		}; /* class BasicBlock */

	template<class IR>
		class BasicBlock<IR>::BBIterator
		{
			public:
				typedef typename std::list<node_t *>::iterator node_iterator_t; ///<node iterato type

				typedef ThisType* value_type; ///<type returned from this iterator
				typedef ptrdiff_t difference_type; ///<type of the difference between two iterators
				typedef ThisType** pointer; ///<type of pointer to a value returned from this iterator
				typedef ThisType*& reference; ///<type of a reference to a value returned from this iterator
				typedef std::forward_iterator_tag iterator_category; ///<category of this iterator

			private:
				node_iterator_t node_iterator; ///<iterator to the undelying list

			public:
				BBIterator(node_iterator_t node_iterator): node_iterator(node_iterator) {}

				BBIterator& operator=(const BBIterator &bbit) 
				{
					this->node_iterator = bbit.node_iterator;
					return *this;
				};

				ThisType*& operator*()
				{
					return (*node_iterator)->NodeInfo;
				}

				ThisType** operator->()
				{
					return (*node_iterator)->NodeInfo;
				}

				BBIterator& operator++()
				{
					node_iterator++;
					return *this;
				}

				BBIterator& operator--()
				{
					node_iterator--;
					return this;
				}

				bool operator==(BBIterator bb)
				{
					return bb.node_iterator == node_iterator;
				}

				bool operator!=(BBIterator bb)
				{
					return bb.node_iterator != node_iterator;
				}

				BBIterator& operator++(int)
				{
					node_iterator++;
					return *this;
				}

				BBIterator& operator--(int)
				{
					node_iterator--;
					return *this;
				}

		}; /* class BBIterator */

	template <class BBType>
		class BBComparator
		{
			typedef typename DiGraph<BBType*>::GraphNode node_t;
			uint16_t _id;

			public:
			BBComparator(uint16_t id): _id(id) {};

			bool operator()(node_t *node)
			{
				return node->NodeInfo->getId() == _id;
			}
		};

//	template<class T>
//		std::ostream& operator<<(std::ostream& os, const struct _liveness_info<T>& linfo)
//		{
//			typedef typename std::set< typename _liveness_info<T>::ContentType >::const_iterator set_it;
//
//			os << "UEVarSet\n";
//			for (set_it i = linfo.UEVarSet.begin(); i != linfo.UEVarSet.end(); i++)
//			{
//				os << *i << ",";
//			}
//			os << std::endl;
//
//			os << "VarKillSet\n";
//			for (set_it i = linfo.VarKillSet.begin(); i != linfo.VarKillSet.end(); i++)
//			{
//				os << *i << ",";
//			}
//			os << std::endl;
//
//			os << "LiveOutSet\n";
//			for (set_it i = linfo.LiveOutSet.begin(); i != linfo.LiveOutSet.end(); i++)
//			{
//				os << *i << ",";
//			}
//			os << std::endl;
//
//			os << "LiveInSet\n";
//			for (set_it i = linfo.LiveInSet.begin(); i != linfo.LiveInSet.end(); i++)
//			{
//				os << *i << ",";
//			}
//			
//			return os << std::endl;
//		}

	template<class IR>
		std::ostream& operator<<(std::ostream& os, BasicBlock<IR>& BB)
		{
			
#if 0
			typedef typename DiGraph<BasicBlock<IR>*>::GraphNode node_t;
			uint32_t *position = BB.template getProperty<uint32_t *>("postOrderPosition");
			if( position )
				os << "BB ID: " << BB.Id; // << "  Post order number: " << *position;
			else
#endif
			os << "BB ID: " << BB.Id;

			return os;
		}

	template<class IR>
		BasicBlock<IR>::BasicBlock(uint16_t Id)
		: Taggable(), Id(Id), startpc(0xffffffff), endpc(0), inOrder(0), nodePtr(NULL){ //, liveness_info(NULL) {
			//reset_Liveness();
		}


	/*!
	 * \brief basic block destructor
	 *
	 * Frees the code list and the liveness info of this basic block
	 */
	template<class IR>
		BasicBlock<IR>::~BasicBlock()
		{
			for(IRStmtNodeIterator i = codeBegin(); i != codeEnd() ; i++)
			{
				delete (*i);
			}

		//	if(liveness_info)
		//		delete liveness_info;
		}

	/*!
	 * \param manager The register manager to which ask the number associated to a register
	 *
	 * <p> Frees old set if any. Performs an iteration on the list of the code and asks every
	 * node their uses and defs. Uses this information to fill the UEVarSet and VarKillSet
	 * in the liveness info struct.</p>
	 * Every definition sets a bit in the VarKillSet.<br>
	 * Every use sets a bit in the UEVarSet if not already in the VarKillSet.<br>
	 */
//	template<class IR>
//		void BasicBlock<IR>::compute_UEVarSets(/* manager_t& manager */)
//		{
//			//			reset_Liveness(manager.getCount());
//
//			for (IRStmtNodeIterator i = codeBegin(); i != codeEnd(); i++)
//			{
//				typename IR::IRNodeIterator j((*i)->nodeBegin());
//				for(; j != (*i)->nodeEnd(); j++) {
//				
//					typedef typename IR::RegType reg_t;
//					typedef typename std::set<reg_t>::iterator iterator_t;
//
//					std::set<reg_t> defs= (*j)->getDefs();
//					std::set<reg_t> uses= (*j)->getUses();
//				
//					#ifdef _DEBUG_LIVENESS
//					std::cout << "Usi di ";
//					(*j)->printNode(std::cout, true);
//					std::cout << '\n';
//					#endif
//					for(iterator_t r = uses.begin(); r != uses.end(); r++)
//					{
//						#ifdef _DEBUG_LIVENESS
//						std::cout << *r;
//						#endif
//						if(liveness_info->VarKillSet.find(*r) == liveness_info->VarKillSet.end()) {
//							liveness_info->UEVarSet.insert(*r);
//						}
//					}
//
//					#ifdef _DEBUG_LIVENESS
//					std::cout << "Definizioni di ";
//					(*j)->printNode(std::cout, true);
//					std::cout << '\n';
//					#endif
//					for(iterator_t r = defs.begin(); r != defs.end(); r++)
//					{
//						//					uint32_t index = manager[*r];
//						#ifdef _DEBUG_LIVENESS
//						std::cout << *r;
//						#endif
//						liveness_info->VarKillSet.insert(*r);
//					}
//				}
//			}
//		}

	/*!
	 * \return true if there were any changes
	 * 
	 * Performs the liveness analysis algorithm step on successors.<br>
	 * For every successor m the LiveOut the current node n is calculated as<br>
	 * LiveOut[n] = UEVar[m] U (LiveOut[m] - VarKill[m])<br>
	 * It also calculates the LiveInSet for every successor
	 */
//	template<class IR>
//		bool BasicBlock<IR>::compute_VarSets()
//		{
//			typedef typename IR::RegType reg_t;
//
//			std::set<reg_t> before;
//			before.insert(liveness_info->LiveOutSet.begin(), liveness_info->LiveOutSet.end());
//
//			for (BBIterator succ = succBegin(); succ != succEnd(); succ++)
//			{
//				Liveness_Info *succ_info = (*succ)->liveness_info;
//
//				std::set<reg_t> disjoint;
//
//				//				nbBitVector disjoint = succ_info->LiveOutSet - succ_info->VarKillSet;
//				std::set_difference(succ_info->LiveOutSet.begin(), succ_info->LiveOutSet.end(),
//						succ_info->VarKillSet.begin(), succ_info->VarKillSet.end(),
//						std::insert_iterator<std::set<reg_t> >(disjoint, disjoint.begin()));
//
//				//				succ_info->LiveInSet  = succ_info->UEVarSet + disjoint; 
//				succ_info->LiveInSet.clear();
//				std::set_union(succ_info->UEVarSet.begin(), succ_info->UEVarSet.end(),
//						disjoint.begin(), disjoint.end(),
//						std::insert_iterator<std::set<reg_t> >(succ_info->LiveInSet, succ_info->LiveInSet.begin()));
//
//				//				liveness_info->LiveOutSet += succ_info->LiveInSet;
//				liveness_info->LiveOutSet.insert(succ_info->LiveInSet.begin(),
//						succ_info->LiveInSet.end());
//			}
//
//			return before != liveness_info->LiveOutSet;
//		}

	template<class IR>
		void BasicBlock<IR>::addNewInstruction(IR *node)
		{
			//			NETVM_ASSERT(node != NULL, __FUNCTION__ "Adding a NULL Node to a BasicBlock");
			mlCode.push_back(node);	
		}

	template<class IR>
		void BasicBlock<IR>::addNewInstructionAtPosition(IR *node, typename std::list<IRNode*>::iterator iterator)
		{
			//			NETVM_ASSERT(node != NULL, __FUNCTION__ "Adding a NULL Node to a BasicBlock");
			mlCode.insert(iterator, node);	
		}

	template<class IR>
	void BasicBlock<IR>::add_copy_head(RegType source, RegType dest)
	{
		// Ask the IR to generate a copy node
		typename IR::CopyType *n(IR::make_copy(getId(), source, dest));
		
		// Scroll the list of instruction until we find the last phi
		typename std::list<IR *>::iterator i, j;
		for(i = getCode().begin(); i != getCode().end() && (*i)->isPhi(); ++i)
			;
			
		getCode().insert(i, n);
	
		return;
	}

	template<class IR>
		IR* BasicBlock<IR>::getLastStatement()
		{
			if(mlCode.size() > 0)
				return *(--codeEnd());
			else
				return NULL;
		}

	template <class IR>
	uint32_t BasicBlock<IR>::get_insn_num() const
	{
		return mlCode.size();
	}
	
	template<class IR>
		void BasicBlock<IR>::add_copy_tail(RegType source, RegType dest)
		{
			// Ask the IR to generate a copy node
			typename IR::CopyType *n(IR::make_copy(getId(), source, dest));

			// Scroll the list of instruction until we find one that is not a jump - should take one try only
			typename std::list<IR *>::iterator i, j;

			getCode().reverse(); // Consider the list from the last element to the first
			for(i = getCode().begin(); i != getCode().end() && (*i)->isJump(); ++i)
				;

			getCode().insert(i, n);

			getCode().reverse(); // Go back to the original order

			return;
		}

		template<class IR>
			struct BasicBlock<IR>::cmpGraphNodeById
			{
				uint16_t _id;
				typedef typename DiGraph<BasicBlock<IR>*>::GraphNode node_t; //!<digraph node type

				cmpGraphNodeById(uint16_t id): _id(id) {};
				bool operator()(node_t * node)
				{
					return node->NodeInfo->getId() == _id;
				}
			};

	template<class IR>
			void BasicBlock<IR>::removeFromSuccessors(uint16_t BBId)
			{
				std::list<node_t*> &successors = getSuccessors();
				successors.remove_if(cmpGraphNodeById(BBId));
			}

	template<class IR>
			void BasicBlock<IR>::removeFromPredecessors(uint16_t BBId)
			{
				std::list<node_t*> &predecessors = getPredecessors();
				predecessors.remove_if(cmpGraphNodeById(BBId));
			}

	template<class IR>
			bool BasicBlock<IR>::operator==(const BasicBlock<IR> &bb) const
			{
				return Id == bb.Id;
			}

} /* namespace jit */
#endif
