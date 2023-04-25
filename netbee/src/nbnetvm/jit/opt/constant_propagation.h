/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file constant_propagation.h
 * \brief this file contains constant propagation class definition
 */
#ifndef CONSTANT_PROPAGATION_H
#define CONSTANT_PROPAGATION_H
#include "cfg.h"
#include "optimization_step.h"
#include "function_interface.h"

namespace jit{
namespace opt{

	template <class _CFG>
	typename _CFG::IRType * nodeConstantSubstitution(typename _CFG::IRType *node, 
			std::map<typename _CFG::IRType::RegType, typename _CFG::IRType::ConstType::ValueType> &map, 
			bool &changed);

	template <class _CFG>
		class ConstantPropagation : public OptimizationStep<_CFG>, nvmJitFunctionI
		{
			typedef _CFG 							CFG;
			typedef typename CFG::BBType 			BBType;
			typedef typename CFG::IRType 			IR;
			typedef typename IR::RegType 			RegType;
			typedef typename IR::ConstType 			ConstType;
			typedef typename ConstType::ValueType 	ValueType;

			struct FillConstMap;
			struct SubstituteConstant;

			public:
				ConstantPropagation(_CFG & cfg);
				void start(bool &changed);
				bool run();
#ifdef ENABLE_COMPILER_PROFILING
			public:
				static uint32_t numModifiedNodes;
				static void incModifiedNodes();
				static uint32_t getModifiedNodes();
#endif
		};
#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t ConstantPropagation<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void ConstantPropagation<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t ConstantPropagation<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif


	template<class _CFG>
		ConstantPropagation<_CFG>::ConstantPropagation(_CFG &cfg) : OptimizationStep<_CFG>(cfg) {};

	template<class _CFG>
	struct ConstantPropagation<_CFG>::FillConstMap
	{
		std::map<typename IR::RegType, typename ConstType::ValueType> &_map;

		FillConstMap(std::map<typename IR::RegType, typename ConstType::ValueType> &map): _map(map) {};

		void operator()(IR *node)
		{
			IR *tree = node->getTree();

			if(!tree)
				return;

			if(tree->isStore() && tree->getKid(0)->isConst())
			{
				_map[tree->getDefReg()] = (tree->getKid(0))->getValue();
#ifdef _DEBUG_OPTIMIZER
				tree->printNode(std::cout, true);
				std::cout << std::endl;
				std::cout << "inserisco nella mappa: " << tree->getDefReg() << " : " << tree->getKid(0)->getValue() << std::endl;
				std::cout << std::endl;
#endif
			}
		}
	};

	template <class _CFG>
	typename _CFG::IRType * nodeConstantSubstitution(typename _CFG::IRType *node, 
			std::map<typename _CFG::IRType::RegType, typename _CFG::IRType::ConstType::ValueType> &map, 
			bool &changed)
	{
		typedef _CFG CFG;
		typedef typename CFG::IRType 			IR;
		typedef typename IR::ConstType 			ConstType;
		typedef typename ConstType::ValueType 	ValueType;		
		typedef typename IR::RegType			RegType;

		typedef typename std::map<RegType, ValueType>::iterator mapIt;

		IR *newnode, *leftkid, *rightkid;

		ValueType const_value;
		RegType defreg;

		leftkid = node->getKid(0);
		rightkid = node->getKid(1);

		if(leftkid)
		{
			newnode = nodeConstantSubstitution<_CFG>(leftkid, map, changed);
			if(newnode != leftkid)
			{
				node->setKid(newnode, 0);
				delete leftkid;
				changed = true;
#ifdef ENABLE_COMPILER_PROFILING
				ConstantPropagation<_CFG>::incModifiedNodes();
#endif
			}
		}
		if(rightkid)
		{
			newnode = nodeConstantSubstitution<_CFG>(rightkid, map, changed);
			if(newnode != rightkid)
			{
				node->setKid(newnode, 1);
				delete rightkid;
				changed = true;
#ifdef ENABLE_COMPILER_PROFILING
				ConstantPropagation<_CFG>::incModifiedNodes();
#endif
			}
		}
		//here we do the job
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Constant propagation: Analizzo il nodo: " << std::endl;
			node->printNode(std::cout, true);
			std::cout << std::endl;
#endif
		if(node->isLoad())
		{
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Il nodo è una load" << std::endl;
#endif
			defreg = node->getDefReg();
			mapIt i = map.find(defreg);
			if(i != map.end() )
			{
#ifdef _DEBUG_OPTIMIZER
				std::cout << "Il nodo:" << std::endl;
				node->printNode(std::cout, true);
				std::cout << std::endl << "Carica il registro: " << defreg << " presente nella mappa" << std::endl; 
#endif
				const_value = (*i).second;
				//return new typename _CFG::ConstNode(node->belongsTo(), const_value, node->getDefReg());
				return IR::make_const(node->belongsTo(), const_value, node->getDefReg());
			}
		}
		return node;
	}

	template <class _CFG>
	struct ConstantPropagation<_CFG>::SubstituteConstant
	{
		typedef std::map<RegType, ValueType> RegValueMap;

		bool &_changed;
		RegValueMap &_map;
		SubstituteConstant(bool &changed, RegValueMap &map): _changed(changed), _map(map) {};
		void operator()(IR *node)
		{
			nodeConstantSubstitution<_CFG>(node, _map, _changed);
		}
	};

	template <class _CFG>
	void ConstantPropagation<_CFG>::start(bool &changed)
	{
		std::map<RegType, ValueType> cnst_map;	

		//fill the map
		FillConstMap fcm(cnst_map);
		visitCfgNodes(this->_cfg, &fcm);		
		//constant substitution
		SubstituteConstant sc(changed, cnst_map);
		visitCfgNodes(this->_cfg, &sc);
	}

	template <class _CFG>
	bool ConstantPropagation<_CFG>::run()
	{
		bool dummy = false;
		start(dummy);
		return dummy;
	}

} /* OPT */
} /* JIT */


#endif
