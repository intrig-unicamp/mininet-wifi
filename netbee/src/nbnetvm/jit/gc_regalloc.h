/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file gc_regalloc.h
 * \brief This file contains definition of the global register allocation algorithm
 */

#pragma once

#include "irnode.h"
#include "basicblock.h"
#include "cfg.h"
#include "cfg_liveness.h"
#include "cfg_printer.h"
#include "bitvectorset.h"

#include <list>
#include <set>
#include <map>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <cassert>

#define INVALID_COLOR 0xfffffff

namespace jit
{
	template<typename _IR>
		class IRegSpiller;
	//!this class implements the global register allocation algorithm
	template<typename _CFG>
		class GCRegAlloc {

			public:
				//forward declaration
				struct _Color;
				typedef struct _Color Color;
				struct _Register;
				typedef struct _Register Register;
				struct _GraphNode;
				typedef struct _GraphNode GraphNode;
				struct _MoveNode;
				typedef struct _MoveNode MoveNode;

				typedef _CFG CFG;
				typedef typename CFG::IRType IR;
				typedef typename IR::RegType RegType;
				typedef typename CFG::BBType BBType;
				typedef typename std::list<RegType> RegList_t;
				typedef typename std::list<GraphNode *> NodeList_t;
				typedef typename std::list<MoveNode *> MoveList_t;

				//!Node sets of the register allocation algorithm
				typedef enum
				{
					GC_NULL_SET = 0,	//USED for debuggin purposes
					GC_PRECOLORED_LST,
					GC_INITIAL_LST,
					GC_SIMPLIFY_LST,
					GC_FREEZE_LST,
					GC_SPILL_LST,
					GC_SPILLED_LST,
					GC_COALESCED_LST,
					GC_COLORED_LST,
					GC_SELECT_STCK,
					GC_NEW_TEMP
				} NodeSetsEnum;

				//!Move sets of the register allocation algorithm
				typedef enum
				{
					GC_COALESCED_MOVES_LST = 1,
					GC_CONSTRAINED_MOVES_LST,
					GC_FROZEN_MOVES_LST,
					GC_WORKLIST_MOVES_LST,
					GC_ACTIVE_MOVES_LST
				} MoveSetsEnum;



				//!constructor
				GCRegAlloc(
						CFG& cfg,
						RegList_t& virtualRegisters,
						RegList_t& machineRegisters,
						RegList_t& colors,
						IRegSpiller<_CFG>& regSpiller);

				//!destructor
				~GCRegAlloc();

				//!perform the algorithm
				bool run();

				//!return the node associated to a register
				GraphNode* getNode(const RegType& reg);
				GraphNode* getNode(const uint32_t num) const;

				//!return the color associated to a register
				std::list<std::pair<RegType, RegType> > getColors();

				//!return if this register has been used for allocation
				bool isAllocated(const RegType& reg) const;

				uint32_t getNumSpilledReg() const;

			private:

				//useful type

				typedef typename NodeList_t::iterator node_iterator_t;
				typedef typename NodeList_t::const_iterator const_node_iterator_t;

				typedef typename MoveList_t::iterator move_iterator_t;
				typedef typename MoveList_t::const_iterator const_move_iterator_t;
				/*!
				 * \brief print the node and move lists
				 * \param os the stream to print on
				 */

				//utils to perform operation on lists
				//!create a new node
				GraphNode* new_node(RegType& reg, NodeSetsEnum list, Color* color = NULL, bool precolored = false);

				//!create a new move_node
				MoveNode* new_move_node(IR* insn, BBType* bb);

				//!returns true if register allocation seems right
				bool test_consistency();

				//!insert a node in a list
				void insert_in_list(NodeSetsEnum list, GraphNode *node);

				//!insert a move node in a list
				void insert_in_list(MoveSetsEnum list, MoveNode *node);

				//!return adjacents of a node
				nbBitVector adjacent(GraphNode *node);

				//!decrements degree of a node
				void decrement_degree(GraphNode *node);

				//!enables a node moves
				void enable_moves(nbBitVector& nodes);
				void enable_moves(GraphNode* n);

				//!add an edge between to nodes
				void add_edge(const RegType& u, const RegType& v);
				void add_edge(GraphNode* u_node, GraphNode* v_node);

				//!return the moves associoted with a node
				MoveList_t node_moves(GraphNode *node);

				void freeze_moves(GraphNode* u);

				//!is a node move related
				bool move_related(GraphNode* node);
				//!add a node to a worklist
				void add_work_list(GraphNode* node);

				bool ok(GraphNode* t, GraphNode* r);

				bool conservative(nbBitVector& nodes);

				bool coalesce_test(GraphNode* u, GraphNode* v);

				void combine(GraphNode *u, GraphNode *v);

				GraphNode* select_spill_node();

				GraphNode* get_alias(GraphNode *node) const;

				void doSpill();

				//utils to dump information
				//!dump all the lists
				void dumpSets(std::ostream& os) const;
				//!dump a set of GraphNode
				void dumpSet(std::ostream& os, const NodeList_t& list) const;
				//!dump a set of MoveNode
				void dumpSet(std::ostream& os, const MoveList_t& list) const;

				void dumpSet(std::ostream& os, const nbBitVector& bv) const;
				//!dump interference graph
				void dumpInterferenceGraph(std::ostream& os) const;
				void dumpInterference(std::ostream& os, const GraphNode* n) const;

				uint32_t maxdegree(void) const;

				void init_bv();
				//algorithm steps
				//!build the interference graph
				void build();
				//!build the worklist to start the algorithm
				void make_worklist();

				void simplify();

				void coalesce();

				void freeze();

				void selectSpill();

				void assignColors();

				void rebuildLists();

				std::vector<Color* > colors; 					//!<colors
				std::map<RegType, GraphNode *> allnodes;		//!<mapping from registers to node
				std::vector< GraphNode* >  nodes_numbers; 	    //!<mapping from registers to numbers
				uint32_t numRegisters;							//!<total number of registers
				int numColors; 				 					//!<total number of colors
				int K;											//!<number of avaible colors
				CFG& cfg;				 					//!<the cfg we are working on
				IRegSpiller<_CFG>& _regSpiller;
				std::vector< MoveList_t* > all_move_lists;   	//!collection of move lists used by the algorithm
				std::vector< nbBitVector* > all_nodes_bv;

				//Nodes bitvectors
				nbBitVector* precolored;
				nbBitVector* initial;
				nbBitVector* simplifyworklist;
				nbBitVector* freezeworklist;
				nbBitVector* spillworklist;
				nbBitVector* spillednodes;
				nbBitVector* coalescednodes;
				nbBitVector* colorednodes;
				nbBitVector* selectstack;
				nbBitVector* newtemp;

				NodeList_t SelectStack;

				//Move lists
				MoveList_t CoalescedMoves;	//!<List of move instructions that have been coalesced
				MoveList_t ConstrainedMoves;	//!<List of move instructions whose source and target interfere
				MoveList_t FrozenMoves;		//!<List of move instructions that will no longer considered for coalescing
				MoveList_t WorkListMoves;		//!<List of move instructions enabled for possible coalescing
				MoveList_t ActiveMoves;		//!<List of move instructions not ready for coalescing

				//Spill profiling
				uint32_t total_spill_cost;
				uint32_t total_reg_spilled;

			public:
				//!color representation
				struct _Color
				{
					uint32_t id; //!<color id
					RegType& reg;  //!<the register associated with this color
					bool allocated; //!<has this color been used

					//!constructor
					_Color(uint32_t id, RegType& reg) : id(id), reg(reg), allocated(false) {}
				};

				//!a node in the graph of the register allocation
				struct _GraphNode
				{
					Register *reg; //!<the register
					GraphNode *alias; //!<the alias if the register is coalesced
					nbBitVector* adjSet; //!<sets of the adjacent nodes of this
					MoveList_t moveList; //!<list of the move instruction of this node
					NodeSetsEnum belongsToSet; //!set this nodes belongs to
					Color *color;
					uint16_t degree;
					bool spilled;
					uint32_t num;
					uint32_t cost;

					//!constructor
					_GraphNode(Register* reg);
					//!destructor
					~_GraphNode();

					//!get this node degree
					uint16_t get_degree() const;

					void dump(std::ostream& os) const;
					//void dumpInterference(std::ostream& os) const;
					bool has_interference_with(GraphNode* node) const;
				};

				//!a register representation
				struct _Register
				{
					RegType& reg; //!<the register
					Color *color; //!<the register color
					bool precolored; //!<is the register precolored

					//!constructor
					_Register(RegType& reg, Color* color = NULL, bool precolored = false);
					~_Register();

					void dump(std::ostream& os) const;
				};

				//!A move instruction
				struct _MoveNode
				{
					IR* MoveInstruction; //!<Pointer to the instruction
					GraphNode *from;     //!<From which node
					GraphNode *to;       //!<To which node
					BBType *BB;  //!<basic block this move belongs to
					MoveSetsEnum belongsToSet; //!<Move set of this move

					_MoveNode(IR* insn, GraphNode* from, GraphNode* to, BBType* BB);
					void dump(std::ostream& os) const;
				};

				struct MoveNodeLess
				{
					bool operator()(const MoveNode *x, const MoveNode *y)
					{
						return x < y;
					}
				};

				struct MoveNodeEqu
				{
					bool operator()(const MoveNode *x, const MoveNode *y)
					{
						return x == y;
					}
				};

				struct node_moves_remove_pred
				{
					bool operator()(MoveNode* m);
				};

				struct enable_moves_remove_pred
				{
					bool operator()(MoveNode* m);
				};
		};

	/*!
	 * \brief Interface required to use a backend object to spill registers
	 */
	template<typename _CFG>
	class IRegSpiller
	{
		public:
			typedef _CFG CFG;
			typedef typename CFG::IRType IR;
			typedef typename IR::RegType RegType; //!<register type
			typedef typename GCRegAlloc<_CFG>::GraphNode GraphNode; //!<graphnode type
			/*!
			 * \brief spill a set of register
			 * \param tospillRegs set of registers to spill
			 * \return the set of new temporaries created
			 *
			 * before each use insert a load from spill registers area.<br>
			 * after each definition insert a store to the spill registers area.
			 */
			virtual std::set<RegType> spillRegisters(std::set<RegType>& tospillRegs) = 0;
			/*!
			 * \brief select a node to spill from a set of high degree node
			 * \param reglist set of register to spill
			 */
			virtual GraphNode* selectSpillNode(std::list<GraphNode*>& reglist);

			//!return the numbers of registers spilled
			virtual uint32_t getNumSpilledReg() const = 0;

			//!return the total cost of spill operation
			virtual uint32_t getTotalSpillCost() const = 0;

			//!is this register a new temporary register
			virtual bool isSpilled(const RegType& reg) = 0;
			//!destructor
			virtual ~IRegSpiller();
	};
}

template<typename _CFG>
jit::GCRegAlloc<_CFG>::_MoveNode::_MoveNode(
		typename _CFG::IRType* insn,
		GraphNode* from,
		GraphNode* to,
		typename _CFG::BBType* BB)
:
	MoveInstruction(insn),
	from(from),
	to(to),
	BB(BB)
	{}

	template<typename _CFG>
inline typename jit::GCRegAlloc<_CFG>::GraphNode*
jit::GCRegAlloc<_CFG>::getNode(const typename _CFG::IRType::RegType& reg)
{
	return allnodes[reg];
}

	template<typename _CFG>
inline typename jit::GCRegAlloc<_CFG>::GraphNode*
jit::GCRegAlloc<_CFG>::getNode(const uint32_t num) const
{
	return nodes_numbers[num];
}

template<typename _CFG>
uint32_t jit::GCRegAlloc<_CFG>::getNumSpilledReg() const
{
	return _regSpiller.getNumSpilledReg();
}

template<typename _CFG>
bool jit::GCRegAlloc<_CFG>::isAllocated(const RegType& reg) const
{
	typedef typename std::vector<Color* >::const_iterator iterator_t;
	for(iterator_t i = colors.begin(); i != colors.end(); i++)
	{
		if((*i)->reg == reg)
			return (*i)->allocated;
	}
	return false;
}

	template<typename _CFG>
	std::list<std::pair<typename _CFG::IRType::RegType, typename _CFG::IRType::RegType> >
jit::GCRegAlloc<_CFG>::getColors()
{
	typedef typename std::map<RegType, GraphNode *>::iterator map_iterator_t;
	typedef std::list<std::pair<RegType, RegType> > list_t;

	list_t retlist;
	for(map_iterator_t i = allnodes.begin(); i != allnodes.end(); i++)
	{
		assert((*i).second->color != NULL || (*i).second->spilled);

		if((*i).second->color != NULL)
			retlist.push_back(std::pair<RegType, RegType>((*i).second->reg->reg, (*i).second->color->reg));
	}
	return retlist;
}

template<typename _CFG>
inline bool jit::GCRegAlloc<_CFG>::GraphNode::has_interference_with(GraphNode* node) const
{
	assert(node != NULL);
	return adjSet->Test(node->num);
}

template<typename _CFG>
void jit::GCRegAlloc<_CFG>::dumpInterference(std::ostream& os, const GraphNode* n) const
{
	for(nbBitVector::iterator i = n->adjSet->begin(); i != n->adjSet->end(); i++)
	{
		nodes_numbers[(*i)]->dump(os);
		os << " ";
	}
}


template<typename _CFG>
uint32_t jit::GCRegAlloc<_CFG>::maxdegree(void) const
{
	uint32_t maxDegree = 0;

	typedef typename std::map<RegType, GraphNode *>::const_iterator map_iterator_t;


	for(map_iterator_t i = allnodes.begin(); i != allnodes.end(); i++)
	{
		GraphNode *n = i->second;
		uint32_t count = 0;
		for(nbBitVector::iterator i = n->adjSet->begin(); i != n->adjSet->end(); i++, count++);
		if (count > maxDegree)
			maxDegree = count;
	}

	return maxDegree;

}


template<typename _CFG>
void jit::GCRegAlloc<_CFG>::_MoveNode::dump(std::ostream& os) const
{
	os << "(" << from->reg->reg << " -> " << to->reg->reg << ") ";
}

template<typename _CFG>
void jit::GCRegAlloc<_CFG>::_GraphNode::dump(std::ostream& os) const
{
	reg->dump(os);
	os << "(" << (color ? color->id : 0xffff)  << ") ";
}

template<typename _CFG>
void jit::GCRegAlloc<_CFG>::_Register::dump(std::ostream& os) const
{
	os << reg;
}

template<typename _CFG>
bool jit::GCRegAlloc<_CFG>::test_consistency()
{
	typedef typename std::map<RegType, GraphNode *>::iterator map_iterator_t;
	for(map_iterator_t n_it = allnodes.begin(); n_it != allnodes.end(); n_it++)
	{
		GraphNode* n = n_it->second;

		for(nbBitVector::iterator t_it = n->adjSet->begin(); t_it != n->adjSet->end(); t_it++)
		{
			GraphNode* t = getNode(*t_it);
			assert(n->color != t->color);
			if(n->color == t->color)
				return false;
		}

	}

	return true;
}

template<typename _CFG>
void jit::GCRegAlloc<_CFG>::rebuildLists()
{
	typedef typename std::map<RegType, GraphNode *>::iterator map_it_t;

	init_bv();

	for(map_it_t m = allnodes.begin(); m != allnodes.end(); m++)
	{
		GraphNode *node = m->second;
		node->alias = NULL;
		node->degree = 0;
		if(!node->reg->precolored)
			node->reg->color = NULL;

		if(node->adjSet)
			delete node->adjSet;
		node->moveList.clear();

		switch(node->belongsToSet)
		{
			case GC_NEW_TEMP:
			case GC_COALESCED_LST:
			case GC_COLORED_LST:
				insert_in_list(GC_INITIAL_LST, node);
				break;
			case GC_PRECOLORED_LST:
				insert_in_list(GC_PRECOLORED_LST, node);
				break;
			case GC_SPILLED_LST:
				//do nothing
				break;
			default:
			assert(1 == 0 && "strange node");
		}
	}

	SelectStack.clear();
	ConstrainedMoves.clear();
	FrozenMoves.clear();
	WorkListMoves.clear();
	ActiveMoves.clear();

#ifdef _DEBUG_SPILL
	std::cout << "Free lists effettuata. Liste vuote" << std::endl;
#endif
}

	template<typename _CFG>
void jit::GCRegAlloc<_CFG>::doSpill()
{
	typedef typename std::set<RegType>::iterator reg_set_it_t;
	std::set<RegType> spillRegSet, new_regs;
	for(nbBitVector::iterator i = spillednodes->begin(); i != spillednodes->end(); i++)
	{
		GraphNode* node = getNode(*i);
		node->spilled = true;
		RegType tempreg = node->reg->reg;
		spillRegSet.insert(tempreg);
		total_spill_cost += node->cost;
		total_reg_spilled++;
	}
	new_regs = _regSpiller.spillRegisters(spillRegSet);

	for(reg_set_it_t i = new_regs.begin(); i != new_regs.end(); i++)
	{
		RegType *tmp = new RegType(*i);
		new_node(*tmp, GC_NEW_TEMP);
	}

	rebuildLists();

#ifdef _DEBUG_SPILL
	typedef typename std::map<RegType, GraphNode *>::iterator all_nodes_it;
	std::cout << "Nodi presenti nella mappa: " << std::endl;
	for(all_nodes_it i = allnodes.begin(); i != allnodes.end(); i++)
	{
		std::cout << (*i).second->reg->reg << ", ";
	}
	std::cout << std::endl;
#endif
	//free lists
}


	template<typename _CFG>
bool jit::GCRegAlloc<_CFG>::run()
{
	bool spilled = false;

#ifdef _DEBUG_REG_ALLOC
	uint32_t build_count = 0;
	uint32_t round_count = 0;
#endif

	do
	{
		spilled = false;


#ifdef _DEBUG_REG_ALLOC
		round_count = 0;
		std::cout << "Going to Build: " << ++build_count<< std::endl;
#endif
		build();
		make_worklist();

		do
		{
			if(!simplifyworklist->empty())
			{
#ifdef _DEBUG_REG_ALLOC
				std::cout << "Going to Simplify" << std::endl;
#endif
				simplify();
			}
			else if (!WorkListMoves.empty())
			{
#ifdef _DEBUG_REG_ALLOC
				std::cout << "Going to Coalesce" << std::endl;
#endif
				coalesce();
			}
			else if (!freezeworklist->empty())
			{
#ifdef _DEBUG_REG_ALLOC
				std::cout << "Going to Freeze" << std::endl;
#endif
				freeze();
			}
			else if (!spillworklist->empty())
			{
#ifdef _DEBUG_REG_ALLOC
				std::cout << "Going to Select Spills" << std::endl;
#endif
				selectSpill();
			}

#ifdef _DEBUG_REG_ALLOC
			if (!simplifyworklist->empty())
				std::cout << "Simplify Worklist Not Empty" << std::endl;
			if (!WorkListMoves.empty())
				std::cout << "Moves Worklist Not Empty" << std::endl;
			if (!freezeworklist->empty())
				std::cout << "Freeze Worklist Not Empty" << std::endl;
			if (!spillworklist->empty())
				std::cout << "Spill Worklist Not Empty" << std::endl;

			std::cout << "round_count " << ++round_count << " done...";
			std::cout << " Max Degree = " << maxdegree() << std::endl << std::endl;
#endif

		}
		while (!simplifyworklist->empty() || !WorkListMoves.empty()
				|| !freezeworklist->empty() || !spillworklist->empty());

#ifdef _DEBUG_REG_ALLOC
		std::cout << "Going to Assign Colors" << std::endl;
#endif
		assignColors();

#ifdef _DEBUG_REG_ALLOC
		std::cout << "\n================================================\nDopo assing_colors()\n";
		dumpSets(std::cout);
		dumpInterferenceGraph(std::cout);
#endif

		if(!spillednodes->empty())
		{
			spilled = true;
#ifdef _DEBUG_REG_ALLOC
			std::cout << "Going to do actual spilling" << std::endl <<std::endl;
#endif
			doSpill();
		}
	}
	while(spilled);

	if(!test_consistency())
	{
		return false;
	}

#ifdef _DEBUG_SPILL
	std::cout << "*************************************************" << std::endl;
	std::cout << "*************************************************" << std::endl;
	std::cout << "register spilled: " << total_reg_spilled << std::endl;
	std::cout << "total cost: " << total_spill_cost << std::endl;
	std::cout << "*************************************************" << std::endl;
	std::cout << "*************************************************" << std::endl;
#endif
	return true;
}

template<typename _CFG>
void jit::GCRegAlloc<_CFG>::assignColors()
{
	assert(simplifyworklist->empty() && WorkListMoves.empty()
			&& freezeworklist->empty() && spillworklist->empty());

	while(!SelectStack.empty())
	{
		GraphNode *n = SelectStack.back();
		SelectStack.pop_back();

		std::vector<bool> okColors(numColors, true);

		#ifdef _DEBUG_REG_ALLOC
		std::cout << "assigning color to ";  n->dump(std::cout); std::cout << std::endl;
		#endif
		for(nbBitVector::iterator w = n->adjSet->begin(); w != n->adjSet->end(); w++)
		{
			GraphNode *a = get_alias(nodes_numbers[*w]);

			assert(a->belongsToSet != GC_COALESCED_LST);

			if(a->belongsToSet == GC_COLORED_LST || a->belongsToSet == GC_PRECOLORED_LST)
			{
				assert(a->color != NULL);
		#ifdef _DEBUG_REG_ALLOC
				std::cout << "no color " << a->color->id << "for "; n->dump(std::cout); std::cout << std::endl;
		#endif
				okColors[a->color->id] = false;
			}
		}

		//find an unused color
		int i;
		for(i = 0; i < K && !okColors[i]; i++);

		if(i < K)
		{
			insert_in_list(GC_COLORED_LST, n);
			n->color = colors[i];
			colors[i]->allocated = true;
		#ifdef _DEBUG_REG_ALLOC
			std::cout << "assigned color " << i << "to "; n->dump(std::cout); std::cout << std::endl;
		#endif
		}
		else
		{
			insert_in_list(GC_SPILLED_LST, n);
		}
	}

	for(nbBitVector::iterator n = coalescednodes->begin(); n != coalescednodes->end(); n++)
	{
		GraphNode *node = getNode(*n);
		(node)->color = get_alias(node)->color;
	}
}

	template <typename _CFG>
void jit::GCRegAlloc<_CFG>::simplify()
{
	assert(!simplifyworklist->empty());

	uint32_t first = *simplifyworklist->begin();
	GraphNode* n = getNode(first);
	simplifyworklist->Reset(first);

	insert_in_list(GC_SELECT_STCK, n);

	nbBitVector adj(adjacent(n));
	for(nbBitVector::iterator i = adj.begin(); i != adj.end(); i++)
	{
		decrement_degree(getNode(*i));
	}
#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo simplify()\n";
	dumpSets(std::cout);
	dumpInterferenceGraph(std::cout);
#endif
}
	template <typename _CFG>
void jit::GCRegAlloc<_CFG>::coalesce()
{
	assert(!WorkListMoves.empty());

	MoveNode* m = WorkListMoves.front(); //TODO select a good move
	WorkListMoves.pop_front();

	GraphNode *x = get_alias(m->from);
	GraphNode *y = get_alias(m->to);

	GraphNode *u, *v;

	if(y->belongsToSet == GC_PRECOLORED_LST)
	{
		u = y;
		v = x;
	}
	else
	{
		u = x;
		v = y;
	}

	if(u == v)
	{
		insert_in_list(GC_COALESCED_MOVES_LST, m);
		add_work_list(u);
	}
	else if (v->belongsToSet == GC_PRECOLORED_LST ||
			 u->has_interference_with(v) || v->has_interference_with(u))
	{
		insert_in_list(GC_CONSTRAINED_MOVES_LST, m);
		add_work_list(u);
		add_work_list(v);
	}
	else
	{
		//nbBitVector adjacents(adjacent(v));
		//nbBitVector u_adjacents(adjacent(u));


		nbBitVector result_set = adjacent(v) + adjacent(u);;

		if( (u->belongsToSet == GC_PRECOLORED_LST && coalesce_test(u, v)) ||
			 (u->belongsToSet != GC_PRECOLORED_LST && conservative(result_set)))
			 {
			 	insert_in_list(GC_COALESCED_MOVES_LST, m);
				combine(u, v);
				add_work_list(u);
			 }
		else
		{
			insert_in_list(GC_ACTIVE_MOVES_LST, m);
		}

	}

#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo coalesce()\n";
	dumpSets(std::cout);
	dumpInterferenceGraph(std::cout);
#endif

}
	template <typename _CFG>
void jit::GCRegAlloc<_CFG>::freeze()
{
	assert(!freezeworklist->empty());

	uint32_t first = *freezeworklist->begin();
	GraphNode* u = getNode(first);
	freezeworklist->Reset(first);

	insert_in_list(GC_SIMPLIFY_LST, u);
	freeze_moves(u);

#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo freeze()\n";
	dumpSets(std::cout);
	dumpInterferenceGraph(std::cout);
#endif
}

template<typename _CFG>
typename jit::GCRegAlloc<_CFG>::GraphNode*
jit::GCRegAlloc<_CFG>::select_spill_node()
{
	typedef std::list<GraphNode*> list_t;
	assert(!spillworklist->empty());

	list_t tmplist;

	for(nbBitVector::iterator it = spillworklist->begin(); it != spillworklist->end(); it++)
		tmplist.push_back(getNode(*it));


	return _regSpiller.selectSpillNode(tmplist);
}

	template <typename _CFG>
void jit::GCRegAlloc<_CFG>::selectSpill()
{
	assert(!spillworklist->empty());
	GraphNode* m = select_spill_node();

	spillworklist->Reset(m->num);
	insert_in_list(GC_SIMPLIFY_LST, m);

	freeze_moves(m);

#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo select_spill()\n";
	dumpSets(std::cout);
	dumpInterferenceGraph(std::cout);
#endif
}

	template<typename _CFG>
void jit::GCRegAlloc<_CFG>::build()
{
	typedef typename std::list< BBType *>::iterator bb_iterator_t;
	typedef typename std::list< IR* >::reverse_iterator rinsn_iterator_t;

	//typedef typename BasicBlock<_IR>::Liveness_Info liveness_info_t;
	typedef typename std::set< RegType > liveness_set_t;
	typedef typename liveness_set_t::iterator reg_iterator_t;

	std::list< BBType* >* bbList = cfg.getBBList();
	jit::Liveness<_CFG> liveness(cfg);
	liveness.run();

	//initialize bit_vectors
	uint32_t size = nodes_numbers.size();
	for(uint32_t i = 0; i < size; i++)
	{
		nodes_numbers[i]->adjSet = new nbBitVector(size);
	}

	//for every basic block
	for(bb_iterator_t bb_it = bbList->begin(); bb_it != bbList->end(); bb_it++)
	{
		BBType* bb = *bb_it;
		uint16_t *loop_level = bb->template getProperty<uint16_t*>("loopLevel");
		assert(loop_level != NULL);
#ifdef _DEBUG_SPILL
		std::cout << "Il Loop Level di " << *bb << " è: " << *loop_level << std::endl;
#endif

		//live = liveout(bb);
		liveness_set_t live(liveness.get_LiveOutSet(bb));
		liveness_set_t livetmp;
		std::list<IR*>& code = bb->getCode();
#ifdef _DEBUG_REG_ALLOC
		std::cout << "BB: " << *bb << std::endl;
		for(typename liveness_set_t::iterator i = live.begin(); i != live.end(); i++)
		{
			std::cout << "Live out: " << *i << std::endl;
		}
#endif

		//for every instruction in reverse order
		for(rinsn_iterator_t i = code.rbegin(); i != code.rend(); i++)
		{
			IR* insn = *i;
			liveness_set_t uses = insn->getUses();
			liveness_set_t defs = insn->getDefs();

			for(reg_iterator_t r = uses.begin(); r != uses.end(); r++)
			{
				GraphNode* node = getNode(*r);
				if (node->belongsToSet == GC_PRECOLORED_LST)
					node->color->allocated = true;
			}

			for(reg_iterator_t r = defs.begin(); r != defs.end(); r++)
			{
				GraphNode* node = getNode(*r);
				if (node->belongsToSet == GC_PRECOLORED_LST)
					node->color->allocated = true;
			}

#ifdef _DEBUG_REG_ALLOC
			std::cout << "\nuses of " << (*insn) << std::endl;
			for(reg_iterator_t r = uses.begin(); r != uses.end(); r++)
			{
				std::cout << *r << " ";
			}

			std::cout << "\ndefs of " << (*insn) << std::endl;
			for(reg_iterator_t r = defs.begin(); r != defs.end(); r++)
			{
				std::cout << *r << " ";
			}
#endif
			//here we compute the spilling cost
			for(reg_iterator_t r = uses.begin(); r != uses.end(); r++)
			{
				GraphNode *tmp_node = getNode(*r);
				tmp_node->cost += 10^(*loop_level);
			}

			for(reg_iterator_t r = defs.begin(); r != defs.end(); r++)
			{
				GraphNode *tmp_node = getNode(*r);
				tmp_node->cost += 10^(*loop_level);
			}

			// if insn is a move instruction
			if(insn->isCopy())
			{
				//live = live - uses
				livetmp.clear();
				std::set_difference(
						live.begin(), live.end(),
						uses.begin(), uses.end(),
						std::insert_iterator<liveness_set_t>(livetmp, livetmp.begin())
						);
				live = livetmp;

				//create a new move
				MoveNode* move = new_move_node(insn, bb);
				//insert it in WORKLIST_MOVES
				insert_in_list(GC_WORKLIST_MOVES_LST, move);

				//add the move to the set of every register used or defined
				for(reg_iterator_t r = uses.begin(); r != uses.end(); r++)
				{
					GraphNode* node = getNode(*r);
					node->moveList.push_back(move);
				}

				for(reg_iterator_t r = defs.begin(); r != defs.end(); r++)
				{
					GraphNode* node = getNode(*r);
					node->moveList.push_back(move);
				}
			}

			//live = live U defs
			livetmp.clear();
			std::set_union(
					live.begin(), live.end(),
					defs.begin(), defs.end(),
					std::insert_iterator<liveness_set_t>(livetmp, livetmp.begin())
					);
			live = livetmp;

#ifdef _DEBUG_REG_ALLOC
			std::cout << "Live Set: " << std::endl;
			for (reg_iterator_t r = live.begin(); r != live.end(); r++)
			{
				std::cout << *r << " " << std::endl;
			}
#endif


			//for all registers defined
			for(reg_iterator_t d = defs.begin(); d != defs.end(); d++)
			{
				//for all registers live
				for(reg_iterator_t l = live.begin(); l != live.end(); l++)
				{
#ifdef _DEBUG_REG_ALLOC
					std::cout << "Adding interference between " << *d << " and " << *l << std::endl;
#endif
					//add the interference
					add_edge(*d, *l);
				}
			}

			//live = uses U (live - defs)
			livetmp.clear();
			std::set_difference(
					live.begin(), live.end(),
					defs.begin(), defs.end(),
					std::insert_iterator<liveness_set_t>(livetmp, livetmp.begin())
					);
			live = livetmp;

			livetmp.clear();
			std::set_union(
					live.begin(), live.end(),
					uses.begin(), uses.end(),
					std::insert_iterator<liveness_set_t>(livetmp, livetmp.begin())
					);
			live = livetmp;


		} //for on instruction
	} //for on bb

#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo build()\n";
	dumpInterferenceGraph(std::cout);
	//dumpInterferenceGraph(std::cout);
#endif
	delete bbList;
}

	template<typename _CFG>
void jit::GCRegAlloc<_CFG>::make_worklist()
{
	//assert(!Initial.empty());

	while(!initial->empty())
	{
		uint32_t first = *initial->begin();
		GraphNode* node = getNode(first);
		initial->Reset(first);

		if(node->get_degree() >= K)
		{
			insert_in_list(GC_SPILL_LST, node);
		}
		else if (move_related(node))
		{
			insert_in_list(GC_FREEZE_LST, node);
		}
		else
		{
			insert_in_list(GC_SIMPLIFY_LST, node);
		}
	}

#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo make_worklist()\n";
	dumpSets(std::cout);
	dumpInterferenceGraph(std::cout);
#endif
}

template<typename _CFG>
void jit::GCRegAlloc<_CFG>::add_edge(GraphNode* u_node, GraphNode* v_node)
{
	assert(u_node != NULL && v_node != NULL);

	if(u_node != v_node)
	{
		if(!u_node->reg->precolored && !u_node->has_interference_with(v_node))
		{
#ifdef _DEBUG_REG_ALLOC
			std::cout << "adding an edge between " << u_node->reg->reg << " and " << v_node->reg->reg << std::endl;
#endif
			u_node->adjSet->Set(v_node->num);
			u_node->degree++;
		}

		if(!v_node->reg->precolored && !v_node->has_interference_with(u_node))
		{
#ifdef _DEBUG_REG_ALLOC
					std::cout << "adding an edge between " << u_node->reg->reg << " and " << v_node->reg->reg << std::endl;
#endif
			v_node->adjSet->Set(u_node->num);
			v_node->degree++;
		}
	}
}

	template<typename _CFG>
void jit::GCRegAlloc<_CFG>::add_edge(const typename _CFG::IRType::RegType& u, const typename _CFG::IRType::RegType& v)
{
	GraphNode *u_node = allnodes[u];
	GraphNode *v_node = allnodes[v];

	add_edge(u_node, v_node);
}

template<typename _CFG>
nbBitVector
jit::GCRegAlloc<_CFG>::adjacent(_GraphNode *node)
{
	assert(node != NULL);
	nbBitVector res = *node->adjSet - *coalescednodes - *selectstack;
	return res;
}

	template<typename _CFG>
void jit::GCRegAlloc<_CFG>::insert_in_list(NodeSetsEnum list, struct _GraphNode* node)
{
	assert(node != NULL && list != 0);

	nbBitVector* bv = all_nodes_bv[list];

	bv->Set(node->num);
	node->belongsToSet = list;

	if(list == GC_SELECT_STCK)
		SelectStack.push_back(node);

//	NodeList_t *l = all_nodes_lists[list];
//	assert(l != NULL);
//
//	l->push_back(node);
}

	template<typename _CFG>
void jit::GCRegAlloc<_CFG>::insert_in_list(MoveSetsEnum list, struct _MoveNode* node)
{
	assert(node != NULL && list != 0);

	MoveList_t *l = all_move_lists[list];
	assert(l != NULL);

	l->push_back(node);
	node->belongsToSet = list;
}

template<typename _CFG>
	typename jit::GCRegAlloc<_CFG>::GraphNode*
jit::GCRegAlloc<_CFG>::new_node(typename _CFG::IRType::RegType& reg, NodeSetsEnum list, Color* color, bool precolored)
{

	GraphNode* new_node = new GraphNode(new Register(reg, color, precolored));
	new_node->num = numRegisters;
	allnodes[reg] = new_node;
	nodes_numbers.push_back(new_node);
	numRegisters++;

	new_node->belongsToSet = list;
#ifdef _DEBUG_REG_ALLOC
	std::cout << "Added node: ";
	new_node->dump(std::cout);
	std::cout << std::endl;
#endif

	return new_node;
}

template<typename _CFG>
typename jit::GCRegAlloc<_CFG>::MoveNode*
jit::GCRegAlloc<_CFG>::new_move_node(typename _CFG::IRType* insn, typename _CFG::BBType* bb)
{
	GraphNode *from, *to;
	from = getNode(insn->get_from());
	to = getNode(insn->get_to());
//#ifdef _DEBUG_SPILL
//	std::cout << "Creazione move node: " << std::endl;
//	std::cout << "FROM: " << from->reg->reg << std::endl;
//	std::cout << "TO:" << to->reg->reg << std::endl;
//#endif
	return new MoveNode(insn, from, to, bb);
}

/*!
 * \param virtualRegister list of virtual register of the code
 * \param machineRegister list of machine register precolored but not avaible colors
 * \param colors list of machine register which are avaible colors
 */
template<typename _CFG>
jit::GCRegAlloc<_CFG>::GCRegAlloc (
		_CFG& cfg,
		std::list<typename _CFG::IRType::RegType>& virtualRegisters,
		std::list<typename _CFG::IRType::RegType>& machineRegisters,
		std::list<typename _CFG::IRType::RegType>& colors,
		IRegSpiller<_CFG>& regSpiller)
: numRegisters(0), cfg(cfg), _regSpiller(regSpiller), all_nodes_bv(11,(nbBitVector*)0), total_spill_cost(0), total_reg_spilled(0)
{
	typedef typename std::list< typename _CFG::IRType::RegType >::iterator iterator_t;

	all_move_lists.push_back(NULL);
	all_move_lists.push_back(&CoalescedMoves);
	all_move_lists.push_back(&ConstrainedMoves);
	all_move_lists.push_back(&FrozenMoves);
	all_move_lists.push_back(&WorkListMoves);
	all_move_lists.push_back(&ActiveMoves);

	uint32_t id = 0;
	Color* new_color = NULL;

	for(iterator_t i = colors.begin(); i != colors.end(); i++)
	{
		new_color = new Color(id, *i);
		this->colors.push_back(new_color);

		new_node(*i, GC_PRECOLORED_LST, new_color, true);
		id++;
	}

	K = id;

	//initialize colors and push them only in the precolored list
	for(iterator_t i = machineRegisters.begin(); i != machineRegisters.end(); i++)
	{
		new_color = new Color(id, *i);
		this->colors.push_back(new_color);
		new_node(*i, GC_PRECOLORED_LST, new_color, true);
		id++;
	}

	numColors = id;

	//initialize virtual register and put them in the initial list
	for(iterator_t i = virtualRegisters.begin(); i != virtualRegisters.end(); i++)
	{
		new_node(*i, GC_INITIAL_LST);
	}

	//initialize bit vectors
	init_bv();

	//put nodes into bitvectors
	typename std::vector<GraphNode *>::iterator node_it;
	for(node_it = nodes_numbers.begin(); node_it != nodes_numbers.end(); node_it++)
	{
		GraphNode *n = *node_it;
		insert_in_list(n->belongsToSet, n);
	}

#ifdef _DEBUG_REG_ALLOC
	std::cout << "\n================================================\nDopo construttore di GCRegAlloc";
	dumpSets(std::cout);
#endif
}

	template<typename _IR>
void jit::GCRegAlloc<_IR>::init_bv()
{
	for(uint32_t i = 0; i < all_nodes_bv.size(); i++)
	{
		if(all_nodes_bv[i])
			delete all_nodes_bv[i];
		all_nodes_bv[i] = new nbBitVector(numRegisters);
	}

	precolored 		 = all_nodes_bv[GC_PRECOLORED_LST];
	initial 		 = all_nodes_bv[GC_INITIAL_LST];
	simplifyworklist = all_nodes_bv[GC_SIMPLIFY_LST];
	freezeworklist 	 = all_nodes_bv[GC_FREEZE_LST];
	spillworklist 	 = all_nodes_bv[GC_SPILL_LST];
	spillednodes 	 = all_nodes_bv[GC_SPILLED_LST];
	coalescednodes 	 = all_nodes_bv[GC_COALESCED_LST];
	colorednodes 	 = all_nodes_bv[GC_COLORED_LST];
	selectstack 	 = all_nodes_bv[GC_SELECT_STCK];
	newtemp 		 = all_nodes_bv[GC_NEW_TEMP];
}

//TODO [SV] free memory of nodes and colors
	template<typename _IR>
jit::GCRegAlloc<_IR>::~GCRegAlloc ()
{
	typename std::map<RegType, GraphNode *>::iterator m;

	for(m = allnodes.begin(); m != allnodes.end(); m++)
	{
		delete m->second;
	}

	allnodes.clear();

	typename std::vector<Color *>::iterator c;
	for(c = colors.begin(); c != colors.end(); c++)
	{
		delete *c;
	}

	colors.clear();

	for (uint32_t i = 0; i < all_nodes_bv.size(); i++)
	{
		if(all_nodes_bv[i])
			delete all_nodes_bv[i];
	}

//	for(uint32_t i = 0; i < all_move_lists.size(); i++)
//	{
//		all_move_lists[i]->clear();
//	}

//	typename std::vector< MoveList_t* >::iterator list_it;
//
//	for(list_it = all_move_lists.begin(); list_it != all_move_lists.end(); list_it++)
//	{
//		move_iterator_t i;
//
//		for(i = (*list_it)->begin(); i != (*list_it)->end(); i++)
//		{
//			delete *i;
//		}
//
//		(*list_it)->clear();
//	}
}

template <typename _IR>
void jit::GCRegAlloc<_IR>::dumpSet(std::ostream& os, const nbBitVector& bv) const
{
	for(nbBitVector::iterator it = bv.begin(); it != bv.end(); it++)
	{
		getNode(*it)->dump(os);
	}
	os << std::endl;
}

template <typename _IR>
void jit::GCRegAlloc<_IR>::dumpSet(std::ostream& os, const NodeList_t& list) const
{
	for(const_node_iterator_t n = list.begin(); n != list.end(); n++)
	{
		(*n)->dump(os);
	}
	os << std::endl;
}

template <typename _IR>
void jit::GCRegAlloc<_IR>::dumpSet(std::ostream& os, const MoveList_t& list) const
{
	for(const_move_iterator_t n = list.begin(); n != list.end(); n++)
	{
		(*n)->dump(os);
	}
	os << std::endl;
}

template<typename _IR>
void jit::GCRegAlloc<_IR>::dumpInterferenceGraph(std::ostream& os) const
{
	typedef typename std::map<RegType, GraphNode *>::const_iterator map_iterator_t;

	os << "\nInterference graph\n";

	for(map_iterator_t i = allnodes.begin(); i != allnodes.end(); i++)
	{
		GraphNode *n = i->second;
		n->dump(os);
		os << ": ";
		dumpInterference(os, n);
		os << std::endl;
	}
}

template<typename _IR>
void jit::GCRegAlloc<_IR>::dumpSets(std::ostream& os) const
{
	os << "Nodes list:" << std::endl;

	os << "Precolored" << std::endl;
	dumpSet(os, *precolored);

	os << "Initial" << std::endl;
	dumpSet(os, *initial);

	os << "SimplifyWorkSet" << std::endl;
	dumpSet(os, *simplifyworklist);

	os << "FreezeWorkSet" << std::endl;
	dumpSet(os, *freezeworklist);

	os << "SpillWorkSet" << std::endl;
	dumpSet(os, *spillworklist);

	os << "SpilledNodes" << std::endl;
	dumpSet(os, *spillednodes);

	os << "CoalescedNodes" << std::endl;
	dumpSet(os, *coalescednodes);

	os << "ColoredNodes" << std::endl;
	dumpSet(os, *colorednodes);

	os << "SelectStack" << std::endl;
	dumpSet(os, SelectStack);

	os << "NewTemp" << std::endl;
	dumpSet(os, *newtemp);

	os << "CoalescedMoves" << std::endl;
	dumpSet(os, CoalescedMoves);

	os << "ConstrainedMoves" << std::endl;
	dumpSet(os, ConstrainedMoves);

	os << "FrozenMoves" << std::endl;
	dumpSet(os, FrozenMoves);

	os << "WorkListMoves" << std::endl;
	dumpSet(os, WorkListMoves);

	os << "ActiveMoves" << std::endl;
	dumpSet(os, ActiveMoves);
}

	template<typename _IR>
jit::GCRegAlloc<_IR>::_Register::_Register(RegType& reg, Color* color, bool precolored)
	: reg(reg), color(color), precolored(precolored) {}

	template<typename _IR>
jit::GCRegAlloc<_IR>::_Register::~_Register()
{
	//delete color;
}

/*!
 * \param reg The register this node represents
 * \param precolored true if this node is precolored
 */
template<typename _IR>
jit::GCRegAlloc<_IR>::_GraphNode::_GraphNode(Register* reg) : reg(reg), alias(NULL), adjSet(NULL), color(reg->color), degree(0), spilled(false), num(0), cost(0) { }

	template<typename _IR>
jit::GCRegAlloc<_IR>::_GraphNode::~_GraphNode()
{
	delete reg;
	delete adjSet;
}

/*!
 * \returns this node degree
 */
template<typename _IR>
uint16_t jit::GCRegAlloc<_IR>::_GraphNode::get_degree() const
{
	return degree;
}

	template<typename _IR>
void jit::GCRegAlloc<_IR>::decrement_degree(GraphNode* node)
{
	assert(node != NULL && node->belongsToSet != GC_SELECT_STCK && node->belongsToSet != GC_COALESCED_LST);

	if(node->belongsToSet == GC_PRECOLORED_LST)
		return;

	node->degree--;

	if(node->degree < K && node->belongsToSet == GC_SPILL_LST)
	{
		nbBitVector union_set(adjacent(node));
		enable_moves(union_set);

		spillworklist->Reset(node->num);

		if(move_related(node))
			insert_in_list(GC_FREEZE_LST, node);
		else
			insert_in_list(GC_SIMPLIFY_LST, node);
	}
}

	template<typename _IR>
bool jit::GCRegAlloc<_IR>::enable_moves_remove_pred::operator()(MoveNode *m)
{
	assert(m != NULL);
	return m->belongsToSet != GC_ACTIVE_MOVES_LST;
}

template<typename _IR>
void jit::GCRegAlloc<_IR>::enable_moves(GraphNode* n)
{
	assert(n != NULL);
	MoveList_t moves_set(node_moves(n));
	MoveList_t active_set;

	remove_copy_if(moves_set.begin(), moves_set.end(),
			std::insert_iterator< MoveList_t >(active_set, active_set.begin()), enable_moves_remove_pred());

	for(move_iterator_t m_it = active_set.begin(); m_it != active_set.end(); m_it++)
	{
		MoveNode* m = *m_it;

		ActiveMoves.remove(m);
		insert_in_list(GC_WORKLIST_MOVES_LST, m);
	}
}

	template<typename _IR>
void jit::GCRegAlloc<_IR>::enable_moves(nbBitVector& nodes)
{
	for(nbBitVector::iterator n_it = nodes.begin(); n_it != nodes.end(); n_it++)
	{
		enable_moves(getNode(*n_it));
	}
}

	template<typename _IR>
bool jit::GCRegAlloc<_IR>::move_related(GraphNode *node)
{
	assert(node != NULL);
	return !node_moves(node).empty();
}

	template<typename _IR>
bool jit::GCRegAlloc<_IR>::node_moves_remove_pred::operator()(MoveNode *m)
{
	assert(m != NULL);
	return m->belongsToSet != GC_ACTIVE_MOVES_LST && m->belongsToSet != GC_WORKLIST_MOVES_LST;
}

	template<typename _IR>
void jit::GCRegAlloc<_IR>::freeze_moves(GraphNode* u)
{
	assert(u != NULL);

	MoveList_t move_list(node_moves(u));

	for(move_iterator_t m_it = move_list.begin(); m_it != move_list.end(); m_it++)
	{
		MoveNode* m = *m_it;
		GraphNode* x = m->from;
		GraphNode* y = m->to;
		GraphNode* v;

		if(get_alias(y) == get_alias(u))
		{
			v = get_alias(x);
		}
		else
		{
			v = get_alias(y);
		}

		ActiveMoves.remove(m);

		insert_in_list(GC_FROZEN_MOVES_LST, m);

		if(v->belongsToSet == GC_FREEZE_LST && node_moves(v).empty())
		{
			freezeworklist->Reset(v->num);
			insert_in_list(GC_SIMPLIFY_LST, v);
		}

	}
}

template<typename _IR>
	typename jit::GCRegAlloc<_IR>::MoveList_t
jit::GCRegAlloc<_IR>::node_moves(GraphNode *node)
{
	assert(node != NULL);
	MoveList_t res;

	remove_copy_if(node->moveList.begin(), node->moveList.end(),
			std::insert_iterator< MoveList_t >(res, res.begin()), node_moves_remove_pred());

	return res;
}

	template<typename _IR>
void jit::GCRegAlloc<_IR>::add_work_list(GraphNode *node)
{
	assert(node != NULL);

	if(node->belongsToSet != GC_PRECOLORED_LST
			&& !move_related(node)
			&& node->degree < K)
	{
		freezeworklist->Reset(node->num);
		insert_in_list(GC_SIMPLIFY_LST, node);
	}
}

	template<typename _IR>
inline bool jit::GCRegAlloc<_IR>::ok(GraphNode* t, GraphNode* r)
{
	assert(t != NULL && r != NULL);

	return t->degree < K
		|| t->belongsToSet != GC_PRECOLORED_LST
		|| t->has_interference_with(r) || r->has_interference_with(t);
}

	template<typename _IR>
void jit::GCRegAlloc<_IR>::combine(GraphNode *u, GraphNode *v)
{
	assert(u != NULL && v != NULL);

	if(v->belongsToSet == GC_FREEZE_LST)
	{
		assert(!freezeworklist->empty());
		freezeworklist->Reset(v->num);
	}
	else
	{
		assert(!spillworklist->empty());
		spillworklist->Reset(v->num);
	}

	insert_in_list(GC_COALESCED_LST, v);
	v->alias = u;


	MoveNodeLess less;
	MoveNodeEqu	equ;
	u->moveList.sort(less);
	u->moveList.unique(equ);

	v->moveList.sort(less);
	v->moveList.unique(equ);

	set_union(
		u->moveList.begin(), u->moveList.end(),
		v->moveList.begin(), v->moveList.end(),
		std::insert_iterator<MoveList_t>(u->moveList, u->moveList.begin()));

	enable_moves(v);

	nbBitVector adjacents(adjacent(v));

	for(nbBitVector::iterator t = adjacents.begin(); t != adjacents.end(); t++)
	{
		add_edge( getNode(*t), u);
		decrement_degree(getNode(*t));
	}

	if(u->degree >= K && u->belongsToSet == GC_FREEZE_LST)
	{
		freezeworklist->Reset(u->num);
		insert_in_list(GC_SPILL_LST, u);
	}
}

	template<typename _IR>
bool jit::GCRegAlloc<_IR>::conservative(nbBitVector& nodes)
{
	uint16_t k = 0;

	for(nbBitVector::iterator i = nodes.begin(); i != nodes.end(); i++)
	{
		if( getNode(*i)->degree >= K )
			k++;
	}

	return k < K;
}

template<typename _IR>
typename jit::GCRegAlloc<_IR>::GraphNode*
jit::GCRegAlloc<_IR>::get_alias(GraphNode *node) const
{
	assert(node != NULL);

	if(node->belongsToSet == GC_COALESCED_LST)
		return get_alias(node->alias);

	return node;
}

template<typename _IR>
bool jit::GCRegAlloc<_IR>::coalesce_test(GraphNode* u, GraphNode* v)
{
	assert(u != NULL && v != NULL);

	nbBitVector adjacents(adjacent(v));

	for(nbBitVector::iterator t = adjacents.begin(); t != adjacents.end(); t++)
	{
		if (!ok( getNode(*t), u))
			return false;
	}

	return true;
}

template<typename _IR>
typename jit::GCRegAlloc<_IR>::GraphNode* jit::IRegSpiller<_IR>::selectSpillNode(std::list< GraphNode* >& reglist)
{
	typedef typename std::list<GraphNode*>::iterator list_iterator_t;
	list_iterator_t i, min;
	double tmp_ratio = 0;
	bool first = true;
	i = reglist.begin();
	uint32_t cost;// = 4,294,967,295;
	uint32_t degree;// = (*i)->degree;
	double result; // = 4294967295;
	//tmp_ratio = result;
	min = i;
	for(; i != reglist.end(); i++)
	{
		if((*i)->spilled)
			continue;
		cost = 1; //(*i)->cost;
		degree = (*i)->degree;
		result = (double)cost / (double)(degree);
#ifdef _DEBUG_SPILL
		std::cout << "Costo del registro: " << (*i)->reg->reg << " " << std::endl;
		std::cout << "  Costo ------> " << cost << std::endl;
		std::cout << "  Grado ------> " << degree << std::endl;
		std::cout << "  Risultato --> " << result << std::endl;
#endif
		if(first)
		{
			tmp_ratio = result;
			min = i;
			first = false;
		}
		else if(result < tmp_ratio)
		{
			tmp_ratio = result;
			min = i;
		}
	}
	assert(!first && "No more unspilled registers");
#ifdef _DEBUG_SPILL
	std::cout << "Select spill: ho scelto il registro: " << (*min)->reg->reg << std::endl;
#endif
	return *min;
}

template<typename _IR>
jit::IRegSpiller<_IR>::~IRegSpiller()
{}

