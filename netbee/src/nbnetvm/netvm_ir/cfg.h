/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file cfg.h
 * \brief This file contain the definition of the template class which implements a control flow graph
 */
#ifndef CFG_H
#define CFG_H

#include "digraph.h"
#include "bitvectorset.h"
#include "basicblock.h"
#include "taggable.h"

#include <string>
#include <list>
#include <set>
#include <map>
#include <cstddef>
#include <iostream>
#include <iterator>

namespace jit {

	template<class _T> class CFGBuilder;
	template<class _T> class CFGPrinter;
	template<class _T> class CFGEdgeSplitter;


 /*!
  * \brief this class represents a control flow graph
  *
  * A CFG is a digraph of basic blocks. It used to represent the shape of the control flow of the code
  * being compiled. It is a template since it can holds different code representations.
  * It has exactly one entry node and one exit node.
  */
	template<class IR, class _BBType = BasicBlock<IR> >
		class CFG: public DiGraph<_BBType*>
	{
		friend class CFGBuilder<IR>;
		friend class CFGPrinter<IR>;
		friend class CFGEdgeSplitter<IR>;

		protected:
		std::map< uint16_t, _BBType * > allBB; //!<map of pointers to the basic block indexed by basic block id

		std::string name; //!<name of the CFG
		_BBType *entryNode; //!<entry node of the CFG
		_BBType *exitNode; //!<exit node of the CFG
		bool SSAForm; //!< true if this CFG is in SSA form

		public:

		/*!
		 * \brief CFG constructor
		 * \param Name the name of the CFG
		 *
		 * It inizializes the entry and the exit node and puts them in the allBB map
		 */
		CFG(std::string Name);
		CFG(std::string Name, bool);

		typedef _BBType BBType;
		typedef typename DiGraph<BBType*>::GraphNode node_t; //!< type of the node of the digraph
		typedef typename DiGraph<BBType*>::GraphEdge edge_t; //!< type of the edge of the digraph
		typedef typename DiGraph<BBType*>::NodeIterator iterator_t; //!< type of an iterator on the nodes of the graph
		typedef typename std::map< uint16_t, BBType * >::iterator mapIterator; //!< type of the iterator on the allBB map
		typedef IR	IRType;
		typedef typename IRType::RegType RegType; //!< type of the register of the underlying code representation
//		typedef RegisterManager<RegType> manager_t; //!< type of the register manager for the underlying code representation

		/*!
		 * \brief get a BB from is ID
		 * \param Id the id searched
		 * \return a pointer to the BB with correspondig Id, NULL otherwise
		 */
		BBType* getBBById(uint16_t Id)
		{
			return allBB[Id];
		}

		void setBBInMap(uint16_t id, BBType *bb)  { allBB[id] = bb; };
		/*!
		 * \brief get the number of basic block
		 * \return the number of basic block in the CFG
		 */
		uint32_t BBListSize() { return allBB.size(); }

		/*!
		 * \brief get a list of the basic block in the graph
		 * \return a list cointaining all the basic blocks in the graph
		 */
		std::list<BBType* >* getBBList();

//		manager_t manager; //!< Register manager for the underlying code representations

		/*!
		 * \brief get the name of the graph
		 * \return a refence to the name of the graph
		 */
		const std::string& getName() const
		{
			return name;
		}

		/*!
		 * \brief get the entry node of the graph
		 * \return a pointer to the entry node of the graph
		 */
		node_t *getEntryNode() 
		{
			return entryNode->getNode();
		}

		/*!
		 * \brief get the exit node of the graph
		 * \return a pointer to the exit node of the graph
		 */
		node_t *getExitNode() 
		{
			return exitNode->getNode();
		}

		/*!
		 * \brief get the entry basic block of the graph
		 * \return a pointer to the basic block node of the graph
		 */
		BBType *getEntryBB() 
		{
			return entryNode;
		}

		/*!
		 * \brief get the exit basic block of the graph
		 * \return a pointer to the exit basic block of the graph
		 */
		BBType *getExitBB() 
		{
			return exitNode;
		}

		/*!
		 * \brief destructor
		 *
		 * Frees al the basic block of the graph
		 */

		~CFG();

		template <class T>
			friend	std::ostream& operator<< (std::ostream& os, CFG<T>& cfg);
		
		/*!
		 * \brief return if this CFG is in SSA Form
		 * \return true if in SSA Form
		 */
		bool isInSSAForm(){ return SSAForm; }

		/*!
		 * \brief set if this CFG is in SSA Form
		 * \param is
		 */
		void setInSSAForm(bool is) { SSAForm = is; }


		/*!
		 * \brief Removes a BasicBlock from CFG
		 * \param Id of the basicblock to remove
		 */
		void removeBasicBlock(uint32_t BBId);

		/*!
		 * \brief get the number of instruction of this cfg
		 * \return the number of instruction of this cfg
		 */
		uint32_t get_insn_num();
	};

	template<class T, class BBType>
	uint32_t CFG<T, BBType>::get_insn_num()
	{
		typedef typename std::map<uint16_t, BBType*>::iterator iterator_t;

		uint32_t res = 0;

		for(iterator_t i = allBB.begin(); i != allBB.end(); i++)
		{
			res += i->second->get_insn_num();
		}

		return res;
	}

	template <class T, class BBType>
		std::ostream& operator<< (std::ostream& os, CFG<T>& cfg)
		{

			typedef typename std::map<uint16_t, BBType*>::iterator iterator_t;

			for(iterator_t i = cfg.allBB.begin(); i != cfg.allBB.end(); i++)
				os << *i->second << std::endl;

			return os;
		}	

	template<class IR, class BBType>
		std::list<BBType*>* CFG<IR, BBType>::getBBList()
		{
			typedef typename std::map< uint16_t, BBType* >::iterator BBIterator;
			std::list< BBType* > *tempList = new std::list<BBType* >;
			for(BBIterator i=allBB.begin(); i != allBB.end(); i++)		
				tempList->push_back((*i).second);
			return tempList;
		}

	template <class IR, class BBType>
		CFG<IR, BBType>::CFG(std::string Name) : 
			name(Name), 
			entryNode (new BBType(BBType::ENTRY_BB)),
			exitNode (new BBType(BBType::EXIT_BB))
	{
          entryNode->setNode(&this->AddNode(entryNode));
          exitNode->setNode(&this->AddNode(exitNode));

		allBB[entryNode->getId()] = entryNode;
		allBB[exitNode->getId()] = exitNode;
		SSAForm = false;
	}

	template <class IR, class BBType>
		CFG<IR, BBType>::CFG(std::string Name, bool dummy) : 
			name(Name), 
			entryNode (NULL),
			exitNode (NULL)
	{
		SSAForm = false;
	}

	template <class IR, class BBType>
		void CFG<IR, BBType>::removeBasicBlock(uint32_t BBId)
		{
			//std::cout << "Removing BB " << BBId << endl;
			typedef typename BBType::node_t GraphNode;
			typedef typename std::vector<GraphNode*>::iterator gnIt;
			BBType *bb = getBBById(BBId);
			assert(bb != NULL);
			GraphNode *graphNode = bb->getNode();
			assert(graphNode != NULL);
			//gnIt i = std::find_if(this->m_NodeList.begin(), this->m_NodeList.end(), BBComparator<BBType>(BBId));
			//if (i != this->m_NodeList.end())
			allBB.erase(BBId);
			this->DeleteNode(*graphNode);
//			this->m_NodeList.erase(i);
			this->m_SortedNodes.clear();
			this->m_Sorted = false;
//			this->m_NodeCount--;
			delete bb;
//			delete graphNode;
		}


	template <class IR, class BBType>
	CFG<IR,BBType>::~CFG()
	{
		uint32_t size = this->m_NodeList.size();
			for (uint32_t i = 0; i < size; i++)
			{
				if (this->m_NodeList[i] != NULL)
					delete this->m_NodeList[i]->NodeInfo;
			}
		//std::cerr << "CFG distrutto" << std::endl;
	}


}
#endif
