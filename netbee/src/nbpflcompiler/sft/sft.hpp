/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef _SFT_H
#define _SFT_H

#include "sft_defs.h"
#include "./digraph_stlplus/digraph.hpp"
#include "sft_traits.hpp"
#include "sft_generic_set.hpp"
#include "sft_basic_iterator.hpp"
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <stack>
#include <assert.h>
#include <string>
#include "librange/range.hpp"
#include <iomanip> // for prettyETDotPrint


template <class SType, class LType, class PType> class sftTools;

/*!
  \brief This class is a template for a Finite State Automaton with predicates on transitions and code associated with the symbols

  Template parameters are:
	SType: Type of the information associated with FSA states
	LType: Type of the symbols on transitions
  	PType: Type of the predicates activating transitions
  	AType: Type of the attributes on which the predicates are expressed
  	VType:
  	SPType:
  	BType:
  	CType: Type of the code associated with the varius symbols
	SFType: Type of the extra information associated with the FSA states
  
  	Each one of these has an associated traits type in sft_traits.hpp. For each user-defined SType, LType, PType a
  	corresponding traits class must be defined.

*/

class Code; //forward declaration

template <class SType, class LType, class PType = bool, class AType = std::string, class VType = uint32, class SPType = std::string, class CType = Code*, class SFType = SymbolField*, class BType = bool>
class sftFsa
{

  friend class sftTools<SType, LType, PType>;

public:
  class State;
  class Transition;
  class ExtendedTransition;
  friend class State;
  friend class Transition;
  friend class ExtendedTransition;

private:

  typedef typename stlplus::digraph<State*, Transition*> Graph_t;
  typedef typename stlplus::digraph<State*, Transition*>::iterator Node_t;
  typedef typename stlplus::digraph<State*, Transition*>::const_iterator Const_Node_t;  
  typedef typename stlplus::digraph<State*, Transition*>::arc_iterator Edge_t;

public:
  typedef STraits<SType> s_traits;
  typedef LTraits<LType> l_traits;
  typedef PTraits<PType> p_traits;
  typedef PTraits<SPType> sp_traits;
  typedef sft_generic_set<State*> StateSet_t;
  typedef sft_generic_set<Transition*> TransSet_t;
  typedef sft_generic_set<LType> Alphabet_t;
  typedef std::map<SType,std::list<CType> > Instruction_t;
  typedef std::pair<Alphabet_t, PType> trans_pair_t;
  typedef std::list<State*> StateList_t;
  
  typedef stlplus_digraph_node<State*, State*, Transition*> node_traits;
  typedef sft_basic_iterator<Graph_t, Node_t, State*, node_traits> StateIterator;

  typedef stlplus_digraph_arc<Transition*, State*, Transition*> arc_traits;
  typedef sft_basic_iterator<Graph_t, Edge_t, Transition*, arc_traits> TransIterator;


  class State
  {
    friend class sftFsa;
    friend class sftTools<SType, LType, PType>;

    sftFsa	*Owner;					//!< Owner FSA
    uint32	Id;						//!< Id of this state inside the owner FSA
    SType	Info;					//!< Information associated to this state
    Node_t	Node;					//!< Graph Node iterator
    bool	accept;					//!< Accepting flag (true if the state is an accepting one)
    bool	action;					//!< Action flag (true if the state is an action one)
    bool    final; 					// no transitions get out from this state
    bool	Initial;				//!< Initial flag (true if the state is initial)
    bool   	Visited;				//!< Visited flag (true if, during a graph traversal, the node has already been visited)
    list<SFType>	ExtraInfo; 		//<! Extra Information associated to this state
    list<VType> 	ExtraInfoPlus;	//<! Furhter information associated with ExtraInfo
    list<BType> 	OtherInfo;		//<! Other information associated with ExtraInfo

    /*!
      \brief Set the graph node corresponding to this state
      \param node reference to a graph node iterator
    */
    void SetGraphNode(Node_t &node)
    {
      Node = node;
    }

    /*!
      \brief Constructor

      \param owner the owner FSA
      \param id id of this state inside the owner FSA
      \param info user information associated to this state

      Note: a State object can only be created through methods of the sftFsa class, so we make it private. The sftFsa class
      still has access to it thanks to the friend clause
    */
    State(sftFsa &owner, uint32 id, SType info)
      {
    	this->Owner = &owner;
    	this->accept = false;
    	this->action = false;
        this->final = false;
    	this->Initial = false;
    	this->Visited = false;
    	this->Id = id;
    	this->Info = info;
      }
      

    /*!
      \brief Constructor that allows to "export" a state from a FSA to another

      \param owner the FSA that will own the newly created state
      \param other the state that is exported to the new FSA

      Note: the state, when it is created, does not belong to a digraph. Therefore, the Node parameter is not initialized (the task must be done "by hand", using SetGraphNode()).
    */
    State(sftFsa *owner, const State &other)
      :Owner(owner), Id(other.Id), Info(other.Info), accept(other.accept),
       action(other.action), final(other.final),Initial(other.Initial),ExtraInfo(other.ExtraInfo){}

    // Set the accepting status
    void setAccepting(bool accepting_status)
    {
      accept = accepting_status;
    }

	// Set the action status
    void setAction(bool action_status)
    {
      action = action_status;
    }

    // Set whether this state is final
    void setFinal(bool final_status)
    {
      final = final_status;
    }
    
    // Set extra info to the state
    void setExtraInfo(list<SFType> ei)
    {
    	for(typename list<SFType>::iterator it = ei.begin(); it != ei.end(); it++)
    		ExtraInfo.push_back(*it);
    }
    
    // Set extra info to the state
    void setExtraInfoPlus(list<VType> eip)
    {
    	for(typename list<VType>::iterator it = eip.begin(); it != eip.end(); it++)
    		ExtraInfoPlus.push_back(*it);
    }

    // Set extra info to the state
    void setOtherInfo(list<BType> oei)
    {
    	for(typename list<BType>::iterator it = oei.begin(); it != oei.end(); it++)
    		OtherInfo.push_back(*it);
    }


    /*!
      \brief Set the state as initial
    */
    void SetInitial()
    {
      Initial = true;
    }

    /*!
      \brief Set the state as non-initial
    */
    void ResetInitial()
    {
      Initial = false;
    }

  public:
  
    sftFsa *GetOwner(void)
    {
    	return Owner;
    }

    void SetInfo(SType i){
      Info = i;
    }

    /*!
      \brief Copy Constructor
      \param other another state
    */
    State(const State &other)
      :Owner(other.Owner), Id(other.Id), Info(other.Info), Node(other.Node),
        accept(other.accept),action(other.action), final(other.final), Initial(other.Initial),
       Visited(other.Visited),ExtraInfo(other.ExtraInfo){}


    /*!
      \brief Mark the state as non-visited
    */
    void ResetVisited()
    {
      Visited = false;
    }

    /*!
      \brief Mark the state as visited
    */
    void SetVisited()
    {
      Visited = true;
    }


    /*!
      \brief Tells if the state is null
      \return true if the state is null, false otherwise
    */

    bool IsNull()
    {
      return Id == 0;
    }

    /*!
      \brief Get the state ID inside the FSA
      \return the id associated to this state
    */
    uint32 GetID()
    {
      return Id;
    }

    /*!
      \brief Tells if the state is accepting
      \return true if the state is accepting, false otherwise
    */

    bool isAccepting()
    {
      return accept;
    }
    
    //Tells if the state is action
    bool isAction()
    {
    	return action;
    }

    // Tells if the state is final
    bool isFinal()
    {
      return final;
    }

    /*!
      \brief Tells if the state is the initial state of the owner FSA
      \return true if the state is initial, false otherwise
    */
    bool IsInitial()
    {
      return Initial;
    }

    /*!
      \brief Tells if the state has been visited (during a graph traversal)
      \return true if the state has been marked as "visited", false otherwise
    */
    bool HasBeenVisited()
    {
      return Visited;
    }

    /*!
      \brief Get the user information associated to this state
      \return a reference to an object of type SType
    */
    SType &GetInfo()
    {
      return Info;
    }

	/*!
      \brief Get the extra user information associated to this state
      \return a reference to an object of type set<SFType>
    */
    list<SFType> &GetExtraInfo()
    {
      return ExtraInfo;
    }

	/*!
      \brief Get the extra user information associated to this state
      \return a reference to an object of type set<VType>
    */
    list<VType> &GetExtraInfoPlus()
    {
      return ExtraInfoPlus;
    }
    
    /*!
      \brief Get the extra user information associated to this state
      \return a reference to an object of type set<VType>
    */
    list<BType> &GetOtherInfo()
    {
      return OtherInfo;
    }


    /*!
      \brief Compare to another state
      \param other reference to another state
      \return +1 if the state is "greater than" the other, 0 if they are equal, -1 if the state is "less than" the other
    */
    int32 CompareTo(const State &other) const
    {
    	if(this->Owner != other.Owner)
    		return -1;
      if (this->Id < other.Id)
	return -1;
      if (this->Id == other.Id)
	return 0;
      return 1;
    }

    /*!
      \brief Assignment operator
      \param other reference to another state
      \return a reference to the modified state
    */
    State &operator=(const State &other)
    {
      Owner = other.Owner;
      Id = other.Id;
      Info = other.Info;
      Node = other.Node;
      accept = other.accept;
      action = other.action;
      final = other.final;
      Initial = other.Initial;
      ExtraInfo = other.ExtraInfo;
      return *this;
    }


    /*!
      \brief Equality operator
      \param other reference to another state
      \return true if the two states are "equal", false otherwise
    */
    bool operator==(const State &other) const
    {
      if (this == NULL)
        return false;
      if (this->CompareTo(other) == 0)
	return true;
      return false;
    }

    /*!
      \brief Inequality operator
      \param other reference to another state
      \return true if the two states are "not equal", false otherwise
    */
    bool operator!=(const State &other) const
    {
      if (this->CompareTo(other) != 0)
	return true;
      return false;

    }

    /*!
      \brief Less than operator
      \param other reference to another state
      \return true if the state is "less than" the other, false otherwise
    */
    bool operator<(const State &other) const
    {
      if (this->CompareTo(other) < 0)
	return true;
      return false;
    }

    /*!
      \brief Create a string representation of this state
      \return a string
    */
    std::string ToString(void)
    {
      return s_traits::ToString(Info);
    }


  };


  class Transition
  {
    friend class sftFsa;


    sftFsa					*Owner;		//!< Owner FSA
    uint32					Id;			//!< Id of this transition inside the owner FSA
    Alphabet_t				Labels;		//!< Set of labels associated to this transition
    PType					Predicate;	//!< Predicate associated to this transition (the transition is activated only if the predicate evaluates to true)	
    Edge_t					Edge;		//!< Graph Edge iterator
    bool					Epsilon;	//!< True if the transition is an epsilon-transition
    bool					Complement;	//!< True if the Labels set must be complemented
    bool 					Visited;    //!< True if the transition has already been visited during code generation
    ExtendedTransition*		related_et; //!< A reference to the ExtendedTransition that includes this transition, if any
	std::pair<VType,BType>	ExtraInfo;	//!< Extra information associated with the transition

    /*!
      \brief Set the graph node corresponding to this state
      \param node reference to a graph node iterator
    */
    void SetEdge(const Edge_t &edge)
    {
      Edge = edge;
    }
  
    /*!
      \brief Constructor

      \param owner the owner FSA
      \param id id of this state inside the owner FSA
      \param label a label associated to this transition
      \param complement if true, the labels set must be complemented
      \param predicate a predicate associate to this transition (the default value is the unity, i.e. "always true" predicate)

      Note: a Transition object can only be created through methods of the sftFsa class, so we make it private. The sftFsa class
      still has access to it thanks to the friend clause
    */
    Transition(sftFsa &owner, uint32 id, LType label, bool complement = false, PType predicate = p_traits::One())
      :Owner(&owner), Id(id), Predicate(predicate), Epsilon(false), Complement(complement), related_et(NULL), ExtraInfo(make_pair<VType,BType>(0,0))
    {
      Labels.insert(label);
    }

    /*!
      \brief Constructor

      \param owner the owner FSA
      \param id id of this state inside the owner FSA
      \param labels a labels set associated to this transition
      \param complement a flag that is true if the set of labels must be complemented
      \param predicate a predicate associate to this transition (the default value is the unity, i.e. "always true" predicate)

      Note: a Transition object can only be created through methods of the sftFsa class, so we make it private. The sftFsa
      class still has access to it thanks to the friend clause
    */
    Transition(sftFsa &owner, uint32 id, Alphabet_t labels, bool complement = false, PType predicate = p_traits::One())
      :Owner(&owner), Id(id), Labels(labels), Predicate(predicate), Epsilon(false), Complement(complement), related_et(NULL), ExtraInfo(make_pair<VType,BType>(0,0)){}


    /*!
    \brief Constructor

    \param owner the owner FSA
    \param id id of this state inside the owner FSA
    \param labels a labels set associated to this transition
    \param complement a flag that is true if the set of labels must be complemented
    \param epsilon a flag that is true if the transition is an epsilon-transition
    \param predicate a predicate associate to this transition (the default value is the unity, i.e. "always true" predicate)

    Note: a Transition object can only be created through methods of the sftFsa class, so we make it private. The sftFsa class
    still has access to it thanks to the friend clause
     */
    Transition(sftFsa &owner, uint32 id, Alphabet_t labels, bool complement, bool epsilon, PType predicate, ExtendedTransition* related_et)
      :Owner(&owner), Id(id), Labels(labels), Predicate(predicate), Epsilon(epsilon), Complement(complement), related_et(related_et), ExtraInfo(make_pair<VType,BType>(0,0)){}


    /*!
      \brief Alternative Constructor for epsilon transitions

      \param owner the owner FSA
      \param id id of this state inside the owner FSA
      \param predicate a predicate associate to this transition  (the default value is the unity, i.e. "always true" predicate)


      Note: no label is added to the transition
    */
    Transition(sftFsa &owner, uint32 id, PType predicate = p_traits::One())
      :Owner(&owner), Id(id), Predicate(predicate), Epsilon(true), Complement(false), Visited(false), related_et(NULL), ExtraInfo(make_pair<VType,BType>(0,0)){}

    /*!
      \brief Constructor that allows to "export" a transition from a FSA to another

      \param owner the FSA that will own the newly created transition
      \param other the transition that is exported to the new FSA

      Note: the transition, when it is created, does not belong to a digraph. Therefore, the Edge parameter is not initialized (the task must be done "by hand", using SetEdge()).
    */
    Transition(sftFsa *owner, const Transition &other)
      :Owner(owner), Id(other.Id), Labels(other.Labels), Predicate(other.Predicate), Epsilon(other.Epsilon), Complement(other.Complement),
       Visited(other.Visited), related_et(NULL), ExtraInfo(other.ExtraInfo()){}

  public:
  		//remove the edge associated to a transition. you need to call this method before to delet a transition
  		//i'm not sure about the case in which the transition is part of an extended transition.
  		//then, if there is an error, probably it's here		    	
    	void RemoveEdge()
    	{
    		if(this->getIncludingET())
    		{
    			ExtendedTransition *et = this->getIncludingET();
    			et->RemoveEdge(Edge);
    		}
    		this->Owner->m_Graph.arc_erase(Edge);//it's possible that this operation is needed only if the transition is not related to an extended transition
    	}

      	const Edge_t GetEdge()
    	{
      		return Edge;
    	}
    
     void SetExtraInfo(VType value)
     {
     	ExtraInfo = std::make_pair<VType,BType>(value,false);
     }
     
     void SetExtraInfoFlag()
     {
     	ExtraInfo.second = true;
     }
     
     VType GetExtraInfo()
     {
     	return ExtraInfo.first;
     }

	  void SetVisited(void)
	  {
		  this->Visited = true;
		  return;
	  }

	  void ResetVisited(void)
	  {
		  this->Visited = false;
		  return;
	  }

	  bool IsVisited(void)
	  {
		  return this->Visited;
	  }

	  void AddLabel(LType label)
	  {
	  	Labels.insert(label);
	  }
	 
	  void RemoveLabel(LType label)
	  {
	  	Labels.remove(label);
	  }

	  void AddLabels(Alphabet_t labels)
	 {
	   Labels.union_with(labels);
	 }
	 
	 Alphabet_t GetLabels(void){
	 	return Labels;	
	 }	 

    virtual bool IsExtended(void){
      return false;
    }

    ExtendedTransition* const getIncludingET(void){
      return this->related_et;
    }

    void setIncludingET(ExtendedTransition* et){
      this->related_et = et;
    }

    /*!
      \brief Copy Constructor
      \param other another transition

    */
    Transition(const Transition &other)
      :Owner(other.Owner), Id(other.Id), Labels(other.Labels), Predicate(other.Predicate), Edge(other.Edge), Visited(other.Visited), ExtraInfo(other.ExtraInfo){}

    virtual ~Transition() {}

    /*!
     * \brief Return the information associated to this transition
     * \return a \link trans_pair_t containing a \link Alphabet_t and a PType object
     *
     */
    trans_pair_t GetInfo()
    {
      Alphabet_t labels(Labels);
      return trans_pair_t(labels, Predicate);
    }

    PType GetPredicate()
    {
    	return this->Predicate;
    }


    /*!
      \brief Compare to another transition
      \param other reference to another transition
      \return +1 if the transition is "greater than" the other, 0 if they are equal, -1 if the transition is "less than" the other
    */
    int32 CompareTo(const Transition &other) const
    {
      if (this->Id < other.Id)
	return -1;
      if (this->Id == other.Id)
	return 0;
      return 1;
    }


    /*!
      \brief Equality operator
      \param other reference to another transition
      \return true if the two transitions are "equal", false otherwise
    */
    bool operator==(const Transition &other) const
    {
      if (this->CompareTo(other) == 0)
		return true;
      return false;
    }


    /*!
      \brief Inequality operator
      \param other reference to another transition
      \return true if the two transitions are "not equal", false otherwise
    */
    bool operator!=(const Transition &other) const
    {
      return !((*this) == other);
    }


    /*!
      \brief Tells if this transition is an epsilon one
      \return true if the transition is an epsilon, false otherwise
    */
    bool IsEpsilon(void)
    {
      return Epsilon;
    }


    /*!
      \brief Tells if the labels associated to this transition should be complemented
      \return true if the labels set is a complement set, false otherwise
    */
    bool IsComplementSet(void)
    {
      return Complement;
    }

   void SetComplementSet(void)
   {
	 Complement = true;
   }

   void ResetComplementSet(void)
   {
	 Complement = false;
   }


    /*!
      \brief Get an iterator for the starting state of this transition
      \return a StateIterator
    */
    StateIterator FromState(void)
    {
      Node_t n = Owner->m_Graph.arc_from(Edge);
      return StateIterator(Owner->m_Graph, n);
    }

    /*!
      \brief Get an iterator for the ending state of this transition
      \return a StateIterator
    */
    StateIterator ToState(void)
    {
      Node_t n = Owner->m_Graph.arc_to(Edge);
      return StateIterator(Owner->m_Graph, n);
    }

    /*!
      \brief Create a string representation of this Transition
      \return a string
    */
    std::string ToString(void)
    {    
      std::stringstream ss;

      if (!Epsilon)
	{
	  uint32 n_syms = Labels.size();
	  uint32 i = 0;
	  if (Complement)
	    ss << "*-";
	  ss << "{";
	  for (typename Alphabet_t::iterator k = Labels.begin(); k != Labels.end(); k++)
	  {
			LType tmp_ltype = *k;
			ss << l_traits::ToString(tmp_ltype);
	
			if(!Complement)
			{
				//print the code related to the symbol
				
				{
					typename Instruction_t::iterator it = Owner->m_Instruction1.find(tmp_ltype.second);
					if(it != Owner->m_Instruction1.end())
					{					
						list<CType> code = (*it).second;
						for(typename list<CType>::iterator c = code.begin(); c != code.end(); c++)
						{
							ss << "/" << (*c)->GetCode();
						}
					}
				}
				
				{
					typename Instruction_t::iterator it = Owner->m_Instruction2.find(tmp_ltype.second);
					if(it != Owner->m_Instruction2.end())
					{
						list<CType> code = (*it).second;
						for(typename list<CType>::iterator c = code.begin(); c != code.end(); c++)
						{
							ss << "/" << (*c)->GetCode();
						}
					}
				}				
			}
			
			i++;
			if (i != n_syms)
				ss << ",";

	  }
	  if(ExtraInfo.first != 0)
	  {
	  	//print the extra info associtated with the transition
	  	ss << " | ";
	  	if(ExtraInfo.second)
	  		ss << ">= ";
	  	else
	  		ss << "== ";
	  	ss << ExtraInfo.first;
	  }
	  
	  ss << "}";
	}
      else
	{
	  ss << "eps";
	}
      if(Predicate)
        if (p_traits::DumpNeeded(Predicate))
        		ss << "/" << p_traits::ToString(Predicate);

      return ss.str();
    }

  };


  /*
   * Internal class modeling the extended transition (or 2nd-level)
   * transition mechanism.
   * 
   * The 'Predicate' attribute loses its meaning inside here
   *
   * 'Edge' should be always NULL, and is superseded by 'edges'
   *
   * 'Epsilon' has no meaning, as the extended transition is always
   * bound to a single input symbol.
   */
  class ExtendedTransition: public Transition  
  {
    friend class sftFsa;

  private:
    class SubState_t; // fwd decl
    typedef Range<VType, SubState_t*,SPType> SubTrans_t;
    
  /************************************************************/
    class SubState_t 
    {
      friend class ExtendedTransition;
      
      bool is_input_gate;
      bool is_output_gate;
      State *statePtr;

      // The following attributes must be ignored if:
      // (is_output_gate == true) && (is_input_gate == false)
      std::set<SubTrans_t*> out_trans; // list of outgoing transitions (except deltas, treated separately)
      std::set<SubState_t*> delta_targets; // list of states reachable with delta transitions
      AType next_checked_attr;

      //SubState_t contructor
      SubState_t(State *ptr = NULL, bool i_gate = false, bool o_gate = false)
        : is_input_gate(i_gate), is_output_gate(o_gate), statePtr(ptr)   {  }

      SubState_t(SubState_t *other, bool copy_transitions = false) //this copies a state into another one
        : is_input_gate(other->is_input_gate), is_output_gate(other->is_output_gate),
          statePtr(other->statePtr)
        {
          if (copy_transitions) {
            this->next_checked_attr = other->next_checked_attr;
            this->delta_targets = other->delta_targets;
            for (typename std::set<SubTrans_t*>::iterator i = other->out_trans.begin();
                 i != other->out_trans.end();
                 ++i)
              this->out_trans.insert(new SubTrans_t(*i)); 
          
          }
          
      }
      
      //Change the state associated with the sub state
      void ChangeStatePtr(State *newPtr)
      {
      	statePtr = newPtr;
      }
      
    };
  /************************************************************/
  
    typedef std::set<SubState_t*> SubStateSet_t;
    typedef std::set<SubTrans_t*> SubTransSet_t;

    // ~~ private properties ~~
    std::list<Edge_t> edges; // list of edges, whose union is equivalent to this ExtendedTransition
    SubState_t *startingNode; // the starting node of the subautomaton (the input gate)  
    // set of all the states, useful for enumeration
    std::set<SubState_t*> states;
    std::set<SubTrans_t*> transitions;
    // map of output gates <state_info,nodeif(op==MATCH)_in_the_graph>, kept here for performance reasons
    typename std::map<State*, SubState_t*> output_gates;
    typename std::set<AType> checked_attrs; /* explicit, ordered (crescent) set of attributes checked by
                                             * the predicates of this subautomaton
                                             */
    // ~~ private methods ~~
    // ~ overrides ~
    void SetEdge(const Edge_t &edge) {
      this->Edge = NULL;
      nbASSERT(false, "DO NOT call me, EVER!");
    }

    /***
    *
    *	This costructor manage the case in wich the extended transition must be built starting from a PType 
    *
    ***/
    ExtendedTransition(sftFsa &owner, uint32 id, LType /*EncapLabel_t*/ label,
                       State* /*EncapStIterator_t*/ in_gate,
                       PType /*PFLTermExpression*/ predicate, State* /*EncapStIterator_t*/ out_gate, Edge_t /*Transition*/ equiv_edge_1,
                       State* /*EncapStIterator_t*/ dfl_out_gate, Edge_t /*Transition*/ equiv_edge_2)
      : Transition(owner, id, label, false)
    {    
      // fetch attribute
           
      AType a = predicate->getAttribute();  //get a string with the field analysed, for example ip.src
      
      RangeOperator_t op = predicate->getIRRangeOperator(); //get the range operator, i.e. =, !=, >, >=, <, <=, matches, contains       
      
      if(op==NOT_EQUAL)
      {
      	//we have to normalize the transition
      	op = EQUAL;
      	State *aux = NULL;
      	aux = out_gate;
      	out_gate = dfl_out_gate;
      	dfl_out_gate = aux;
      }
      
      checked_attrs.insert(a);
     
      //here we have each part of the predicate
      
      // create the starting node
      startingNode = new SubState_t(in_gate, true);
      startingNode->next_checked_attr = a;
      states.insert(startingNode); //add the startingNode to the set of subStates

      // create the output gates
      SubState_t *out_1 = new SubState_t(out_gate, false, true); //the predicate is true
      states.insert(out_1);
      output_gates[out_gate]=out_1;//assign the output gate of the subautomaton to a state of the automaton
      SubState_t *out_dfl = new SubState_t(dfl_out_gate, false, true); //the predicate is false
      states.insert(out_dfl);
      output_gates[dfl_out_gate]=out_dfl;//assign the output gate of the subautomaton to a state of the automaton. this gate is reached in case of the predicates is false
	
      // connect them      
      SubTrans_t *tr = new SubTrans_t(out_dfl); //Range<VType, SubState_t*> SubTrans_t - initialize the default transition      
      transitions.insert(tr);
      SPType stringPredicate("");
      VType v; 
      if((op==MATCH)||(op==CONTAINS))
         stringPredicate=predicate->getIRStringValue();//the value of the predicate in case of the range operator is "matches" or "contains"
      else
      	 v = predicate->getIRValue(); //the value of the predicate
	  
      tr->addRange(op, v, out_1,stringPredicate); //add the true transition (rangeOperator,valueOfThePredicate,true destination,string value of the predicate)
												  //note that v or stringPredicate is used
          
      startingNode->out_trans.insert(tr);//link the transition to its starting state
	
      // save equivalent edges
      edges.push_back(equiv_edge_1);
      edges.push_back(equiv_edge_2);
     
      // ... well, I am myself :)
      this->related_et=this;
    }
        
    /***
    *
    *	This contructor is needed to manage those cases in which we have a VType, a RangeOperator_t and a AType
    *
    ***/
    ExtendedTransition(sftFsa &owner, uint32 id, LType label,
                       State* in_gate,
                       VType value, RangeOperator_t oper, AType attribute,  State* out_gate, Edge_t equiv_edge_1,
                       State* dfl_out_gate, Edge_t equiv_edge_2)
      : Transition(owner, id, label, false)
    {    
        // create the starting node
      	startingNode = new SubState_t(in_gate, true);   
      	states.insert(startingNode); //add the startingNode to the set of subStates
      	
      	//create the output nodes and link them with the related output gate
      	SubState_t *out_1 = new SubState_t(out_gate, false, true); 
      	states.insert(out_1);
      	output_gates[out_gate]=out_1;
    	SubState_t *out_dfl = new SubState_t(dfl_out_gate, false, true);
      	states.insert(out_dfl);
      	output_gates[dfl_out_gate]=out_dfl;
      	
      	// save equivalent edges
      	edges.push_back(equiv_edge_1);
      	edges.push_back(equiv_edge_2);
		
		//manage the attribute provided outside of the PType
		checked_attrs.insert(attribute);
		startingNode->next_checked_attr = attribute;
		SubTrans_t *tr = new SubTrans_t(out_dfl);
		transitions.insert(tr);
		SPType stringPredicate("");
		SubState_t *middleNode = NULL;
		
		//get the value of the predicate
		tr->addRange(oper, value, (middleNode)? middleNode : out_1,stringPredicate);	
		startingNode->out_trans.insert(tr);
    			
		// ... well, I am myself :)
		this->related_et=this;
    }
        
    /***
    *
    *	This is a kind of copy constructor. But the input and output gates are different than those of the original ET
    *	For example, you can use it when the old and the new et are of two different automata
    *
    ***/
    ExtendedTransition(sftFsa &owner, uint32 id, LType label, State* in_gate, State* out_gate, Edge_t equiv_edge_1,
                       State* dfl_out_gate, Edge_t equiv_edge_2,ExtendedTransition *other)
      : Transition(owner, id, label, false),checked_attrs(other->checked_attrs)
    {    
		// save equivalent edges
		edges.push_back(equiv_edge_1);
		edges.push_back(equiv_edge_2);

		// ... well, I am myself :)
		this->related_et=this;    

		map<SubState_t *,SubState_t *> old2new;
		//iterate over the substates of the "other" et
      	for(typename std::set<SubState_t*>::iterator s = other->states.begin(); s != other->states.end();++s) 
      	{
        	SubState_t *new_s = this->copyInsert(*s,true); 
      //  	new_s->statePtr = (*s)->statePtr;
			old2new[*s]=new_s;
        	if((*s)->is_input_gate)
        	{
        		//set the initial state of the sub automaton
          		this->startingNode = new_s;
          		//new_s must be associated with the new input gate, i.e. in_gate
          		new_s->ChangeStatePtr(in_gate);
          	}
        	if((*s)->is_output_gate)
        	{
        		//set the output gates of the sub automaton
        		if(((*s)->statePtr)->isAccepting())
        		{
					new_s->ChangeStatePtr(out_gate);
		    		this->output_gates[out_gate] = new_s;
		    	}
		    	else
		    	{
		    		new_s->ChangeStatePtr(dfl_out_gate);
		    		this->output_gates[dfl_out_gate] = new_s;
		    	}
   		    }
   		    
   		    transitions.insert(new_s->out_trans.begin(),new_s->out_trans.end());
		}
		
		changeStateRefs(this->states, old2new);
    }
    
    // ~~ helper methods ~~
    static SubStateSet_t followDeltas
    (SubState_t* state)
    {
      nbASSERT(state != NULL, "state cannot be NULL");
      std::set<SubState_t*> to_ret = state->delta_targets;
      to_ret.insert(state);

      return to_ret;
    }

    static SubStateSet_t followDeltas
    (const SubStateSet_t stateset)
    {
      SubStateSet_t to_ret(stateset);
      for (typename SubStateSet_t::iterator i = stateset.begin();
           i != stateset.end();
           ++i) {
        SubStateSet_t partial = followDeltas(*i);
        to_ret.insert(partial.begin(), partial.end());
      }
      return to_ret;
    }

    // copy and insert
    SubState_t* copyInsert(SubState_t *s, bool copy_transitions = true) {
      SubState_t* node = new SubState_t(s, copy_transitions);
      this->states.insert(node);
      return node;
    }

    // change all the state mappings inside all the transitions, normal and deltas
    // (useful when some states were copied over by somewhere else)
    // Made it static, so we can run this also on ETs that are still being built.
    static void changeStateRefs(const set<SubState_t*> &states,
                                const map<SubState_t *,SubState_t *> old2new) {
      for(typename std::set<SubState_t*>::iterator s = states.begin();
          s != states.end();
          ++s) {
        // normal transitions (use the library)
        for(typename std::set<SubTrans_t*>::iterator i = (*s)->out_trans.begin();
            i != (*s)->out_trans.end();
            ++i)
          (*i)->changeActions(old2new);
          
        // delta targets (do it by hand)
        std::set<SubState_t*> delta_replacement;
        for (typename std::set<SubState_t*>::iterator ds_iter = (*s)->delta_targets.begin();
             ds_iter != (*s)->delta_targets.end();
             ++ds_iter) {
          typename map<SubState_t *,SubState_t *>::const_iterator old2new_mapping =
            old2new.find(*ds_iter);
          if(old2new_mapping != old2new.end())
            // if this needs to be replaced
            delta_replacement.insert(old2new_mapping->second);
          else
            // otherwise keep it unchanged
            delta_replacement.insert(*ds_iter);
        }
        (*s)->delta_targets=delta_replacement; // replace it
      }
    }

    // returns attrs.end() if there's no lesser attribute
    static typename std::set<AType>::iterator takeLesserAttribute
    (const SubStateSet_t &states,
     typename std::set<AType> &attrs)
    {
      AType lesser_attr;
      bool atLeastOneWasFound = false;

      for (typename SubStateSet_t::iterator s = states.begin();
           s != states.end();
           ++s) {
        if((*s)->is_output_gate)
          continue; // skip attributes from out gates

        if(!atLeastOneWasFound) {
          lesser_attr = (*s)->next_checked_attr;
          atLeastOneWasFound = true;
          continue;
        }

        lesser_attr = ((*s)->next_checked_attr < lesser_attr?
                       (*s)->next_checked_attr : lesser_attr);
      }
      
      if (!atLeastOneWasFound)
        return attrs.end();

      nbASSERT(attrs.find(lesser_attr)!=attrs.end(), "Unexpected attribute found");
      
      return attrs.find(lesser_attr);
    }

    /*
     * Grabs all the non-delta transitions of the subautomaton,
     * exiting from any of the states of the given set, whose
     * predicates are referred to the given attribute.
     */
    static SubTransSet_t grabExitingTransWithAttribute
    (const SubStateSet_t &states,
     const typename std::set<AType>::iterator attr)
    {
      SubTransSet_t to_ret;
      for (typename SubStateSet_t::iterator s = states.begin();
           s != states.end();
           ++s) {
        if ( (*attr) == ((*s)->next_checked_attr) )
          to_ret.insert((*s)->out_trans.begin(), 
                        (*s)->out_trans.end() );
      }
      return to_ret;
    }
    
   /*static set<AdvancedPredicate<RangeOperator_t,AType,SubState_t>*>  grabExitingAdvPrWithAttribute
    (const SubStateSet_t &states,
     const typename std::set<AType>::iterator attr)
    {
      set<AdvancedPredicate<RangeOperator_t,AType,SubState_t>*>  ap;
      for (typename SubStateSet_t::iterator s = states.begin();
           s != states.end();
           ++s) {
        if ( (*attr) == ((*s)->next_checked_attr) )
          ap.insert((*s)->advancedPredicates.begin(), 
                        (*s)->advancedPredicates.end() );
      }
      return ap;
    }
    */
    
    

    /*
     * Return all the states that are ahead of the provided attribute.
     *
     * More formally, return all the states in the set that have a 'next_checked_attr'
     * different from the one provided and that are either output gates
     * or have at least one non-epsilon exiting transition.
     */
    static SubStateSet_t grabAheadStates
    (const SubStateSet_t &states,
      const typename std::set<AType>::iterator a)
    {
      SubStateSet_t to_ret;
      for(typename SubStateSet_t::iterator s = states.begin();
          s != states.end();
          ++s) {
        if( (*a) != ((*s)->next_checked_attr) &&
            ( (*s)->is_output_gate || (*s)->out_trans.size() )
          )
          to_ret.insert(*s);
	}

      return to_ret;
    }

    /* skip to intersectTransitions for a description of what is done here */
    struct _intersect_cb_pack{
      std::stack<SubStateSet_t> *s_stack;
      std::map<SubStateSet_t, SubState_t*> *s_mappings;
      std::set<SubState_t*> *s_list;
      bool this_is_last_intersect;
      std::map<SubStateSet_t, SubState_t*> *tmp_mappings;
      std::map<SubState_t*, SubStateSet_t> *tmp_mappings_reverse;
    };
    static SubState_t* intersect_callback(SubState_t * const a,
                                          SubState_t * const b,
                                          void *info_v) {
      struct _intersect_cb_pack *info = (struct _intersect_cb_pack *)info_v;

      SubStateSet_t sset;
      // as a first step, try and expand the received parameters to
      // see if they are temporary values 
      typename std::map<SubState_t*, SubStateSet_t>::iterator r_iter;
      r_iter = info->tmp_mappings_reverse->find(a);
      if(r_iter != info->tmp_mappings_reverse->end())
        sset.insert(r_iter->second.begin(), r_iter->second.end());
      else
        sset.insert(a);
      r_iter = info->tmp_mappings_reverse->find(b);
      if(r_iter != info->tmp_mappings_reverse->end())
        sset.insert(r_iter->second.begin(), r_iter->second.end());
      else
        sset.insert(b);

      // [pseudocode]
      // now 'sset' contains 'union( expand(a), expand(b) )'
      SubStateSet_t sset_complete = followDeltas(sset);

      // check if it has been seen already
      typename std::map<SubStateSet_t, SubState_t*>::iterator m;
      SubState_t *m_target = NULL;
      
      m = info->s_mappings->find(sset_complete);
      if (m == info->s_mappings->end()) {
        // search in the temporary mappings
        m = info->tmp_mappings->find(sset_complete);
        if (m == info->tmp_mappings->end())
          ; // OK, do nothing
        else
          m_target = m->second;
      }
      else 
        m_target = m->second;

      // now m_target is NULL if this stateset was never seen,
      // otherwise it contains a reference to the equivalent state in
      // the determinized ExtendedTransition

      if (!info->this_is_last_intersect) {
        // if this is not the last intersection, just set up some
        // temporary mappings and return
        if (!m_target) {


          m_target = new SubState_t();
          (*info->tmp_mappings)[sset_complete] = m_target;
        }
        // always make sure that we've got a reverse mapping at hand
        // (will silently fail if the reverse mapping is there already)
        (*info->tmp_mappings_reverse)[m_target] = sset_complete;
        return m_target;
      }
      // ... otherwise complete the SubState_t* merge!
      
      if (!m_target) {
        // create the target if not seen and add it in every caching variable
        m_target = new SubState_t();
        (*info->s_mappings)[sset_complete] = m_target;
        info->s_stack->push(sset_complete);
        info->s_list->insert(m_target);
      } else if (info->s_mappings->find(sset_complete) == info->s_mappings->end()) {
        // also, if the target was seen, but was only cached in the temporary mappings,
        // bring it into the permanent caching variables
        (*info->s_mappings)[sset_complete] = m_target;
        info->s_stack->push(sset_complete);
        info->s_list->insert(m_target);
      }

      return m_target;
    }
    
    /* Perform the intersection between the provided transitions and
     * return it. To achieve this, the callback and the struct defined
     * just above are used heavily.
     *
     * 'states_stacks', 'mappings' and 'states_list' are received from
     * the determinization algorithm and are updated/used accordingly
     * to their meaning.
     *
     * 'ahead_states' is a set of 'static' states, that have to be
     * intersected with all the results of the intersections. This is
     * faked here with a trick (see the code). Backtrack to
     * determinize() for a broader explanation about them.
     *
     * This method has a semantic that is similar to the one that the
     * method 'assignTruthValuesAndEvaluate' used to have.
     */
    static SubTrans_t* intersectTransitions // typedef Range<VType, SubState_t*> SubTrans_t;  

    (SubTransSet_t trans_set,
     const SubStateSet_t ahead_states,
     std::stack<SubStateSet_t> *states_stack,
     std::map<SubStateSet_t, SubState_t*> *mappings,
     std::set<SubState_t*> *states_list
     ) {
      SubTrans_t *result = NULL;
      struct _intersect_cb_pack info;
      info.s_stack = states_stack;
      info.s_mappings = mappings;
      info.s_list = states_list;
      info.this_is_last_intersect = false;
      info.tmp_mappings = new std::map<SubStateSet_t, SubState_t*>;
      info.tmp_mappings_reverse = new std::map<SubState_t*, SubStateSet_t>;
      /* A few words about this last three variables.
       *
       * The intersection might be used to join several
       * transitions. The intermediate steps generate temporary
       * AType(s) (= SubState_t*) that will be joined again in the
       * following round. So, letting the callback pollute the general
       * s_mapping with short-lived mappings is a bad idea. Therefore,
       * the callback will store the temporary values needed for these
       * intermediate steps in 'tmp_mappings'. When
       * 'this_is_last_intersect' will become true, the callback will
       * stop using them and will switch to use permanent values
       * instead.
       * 
       * 'tmp_mappings_reverse' is just a reverse lookup map, to fully
       * expand (eventually) temporary SubState_t(s) to the full set.
       *
       * Note that, if trans_set contains only two transitions,
       * tmp_mappings will never get filled.
       */

      // ahead_states trick: fake transitions!
      for (typename SubStateSet_t::iterator i = ahead_states.begin();
           i != ahead_states.end();
           ++i)
        trans_set.insert(new SubTrans_t(*i)); 

      const int trans_count = trans_set.size();
      int loopno = 1;
      for (typename SubTransSet_t::iterator tr = trans_set.begin();
           tr != trans_set.end();
           ++tr, ++loopno)
      {
        if (loopno == trans_count)
          info.this_is_last_intersect = true;

        if (result)
          result = SubTrans_t::intersect(result, *tr, &intersect_callback, &info); 
        else
          result = new SubTrans_t(*tr);
      }
      // Quick fix: if only a transition was analyzed, nothing was done.
      // So manually scan its output gates, duplicate them and fill the variables.
      // Then notify  the transition itself.
      if (trans_count == 1) {
        SubStateSet_t out_gates = result->findAll();
        map<SubState_t*, SubState_t*> old2new;
        for(typename SubStateSet_t::iterator ii = out_gates.begin();
            ii != out_gates.end();
            ++ii) {
          SubStateSet_t sset;
          sset.insert(*ii);

          // check if the output gate has been seen already
          typename std::map<SubStateSet_t, SubState_t*>::iterator m_iter = 
            mappings->find(sset);
          if(m_iter == mappings->end()) {
            // if not, create a new mapping
            SubState_t *tmp = new SubState_t(*ii, false);
            (*mappings)[sset] = tmp;
            states_stack->push(sset);
            states_list->insert(tmp);
            old2new[*ii]=tmp;
          }
          else {
            // otherwise, reuse the mapping created previously
            old2new[*ii] = m_iter->second;
          }
        } // end of loop on out_gates
        // change the pointers inside the transition itself
        result->changeActions(old2new);
      }

      delete info.tmp_mappings;
      delete info.tmp_mappings_reverse;
      return result;
    }

    // proxy callbacks for walk()
    struct _proxy_cb_t{
      void (*cb_newrange)(RangeOperator_t, uint32, void*);
      void (*cb_newpunct)(RangeOperator_t, const map<uint32,unsigned long> &, void*);
      void (*cb_newaction)(unsigned long, void*);
      void (*cb_newspecial)(RangeOperator_t, SPType, void*);
      void *ptr;
    };
    static void cb_newrange_proxy(RangeOperator_t r, uint32 sep, void *ptr){
      struct _proxy_cb_t *info = (struct _proxy_cb_t *)ptr;
      (*info->cb_newrange)(r, sep, info->ptr);
    }
    static void cb_newpunct_proxy(RangeOperator_t r, const map<uint32,SubState_t*> &mappings, void *ptr) {
      struct _proxy_cb_t *info = (struct _proxy_cb_t *)ptr;
      map<uint32,unsigned long> proxy_map;
      for (typename map<uint32,SubState_t*>::const_iterator i = mappings.begin();
           i != mappings.end();
           ++i)
        proxy_map[i->first] = (unsigned long) i->second;
      (*info->cb_newpunct)(r, proxy_map, info->ptr);
    }
    static void cb_newaction_proxy(SubState_t* s, void *ptr) {
      struct _proxy_cb_t *info = (struct _proxy_cb_t *)ptr;
      (*info->cb_newaction)((unsigned long)s, info->ptr);
    }
    static void cb_newspecial_proxy(RangeOperator_t r, std::string val, void *ptr){
      struct _proxy_cb_t *info = (struct _proxy_cb_t *)ptr;
      (*info->cb_newspecial)(r, val, info->ptr);
    }

    /* type and callbacks to write ExtendedTransitions on DOT files */
    struct _dot_cb_pack {
      ostream *s;
      stack<string> *src_state; // name of the parent state in the DOT graph
      stack<string> *local_state_stub; // stub for the name of the local state
      stack<string> *tr_label; // label to write on the current incoming arc
    };
    static void dot_cb_range(RangeOperator_t r, uint32 sep, void *ptr){
      struct _dot_cb_pack *info = (struct _dot_cb_pack *)ptr;

      // retrieve info
      string src_state = info->src_state->top();
      info->src_state->pop();
      string local_stub = info->local_state_stub->top();
      info->local_state_stub->pop();
      string tr_label = info->tr_label->top();
      info->tr_label->pop();

      // generate a local state
      string local_state = local_stub + string("r");
      std::stringstream myconv;
      myconv << SubTrans_t::rangeOp2shortStr(r) << " " << sep;
      string local_state_label(myconv.str());

      // output
      (*info->s) << "\t" << local_state << "[label=\"" << local_state_label <<
        "\",heigth=0.3,shape=box,color=blue,fontcolor=blue];" << std::endl;
      (*info->s) << "\t" << src_state << " -> " << local_state <<
        "[label=\"" << tr_label << "\",color=blue,fontcolor=blue];" << endl;

      // push info for child nodes (yes, push things twice)
      info->src_state->push(local_state);
      info->src_state->push(local_state);
      info->local_state_stub->push(local_state);
      info->local_state_stub->push(local_state);
      info->tr_label->push(string("FALSE"));
      info->tr_label->push(string("TRUE"));
    }
    static void dot_cb_special(RangeOperator_t r, SPType val, void *ptr){
      struct _dot_cb_pack *info = (struct _dot_cb_pack *)ptr;

      // retrieve info
      string src_state = info->src_state->top();
      info->src_state->pop();
      string local_stub = info->local_state_stub->top();
      info->local_state_stub->pop();
      string tr_label = info->tr_label->top();
      info->tr_label->pop();

      // generate a local state
      string local_state = local_stub + string("r");
      std::stringstream myconv;
      myconv << SubTrans_t::rangeOp2shortStr(r) << " \'" << val << "\'";
      string local_state_label(myconv.str());

      // output
      (*info->s) << "\t" << local_state << "[label=\"" << local_state_label <<
        "\",width=1.5, heigth=0.3,shape=box,color=blue,fontcolor=blue];" << std::endl;
      (*info->s) << "\t" << src_state << " -> " << local_state <<
        "[label=\"" << tr_label << "\",color=blue,fontcolor=blue];" << endl;

      // push info for child nodes (yes, push things twice)
      info->src_state->push(local_state);
      info->src_state->push(local_state);
      info->local_state_stub->push(local_state);
      info->local_state_stub->push(local_state);
      info->tr_label->push(string("FALSE"));
      info->tr_label->push(string("TRUE"));
    }
    static void dot_cb_punct(RangeOperator_t r, const map<uint32,SubState_t*> &mappings, void *ptr) {
    
      struct _dot_cb_pack *info = (struct _dot_cb_pack *)ptr;

      // retrieve info
      string src_state = info->src_state->top();
      info->src_state->pop();
      string local_stub = info->local_state_stub->top();
      info->local_state_stub->pop();
      string tr_label = info->tr_label->top();
      info->tr_label->pop();

      // generate a local state
      string local_state = local_stub + string("p");
      string local_state_label(SubTrans_t::rangeOp2shortStr(r));

      // output the "main" state
      (*info->s) << "\t" << local_state << "[label=\"" << local_state_label <<
        "\",heigth=0.2,shape=box,color=blue,fontcolor=blue];" << std::endl;
      (*info->s) << "\t" << src_state << " -> " << local_state <<
        "[label=\"" << tr_label << "\",color=blue,fontcolor=blue];" << endl;

      // output another link for each "little" state of each mapping
      for(typename map<uint32,SubState_t*>::const_iterator i = mappings.begin();
          i != mappings.end();
          ++i) {
        std::stringstream myconv;
        myconv << i->first;
        string lil_tr_label(myconv.str());

        (*info->s) << "\t" << local_state << " -> st_" << i->second <<
          "[label=\"" << lil_tr_label << "\",color=blue,fontcolor=blue];" << endl;
      }

      // push info for the default child node
      info->src_state->push(local_state);
      info->local_state_stub->push(local_state);
      info->tr_label->push(string("default"));
    }
    static void dot_cb_action(SubState_t* s, void *ptr) {
      struct _dot_cb_pack *info = (struct _dot_cb_pack *)ptr;

      // retrieve info
      string src_state = info->src_state->top();
      info->src_state->pop();
      info->local_state_stub->pop();
      string tr_label = info->tr_label->top();
      info->tr_label->pop();

      (*info->s) << "\t" << src_state << " -> st_" << s <<
        "[label=\"" << tr_label << "\",color=blue,fontcolor=blue];" << endl;
    }

    /*
     * Internal function to dump on a DOT file the internals of an
     * ExtendedTransition, recursively, from the given internal state.
     *
     * Use the macro below in most of the cases; please refer to
     * prettyETDotPrint for the public API.
     */
#ifdef ENABLE_PFLFRONTEND_DBGFILES
#define PRINT_ET_DOT_INTERNAL(root,desc,filename) do {                  \
      ExtendedTransition::ETdumpToStream((root),(desc),(filename),__FILE__,__LINE__); \
    } while (0)
#else
#define PRINT_ET_DOT_INTERNAL(root,desc,filename)
#endif /* #ifdef ENABLE_PFLFRONTEND_DBGFILES */

    static void ETdumpToStream(SubState_t *root_state, const char *description, const char *dotfilename,
                               const char *src_file = NULL, int src_line = -1) {
      static int dbg_incremental_id = 0;

      ++dbg_incremental_id;
      if (src_file != NULL && src_line != -1){
        cerr << setfill('0');
        cerr << "MINI-" << description << ": MINI-" << setw(3) << dbg_incremental_id << " (@ "
             << src_file << " : " << src_line << ")" << endl;
      }

      std::stringstream myconv;
      myconv << setfill('0');
      myconv << setw(3) << dbg_incremental_id;
      std::string fname = string("dbgmini_") + myconv.str() +
        string("_") + string(dotfilename) + string(".dot");
      std::ofstream outfile(fname.c_str());

      ExtendedTransition::ETdumpToStream_r(outfile,root_state);
    }

    // the actual substate dumper for the above method
    static void ETdumpToStream_r(std::ostream &s, SubState_t *root_state){
      s << "Digraph fsa{" << std::endl;
      s << "rankdir=LR" << endl;
      s << "edge[fontsize=10];" << endl;
      s << "node[fontsize=11, fixedsize=true, shape=circle, width=0.65,heigth=0.65,color=blue,fontcolor=blue];" << endl;

      std::stack<SubState_t*> to_visit;
      std::set<SubState_t*> visited;

      to_visit.push(root_state);
      while(!to_visit.empty()) {
        SubState_t *tos = to_visit.top();
        to_visit.pop();

        if(visited.find(tos) != visited.end())
          continue;

        visited.insert(tos);

        s << "\tst_" << tos << "[label=\"" << (tos->is_input_gate? "T" : "F") <<
          (tos->is_output_gate? "T" : "F") << "\",width=0.3,heigth=0.3,color=blue,fontcolor=blue];" << std::endl;

        // dump the "normal" transitions...
        int incr = 0;
        for(typename std::set<SubTrans_t*>::iterator t = tos->out_trans.begin();
            t != tos->out_trans.end();
            ++t) {
          std::stringstream myconv;
          myconv << "st_" << (tos);
          string start_state_name(myconv.str());
          std::stringstream myconv1;
          myconv1 << "st_" << (tos) << "_" << (++incr);
          string state_name_stub(myconv1.str());
          std::stringstream myconv2;
          myconv2 << (tos)->next_checked_attr;
          string first_label(myconv2.str());

          struct _dot_cb_pack info;
          info.s = &s;
          info.src_state = new stack<string>();
          info.local_state_stub = new stack<string>();
          info.tr_label = new stack<string>();

          info.src_state->push(start_state_name);
          info.local_state_stub->push(state_name_stub);
          info.tr_label->push(first_label);

          (*t)->traverse(dot_cb_range, dot_cb_punct, dot_cb_action, dot_cb_special, &info);

          delete info.src_state;
          delete info.local_state_stub;
          delete info.tr_label;

          std::set<SubState_t*> targets = (*t)->findAll();
          for(typename std::set<SubState_t*>::iterator tt = targets.begin();
              tt != targets.end();
              ++tt) {
            if(visited.find(*tt) == visited.end())
              to_visit.push(*tt);
          }
        }

        // ...and the epsilon transitions
        for(typename std::set<SubState_t*>::iterator j = tos->delta_targets.begin();
            j != (tos)->delta_targets.end();
            ++j) {
          s << "\tst_" << (tos) << " -> st_" << (*j) <<
            "[label=\"epsilon\",color=blue,fontcolor=blue];" << std::endl;
          if(visited.find(*j) == visited.end())
            to_visit.push(*j);
        }
      }

      s << "}" << std::endl;
    }


  public:
    // ~~ public methods ~~
    
    void RemoveEdge(Edge_t edge){
    	for(typename std::list<Edge_t>::iterator it = edges.begin(); it != edges.end(); it++){
    		if((*it) == edge){
    			edges.erase(it);
    			break;
    		}    						
    	}
    }
    
    set<AType> GetPred()
	{
		return checked_attrs;
	}

    /*
     * Perform the union of the current ExtendedTransition with a
     * 'shortcut' directly leading to the provided State*. This is
     * internally achieved with a epsilon-transition from the starting
     * node to the correct gate.
     */
    void unionWithoutPredicate(State* out_gate, Edge_t equiv_edge) {
      // check if the gate is already among those currently available, or add them
      SubState_t *gate;
      typename std::map<State*, SubState_t*>::iterator tmp;

      tmp = output_gates.find(out_gate);
      if (tmp == output_gates.end()){
        gate = new SubState_t(out_gate, false, true);
        states.insert(gate);
        output_gates[out_gate]=gate;
      }
      else
        gate = tmp->second;

      // connect the gate
      this->startingNode->delta_targets.insert(gate);

      // save equivalent edge
      edges.push_back(equiv_edge);
    }

    //copy costructor
    ExtendedTransition(ExtendedTransition *other) 
      : Transition(*(other->Owner), 0, other->Labels, false),
        edges(other->edges), transitions(other->transitions), checked_attrs(other->checked_attrs)
    { 
       // I need to copy the states, otherwise when I use the pointers
      // I will change the 'other' ones as well
      this->related_et=this;
      this->startingNode=NULL;

      map<SubState_t *,SubState_t *> old2new;
      for(typename std::set<SubState_t*>::iterator s = other->states.begin();
          s != other->states.end();
          ++s) {
        SubState_t *new_s = this->copyInsert(*s); 

        old2new[*s]=new_s;
        if((*s)->is_input_gate)
          this->startingNode = new_s;
        if((*s)->is_output_gate)
          this->output_gates[(*s)->statePtr] = new_s;
      }

      changeStateRefs(this->states, old2new);
    }

    virtual bool IsExtended(void){
      return true;
    }

    void AddLabel(LType label)
    {
      nbASSERT(false,"You CANNOT add new labels to this ExtendedTransition");
    }

    void AddLabels(Alphabet_t labels)
    {
      nbASSERT(false,"You CANNOT add new labels to this ExtendedTransition");
    }

    State* FromState(void)
    {
      return this->startingNode->statePtr;
    }

    /*
     * This method should never be called, as there is no 'single' landing
     * state
     */
    StateIterator ToState(void)
    {
      nbASSERT(false, "This function should never be called");
      return NULL; // see comment
    }

    std::list<StateIterator> ToStates(void)
    {
      std::list<StateIterator> landing_states;
      for (typename std::list<Edge_t>::iterator i = edges.begin(); i != edges.end(); i++){
        typename Graph_t::iterator state_iter = this->Owner->m_Graph.arc_to(*i);
        landing_states.push_back(StateIterator(this->Owner->m_Graph, state_iter));
      }
      return landing_states;
    }

    void changeGateRefs(sftFsa *new_owner, State *new_ingate, std::map<State*, State*> changed_outgates)
    {
      // handle new owner (if desired)
      if (new_owner)
        this->Owner = new_owner;

      // handle input gate (if desired)
      if (new_ingate)
        this->startingNode->statePtr = new_ingate;

      // handle changed output gates (if any)
      for (typename std::map<State*, State*>::iterator i = changed_outgates.begin();
           i != changed_outgates.end();
           i++) 
      {
        typename std::map<State*, SubState_t*>::iterator old_gate_ref =
          this->output_gates.find(i->first);

        if(old_gate_ref == this->output_gates.end())
          continue; // this change does not bother us

        typename std::map<State*, SubState_t*>::iterator candidate_ref =
          this->output_gates.find(i->second);
        if(candidate_ref != this->output_gates.end()) {
          // If i->second has already a mapping in output_gates, reuse that SubState.
          // Forget about the substate that is mapped now and re-route all the transitions
          states.erase(old_gate_ref->second);
          
          map<SubState_t *,SubState_t *> refchange;
          refchange[old_gate_ref->second] = candidate_ref->second;
          changeStateRefs(states, refchange);

          continue;
        }

        // change the information in the node
        SubState_t *iter_to_node = old_gate_ref->second;
        iter_to_node->statePtr = i->second;
        
        // delete old mapping and save new one
        output_gates.erase(old_gate_ref);
        output_gates[i->second] = iter_to_node;
        }
    }

    void wipeEdgeRefs(){
      this->edges.clear();
    }

    void addEdgeRef(TransIterator tref){
      this->edges.push_back( (*tref).Edge );
    }

    void changeEdgeRef(TransIterator old_t, TransIterator new_t){
      Edge_t old_edge = (*old_t).Edge, new_edge = (*new_t).Edge;

      this->edges.remove(old_edge);
      
      this->edges.push_back(new_edge);
    }

    void dumpToStream(std::ostream &s,string label){
      // dump all the subautomaton internal states
      for (typename std::set<SubState_t*>::iterator i = states.begin(); i != states.end();i++)
      {
        s << "\tst_" << (*i) << "[label=\"" << ((*i)->is_input_gate? "T" : "F") << ((*i)->is_output_gate? "T" : "F") << "\",width=0.3,heigth=0.3,color=blue,fontcolor=blue];" << std::endl;
        if ( (*i)->is_input_gate ) 
    	    s << "\tst_" << (*i)->statePtr->GetID() << " -> st_" << (*i) <<"[label=\"" << label << "\"];" << std::endl;
        if ( (*i)->is_output_gate ) 
        {
            s << "\tst_" << (*i) << " -> st_" << (*i)->statePtr->GetID() << "[label=\"\"];" << std::endl;
        }

        // dump the "normal" transitions...
        int incr = 0;
        for(typename std::set<SubTrans_t*>::iterator t = (*i)->out_trans.begin();
            t != (*i)->out_trans.end();
            ++t) {
          std::stringstream myconv;
          myconv << "st_" << (*i);
          string start_state_name(myconv.str());
          std::stringstream myconv1;
          myconv1 << "st_" << (*i) << "_" << (++incr);
          string state_name_stub(myconv1.str());
          std::stringstream myconv2;
          myconv2 << (*i)->next_checked_attr;
          string first_label(myconv2.str());

          struct _dot_cb_pack info;
          info.s = &s;
          info.src_state = new stack<string>();
          info.local_state_stub = new stack<string>();
          info.tr_label = new stack<string>();

          info.src_state->push(start_state_name);
          info.local_state_stub->push(state_name_stub);
          info.tr_label->push(first_label);

          (*t)->traverse(dot_cb_range, dot_cb_punct, dot_cb_action, dot_cb_special, &info); 

          delete info.src_state;
          delete info.local_state_stub;
          delete info.tr_label;
        }

        // ...and the epsilon transitions
        for(typename std::set<SubState_t*>::iterator j = (*i)->delta_targets.begin();
            j != (*i)->delta_targets.end();
            ++j)
          s << "\tst_" << (*i) << " -> st_" << (*j) <<
            "[label=\"epsilon\",color=blue,fontcolor=blue];" << std::endl;
      }
    }

    // Print the internal representation of the ExtendedTransition as
    // a DOT file. Please use the macro.
#ifdef ENABLE_PFLFRONTEND_DBGFILES
#define PRINT_ET_DOT(ext_trans,desc,filename) do {                      \
      (ext_trans)->prettyETDotPrint((desc),(filename),__FILE__,__LINE__); \
    } while (0)
#else
#define PRINT_ET_DOT(ext_trans,desc,filename)
#endif /* #ifdef ENABLE_PFLFRONTEND_DBGFILES */

    void prettyETDotPrint(const char *description, const char *dotfilename,
                          const char *src_file = NULL, int src_line = -1){
      ExtendedTransition::ETdumpToStream(this->startingNode,
                                         description,
                                         dotfilename,
                                         src_file,
                                         src_line);
    }

    // ~overloaded operators~
    const ExtendedTransition operator+(const ExtendedTransition &other){
      ExtendedTransition result(*this);
      nbASSERT(false,"FIXME actually implement the copy constructor");
      result += other;
      return result;
    }

    const ExtendedTransition& operator+=(const ExtendedTransition &foo){
      ExtendedTransition &other = const_cast<ExtendedTransition &>(foo); // DARK: this is a hack, but stlplus library handles badly (LIKE SHIT) 'const'

      // sanity checks
      if ((this->Owner) != (other.Owner) ||
          (this->Labels.size() != 1) ||
          (this->Labels != other.Labels)
          )
        return *this; // we cannot merge those!

      PRINT_ET_DOT_INTERNAL(this->startingNode, "this subfsa at the begin of the += operator", "subfsa_plusequal_this");
      PRINT_ET_DOT_INTERNAL(other.startingNode, "other subfsa at the begin of the += operator", "subfsa_plusequal_other");

      // ~~ first, see who is going first and save this information
      const int who_goes_first = (*this->checked_attrs.begin()).compare( *(other.checked_attrs.begin()) ); //compares the attribute to see which is the first one

      // ~~ merge the attributes (simple)
      this->checked_attrs.insert(other.checked_attrs.begin(), other.checked_attrs.end());

      // ~~ merge the output gates (more difficult). We need more data structures...
      /* This map associates an iterator pointing to a node in 'other' to the iterator pointing 
       * to the corresponding node in 'this'.
       * It is needed to correctly redraw all the transitions.
       */
      std::map<SubState_t*, SubState_t*> old2new;
      for(typename std::map<State*, SubState_t*>::iterator gate_ptr = other.output_gates.begin();
          gate_ptr != other.output_gates.end();
          gate_ptr ++) {
        // is this output gate shared between both ExtendedTransitions?
        typename std::map<State*, SubState_t*>::iterator shared = 
          this->output_gates.find(gate_ptr->first);

        if (shared == this->output_gates.end()){ 
          // this output gate is not shared among Extended Transitions.
          SubState_t *node = this->copyInsert(gate_ptr->second); // add it...
          // ... save its reference among the local list of gates ...
          this->output_gates[gate_ptr->first]=node;
          // ... and save it in our temporary data structure!
          old2new[gate_ptr->second]=node;
        }
        else{
          // good news! it is already shared!
          // therefore just save the mapping in the local data structure
          old2new[gate_ptr->second]=shared->second;
        }
      }

      PRINT_ET_DOT_INTERNAL(this->startingNode, "this subfsa before states", "subfsa_plusequal_beforestates");

      // ~~ merge states
      for(typename std::set<SubState_t*>::iterator state_iter = other.states.begin();
          state_iter != other.states.end();
          ++state_iter){
        if(!(*state_iter)->is_input_gate && !(*state_iter)->is_output_gate){
          // internal states are always merged
          SubState_t *node = this->copyInsert(*state_iter);
          old2new[*state_iter] = node;
        } else {
          // if the state is a gate (either input or output), check carefully
          if( (*state_iter)->is_input_gate &&
              old2new.find(*state_iter) == old2new.end() ){
            /* if this is the input gate of the 'other' ExtendedTransition AND if it wasn't
             * already handled before, during the output gates phase
             * (that might happen when the ext transition loops on the
             * same state, because of a loop in the encap graph, like
             * IP in IP)
             */

            if (who_goes_first < 0){
              /* the first check of 'this' comes before the first check of 'other', so add
               * the input gate of 'other' as an internal node of the subautomaton
               */
              SubState_t *node = this->copyInsert(*state_iter);
              old2new[*state_iter] = node;
              this->startingNode->delta_targets.insert(node);

              // demote the 'node'
              node->is_input_gate = false;
              node->statePtr = NULL;
            } else if (who_goes_first > 0){
              /* the first check of 'this' comes after the first check of 'other', so add
               * the input gate of 'other' as the new input gate and demote the current one to 
               * an internal node
               */
              SubState_t *node = this->copyInsert(*state_iter);
              old2new[*state_iter] = node;
              node->delta_targets.insert(this->startingNode);

              // demote to internal node
              this->startingNode->is_input_gate = false;
              this->startingNode->statePtr = NULL;

              // promote the node just added
              this->startingNode = node; // promote the node just added
            } else {
              /* the first check of 'this' is the same of 'other', so bring both checks in parallel
               * (no need to insert a new input gate, just reuse the existing one)
               */
              old2new[*state_iter]=this->startingNode;
              
              // I'll fix state references in the next step
              this->startingNode->out_trans.insert((*state_iter)->out_trans.begin(),
                                                   (*state_iter)->out_trans.end());
              this->startingNode->delta_targets.insert((*state_iter)->delta_targets.begin(),
                                                       (*state_iter)->delta_targets.end());
            }
          }
          // else do nothing, output gates have already been merged
        }
      }

      PRINT_ET_DOT_INTERNAL(this->startingNode, "this subfsa before trans", "subfsa_plusequal_beforetrans");

      // in the end, merge transitions, by changing the references stored already
      changeStateRefs(this->states, old2new); 

      // copy also the list of underlying edges
      this->edges.insert(this->edges.begin(), other.edges.begin(), other.edges.end());

      PRINT_ET_DOT_INTERNAL(this->startingNode, "subfsa at the end of the += operator", "subfsa_plusequal_end");
      return *this;
    }

    /* Proceed to the determinization of the current ExtendedTransition
     * and return to the caller, via the pointer, a mapping that later can be reused
     * to correctly refill the statePtr(s) inside all the SubState_t(s)
     */
    void determinize(std::map<std::set<State*>, State*> *det_tokens){
      std::stack<SubStateSet_t> state_stack;
      // mappings FROM the state sets of the old subautomaton TO the states of the new one
      std::map<SubStateSet_t, SubState_t*> mappings;
      std::set<SubState_t*> tmp_states_list;
      typename std::map<State*, SubState_t*> tmp_output_gates;

      // safety check
      nbASSERT(this->startingNode, "starting node is NULL, something broke");
      
      // init
      SubStateSet_t first = followDeltas(this->startingNode);       
      SubState_t *first_st = new SubState_t(this->startingNode, false);
      tmp_states_list.insert(first_st);
      mappings[first] = first_st;
      state_stack.push(first);


      PRINT_ET_DOT_INTERNAL(this->startingNode, "subfsa to determinize", "subfsa_to_determ");
      // I'll use it with reasonably low values, hoping that that there's no clash
      State* tmp_token_ptr = NULL;
      // main loop
      while(!state_stack.empty()){
        PRINT_ET_DOT_INTERNAL(first_st,"tmp subfsa inside loop", "tmp_loop");
        const SubStateSet_t tos = state_stack.top();
        state_stack.pop();
        const typename std::map<SubStateSet_t, SubState_t*>::iterator m = mappings.find(tos);
        
        nbASSERT(m!=mappings.end(), "tos should already have a corresponding state");
        SubState_t *DFAd_state = m->second;

        // take the lesser attribute
        
        const typename std::set<AType>::iterator lesser_attribute = takeLesserAttribute(tos, this->checked_attrs);
        
        if (lesser_attribute == checked_attrs.end()){
          // if there's no lesser attribute, the stateset is made only of out gates
          DFAd_state->is_output_gate = true; // so set it as an out gate
          DFAd_state->statePtr = ++tmp_token_ptr; // generate a (unique?) token value
          tmp_output_gates[tmp_token_ptr] = DFAd_state;
          // add those data into the det_tokens structure that will be used during determinization
          std::set<State*> stateset;
          for (typename SubStateSet_t::const_iterator s = tos.begin();
               s != tos.end();
               ++s)
            stateset.insert((*s)->statePtr); // gather all the State* reached by these gates 
           

          if (det_tokens->find(stateset) != det_tokens->end()) {
            // The current set of substates has no duplicates, that is
            // was never encountered before; but the set of external
            // states that the determinized substate will point to
            // has, instead, already encountered and handled by
            // another substate. Here I delete the current substate
            // and update all the data structures to use the old one,
            // instead.

            // retrieve the old values
            State* old_token = det_tokens->find(stateset)->second;
            SubState_t* old_DFAd_state = tmp_output_gates[old_token];

            // delete the old references
            tmp_states_list.erase(DFAd_state);

            // change to the old substate value
            map<SubState_t *,SubState_t *> refchange;
            refchange[DFAd_state] = old_DFAd_state;
            changeStateRefs(tmp_states_list, refchange);

            SubStateSet_t tmp = m->first;
            mappings.erase(m);
            mappings[tmp] = old_DFAd_state;

            continue;
          }

          (*det_tokens)[stateset]=tmp_token_ptr;
          continue;
        }

        PRINT_ET_DOT_INTERNAL(first_st,"tmp subfsa middle loop", "tmp_loop_middle");

        // analyze the transitions with the lesser attribute
        SubTransSet_t exit_transitions = grabExitingTransWithAttribute(tos, lesser_attribute); 
        nbASSERT(!exit_transitions.empty(), "No transition retrieved?");
        
        // set the next checked attribute...
        DFAd_state->next_checked_attr = *lesser_attribute;
        // ... and "determinize" the transitions.
        // Since the control might be in multiple starting states, not all of them might
        // have an exiting transition among 'exit_transitions'. This means that some paths
        // in the subautomaton are already 'slightly ahead' and, since we check one attribute at
        // a time, they have to 'wait' for the control to reach their checked attribute.
        // For the determinization to work, they must also be considered as well in the
        // destination statesets that this ET will reach. We call them 'ahead_states'.
        const SubStateSet_t ahead_states = grabAheadStates(tos, lesser_attribute);
        DFAd_state->out_trans.insert(  
          intersectTransitions(exit_transitions, ahead_states,
                               &state_stack, &mappings, &tmp_states_list)
          );
      } // end of: while(!state_stack.empty())

      PRINT_ET_DOT_INTERNAL(first_st,"tmp subfsa at loop end", "tmp_end");

      this->startingNode = first_st;
      this->states = tmp_states_list;
      
      this->output_gates = tmp_output_gates;
      // end of determinization
      PRINT_ET_DOT_INTERNAL(this->startingNode, "this->subfsa before return", "determinize_end");
    } //end of method

    // -- a couple of methods used in the code generation phase

    /*
     * Returns a map with a few information about the substates:
     * a mapping from an ID to the (possible) external State*
     * that the substate references.
     */
    map<unsigned long, State*> getSubstatesInfo() {
      map<unsigned long, State*> to_ret;

      for(typename std::set<SubState_t*>::iterator s_iter = states.begin();
          s_iter != states.end();
          ++s_iter)
        to_ret[(unsigned long)*s_iter] = ( (*s_iter)->is_output_gate ? (*s_iter)->statePtr : NULL );

      return to_ret;
    }

    /*
     * Walk the current ExtendedTransition with the provided callbacks, using the underlying library
     * and its callback mechanism.
     *
     * cb_newtrans: called at the beginning of each sub-transition
     * cb_newrange: called everytime there is a binary fork. The true branch will be taken first,
     *              recursively, then the false one.
     * cb_newpunt: called when there are multiple 'punctual' choices
     * cb_newaction: called once for each of the states that terminate a transition.
     *
     * (internal note: I'll use proxy callbacks)
     */
    void walk(void (*cb_newtrans)(unsigned long, AType, void*),
              void (*cb_newrange)(RangeOperator_t, uint32, void*),
              void (*cb_newpunct)(RangeOperator_t, const map<uint32,unsigned long> &, void*),
              void (*cb_newaction)(unsigned long, void*),
              void (*cb_newspecial)(RangeOperator_t, SPType, void*),
              void *ptr) {
      // set up proxy methods
      struct _proxy_cb_t info;
      info.cb_newrange = cb_newrange; //this is a method
      info.cb_newpunct = cb_newpunct; //this is a method
      info.cb_newaction = cb_newaction; //this is a method
      info.cb_newspecial = cb_newspecial; //this is a method
      info.ptr = ptr;
      
      // be sure to start from the startingNode
      nbASSERT(startingNode->out_trans.size() == 1, "This is not a DFA");
      nbASSERT(startingNode->delta_targets.size() == 0, "This is not a DFA");
      
      (*cb_newtrans)((unsigned long)startingNode, startingNode->next_checked_attr, ptr);
      (*(startingNode->out_trans.begin()))->traverse(cb_newrange_proxy, cb_newpunct_proxy, cb_newaction_proxy, cb_newspecial_proxy, &info); //traversal is defined in librange/internal.hpp

      // then walk among the other ones (the order does not matter, actually)
      for(typename std::set<SubState_t*>::iterator s = states.begin();
          s != states.end();
          ++s){
        if(*s == 
        startingNode)
          continue; // skip, done already
        if((*s)->is_output_gate)
          continue; // we do not walk on these

        nbASSERT((*s)->out_trans.size() == 1, "This is not a DFA");
        nbASSERT((*s)->delta_targets.size() == 0, "This is not a DFA");
        (*cb_newtrans)((unsigned long)(*s), (*s)->next_checked_attr, ptr);
        (*((*s)->out_trans.begin()))->traverse(cb_newrange_proxy, cb_newpunct_proxy, cb_newaction_proxy, cb_newspecial_proxy, &info); //traverse is in internals.hpp
      }
    }

  }; // class ExtendedTransition


private:
  uint32			m_StateCount;		//!< Number of states present inside the FSA
  uint32			m_TransCount;		//!< Number of transitions present inside the FSA
  Graph_t			m_Graph;			//!< Underlying graph
  State				*m_InitialState;	//!< Initial state of the FSA
  StateSet_t		m_AcceptingStates;	//!< Set of Accepting States
  StateSet_t		m_ActionStates;		//!< Set of Action States
  StateSet_t		m_AllStates;		//!< Set of all the states of the FSA



  TransIterator AddTrans(StateIterator &from, StateIterator &to, Transition *trans);

  void DFSPreorder(State &state, StateList_t &sortedStates);
  void DFSReversePreorder(State &state, StateList_t &sortedStates);
  void DFSPostorder(State &state, StateList_t &sortedStates);
  void DFSReversePostorder(State &state, StateList_t &sortedStates);
  std::list<Transition*> getTransExitingFrom(State *state, LType label);
 

public:

  Alphabet_t		&m_Alphabet;		//!< Alphabet Set
  Instruction_t		m_Instruction1;		//!< Instruction Set 1
  Instruction_t		m_Instruction2;		//!< Instruction Set 2


  /*!
    \brief Constructor
  */  
  
  sftFsa(Alphabet_t &alphabet);


  /*!
    \brief Copy Constructor
  */
  sftFsa(sftFsa<SType, LType, PType, AType, VType, SPType, SFType, BType > &other);


  /*!
    \brief Destructor
  */
  virtual ~sftFsa();


  /*!
    \brief Add a state to the FSA
    \param info information associated to the state
    \return a reference to the newly created state
  */
  StateIterator AddState(SType info = s_traits::null());


  /*!
    \brief Add a state to the FSA, copying significant parameters from another state

    \param state - usually belonging to a different FSA - from which the parameters are copied
    \return a reference to the newly created state
  */
  StateIterator AddState(State &other);


  // removes a state from the automaton
  void removeState(State* state);


  /*!
    \brief Add a transition to the FSA
    \param from State Iterator of the starting endpoint of the transition
    \param to State Iterator of the ending endpoint of the transition
    \param lbl alphabet symbol associated to the transition
    \param complement if true the alphabet set associate to the transition should be complemented
    \param pred	boolean predicate associated to the transition (the default value is the unity, implying that the transition is always active)
    \return a reference to the newly created transition
  */
  TransIterator AddTrans(StateIterator &from, StateIterator &to,  LType lbl, bool complement = false, PType pred = p_traits::One());


  /*!
    \brief Add a transition to the FSA
    \param from State Iterator of the starting endpoint of the transition
    \param to State Iterator of the ending endpoint of the transition
    \param labels alphabet symbols associated to the transition
    \param complement if true the alphabet set associate to the transition should be complemented
    \param pred	boolean predicate associated to the transition (the default value is the unity, implying that the transition is always active)
    \return a reference to the newly created transition
  */
  TransIterator AddTrans(StateIterator &from, StateIterator &to,  Alphabet_t &labels, bool complement = false, PType pred = p_traits::One());


  /*!
  \brief Add a transition to the FSA, using information from another transition to build it
  \param from State Iterator of the starting endpoint of the transition
  \param to State Iterator of the ending endpoint of the transition
  \param other Transition from which relevant information is copied
  \return a reference to the newly created transition
  */
  TransIterator AddTrans(StateIterator from, StateIterator to, Transition &other);


  /*
   * Add a couple of transitions (with predicates: pred and !pred), backed up by the same ExtendedTransition.
   * The first one leads to 'to', the negated one to 'to_dfl'.
   */
   ExtendedTransition* addExtended(StateIterator &from, LType label, StateIterator &to, StateIterator &to_dfl, PType pred);

   ExtendedTransition* addExtended(StateIterator &from, LType label, StateIterator &to, StateIterator &to_dfl, VType, RangeOperator_t, AType);
  
   ExtendedTransition* addExtended(StateIterator &from, LType label, StateIterator &to, StateIterator &to_dfl, ExtendedTransition *other);

  /*!
    \brief Add a transition to the FSA
    \param from State Iterator of the starting endpoint of the transition
    \param to State Iterator of the ending endpoint of the transition
    \param pred	boolean predicate associated to the transition (the default value is the unity, implying that the transition is always active)
    \return a reference to the newly created transition
  */
  TransIterator AddEpsilon(StateIterator &from, StateIterator &to, PType pred = p_traits::One());


  /*!
    \brief Set the initial state of the FSA
    \param st state that will be set as initial for the FSA
  */
  void SetInitialState(StateIterator &st);

 /*!
 	\brief Set additional information to the action state(s) of the automaton
 	\param ei extra info to add to the action state(s)
 */
 void  SetExtraInfoToAction(list<SFType> ei);
 
 /*!
 	\brief Set additional information to the action state(s) of the automaton
 	\param eip extra info to add to the action state(s)
 */
 void  SetExtraInfoPlusToAction(list<VType> eip); 
 
  /*!
 	\brief Set additional information to the action state(s) of the automaton
 	\param oi extra info to add to the action state(s)
 */
 void  SetOtherInfoToAction(list<BType> oi);
 
 /*!
 	\brief Set additional information to the specified state
 	\param state state to set the ei 
 	\param ei extra info to add to the specified state
 */
 void  SetExtraInfoToState(State *state, list<SFType> ei);
 
 /*!
 	\brief Set additional information to the specified state
 	\param state state to set the ei 
 	\param eip extra info to add to the specified state
 */
 void  SetExtraInfoPlusToState(State *state, list<VType> eip);
 
  /*!
 	\brief Set additional information to the specified state
 	\param state state to set the ei 
 	\param oi extra info to add to the specified state
 */
 void  SetOtherInfoToState(State *state, list<BType> oi);
 
  /*!
    \brief Set a state as accepting
    \param st state that will be set as accepting
  */
  void setAccepting(StateIterator &st);
  void resetAccepting(StateIterator &st);
  void setAction(StateIterator &st);
  void resetAction(StateIterator &st);
  void setFinal(StateIterator &st);
  void resetFinal(StateIterator &st);

  /*!
    \brief Return the initial state of the FSA
    \return a reference to the initial state of the FSA
  */

  StateIterator GetInitialState(void);

  /*!
    \brief Create an iterator for the first state in the FSA
    \return a StateIterator
  */
  StateIterator FirstState(void);

  /*!
    \brief Create an iterator for the last state in the FSA
    \return a StateIterator
  */
  StateIterator LastState(void);

  /*!
    \brief Create an iterator for the first transition in the FSA
    \return a TransIterator
  */
  TransIterator FirstTrans(void);

  /*!
    \brief Create an iterator for the last transition in the FSA
    \return a TransIterator
  */
  TransIterator LastTrans(void);

 // retrieve all transitions exiting from a state
 std::list<Transition*> getTransExitingFrom(State *state);
 
 // retrieve all transitions entering in a state 
 std::list<Transition*> getTransEnteringTo(State *state);

  /*
   * For each mapping 'a' => 'b' provided, change all transitions leaving from or
   * going into 'a' to use 'b' instead, then delete 'a'.
   *
   * By default (to find interesting bugs), this method aborts if the
   * compacted states have different stateInfo. If you enable
   * 'clearStateInfoIfConflicting', instead, in that case this method
   * does not abort, but just clears stateInfo if different ones were
   * provided.
   */
  void compactStates(map<State*, State*> mappings, bool clearStateInfoIfConflicting = false);

  /*!
    \brief Mark all the states in the FSA as "non-visited"
  */
  void ResetVisited(void);


  StateList_t SortRevPostorder(void);
  
  StateList_t SortRevPostorderAction(void);
  
  StateList_t GetAllStates(void); 

  StateList_t SortPostorder(void);

  StateList_t SortPreorder(void);

  StateList_t SortRevPreorder(void);
  
  
  /*
  *	This method returns the accepting states of the automaton
  */
  StateList_t GetAcceptingStates(void)
   {
		StateList_t stateList;
		typename StateSet_t::iterator it = m_AcceptingStates.begin();
	  	for(;it!=m_AcceptingStates.end();it++)
	  		stateList.push_back(&(*it));
		return stateList;
   }
   
  /*
  *	This method returns the action states of the automaton
  */
  StateList_t GetActionStates(void)
   {
		StateList_t stateList;
		typename StateSet_t::iterator it = m_ActionStates.begin();
	  	for(;it!=m_ActionStates.end();it++)
	  		stateList.push_back(&(*it));
		return stateList;
   }
  
  /*
  *	Returns the final states of the automaton
  */
  StateList_t GetFinalStates(void)
  {
  	StateList_t stateList;
  	typename StateSet_t::iterator i = m_AllStates.begin();
  	for(;i!=m_AllStates.end();i++)
  	{
  		State s = *i;
  		if(s.isFinal())
  			stateList.push_back(&(*i));
  	}
  	return stateList;
  }

};

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::sftFsa(typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::Alphabet_t &alphabet)
  :m_StateCount(0), m_TransCount(0), m_InitialState(NULL), m_Alphabet(alphabet)
{
  assert(!isVoid<LType>::value);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::sftFsa(sftFsa<SType, LType, PType, AType, VType, SPType, SFType, BType> &other)
                                           : m_Alphabet(other.m_Alphabet)
{
  Node_t st_it, ins_st_it;
  std::map<uint32, State*> st_map;
  Edge_t ed_it, ins_ed_it;
  State *st, *from_st, *to_st;
  Transition *tr;

  m_InitialState = NULL;
  m_StateCount = other.m_StateCount;
  m_TransCount = other.m_TransCount;


  for ( st_it = other.m_Graph.begin(); st_it != other.m_Graph.end(); st_it++ ) 
  {
    st = new State( this, **st_it );
    ins_st_it = m_Graph.insert( st );
    st->SetGraphNode( ins_st_it );
    st_map[st->Id] = st;
    m_AllStates.insert( st );
    if ( st->IsInitial() )
      m_InitialState = st;
    if ( st->IsAccepting() )
      m_AcceptingStates.insert( st );
     if(st->IsAction())
     	m_ActionStates.insert(st);
  }

  for ( ed_it = other.m_Graph.arc_begin(); ed_it != other.m_Graph.arc_end(); ed_it++ ) 
  {
    tr = new Transition( this, **ed_it );
    from_st = st_map[ (*(other.m_Graph.arc_from(ed_it)))->Id ];
    to_st = st_map[ (*(other.m_Graph.arc_to(ed_it)))->Id ];
    ins_ed_it = m_Graph.arc_insert(from_st->Node, to_st->Node, tr);
    tr->SetEdge( ins_ed_it );
    st = &(*(tr->FromState()));
  }

}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
sftFsa<SType, LType, PType, AType, VType,SPType, CType, SFType, BType>::~sftFsa()
{
  //we delete all the state and transition objects
  Node_t node = m_Graph.begin();
  for (; node != m_Graph.end(); node++)
    delete (*node);
  Edge_t edge = m_Graph.arc_begin();
  for (; edge != m_Graph.arc_end(); edge++)
    delete (*edge);

}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddState(SType info)
{
  State *st = new State(*this, ++m_StateCount, info);
  //create a new graph node for the new state
  Node_t node = m_Graph.insert(st);
  //set the reference to the graph node
  st->SetGraphNode(node);
  //add the node to the AllStates set
  m_AllStates.insert(st);
  return StateIterator(m_Graph, node);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddState(State& other)
{
  State *st = new State(*this, ++m_StateCount, other.GetInfo());
  //create a new graph node for the new state
  Node_t node = m_Graph.insert(st);
  //set the reference to the graph node
  st->SetGraphNode(node);
  //add the node to the AllStates set
  m_AllStates.insert(st);
  if ( other.Accept == true ) 
  {
    st->Accept = true;
    m_AcceptingStates.insert(st);
  }
  if(other.Action == true)
  {
  	st->Action = true;
  	m_ActionStates.insert(st);
  }

  return StateIterator(m_Graph, node);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::removeState(State* state)
{
  // delete it from the graph
  m_Graph.erase(state->Node);

  // remove from the ancillary variables, if needed
  m_AllStates.remove(state);
  if(state->isAccepting())
    m_AcceptingStates.remove(state);
  if(state->isAction())
  	m_ActionStates.remove(state);

  delete state;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddTrans(StateIterator &from, StateIterator &to, Transition *trans)
{
  //create a new graph edge for the new transition
  Edge_t edge = m_Graph.arc_insert((*from).Node, (*to).Node, trans);
  //set the reference to the graph edge
  trans->SetEdge(edge);
  return TransIterator(m_Graph, edge);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddTrans(StateIterator &from, StateIterator &to,  LType lbl, bool complement, PType pred)
{
  assert((*from).Owner == this);
  assert((*to).Owner == this);

  // optimiziation: instead of forcefully adding a new transition, try and see if I can reuse another one
  typename Graph_t::arc_vector arcs_out = m_Graph.outputs((*from).Node);
  for(typename Graph_t::arc_vector::iterator i = arcs_out.begin();
      i != arcs_out.end();
      i++)
  {
    // see if this transition is ok for the optimization (same target state, same predicate, not an epsilon-transition, not related to an extended tr)
    if(m_Graph.arc_to(*i) == (*to).Node && (**i)->Predicate == pred && !(**i)->Epsilon && !(**i)->related_et)
    {
      if(complement){
        // if the transition we want to add is a complement-set one

        if((**i)->Complement) {
          // the new alphabet_t will be '*-{lbl}' if and only if {lbl} is already in (**i);
          // otherwise it will be empty: just '*'
          if((**i)->Labels.contains(lbl)) {
            (**i)->Labels.clear();
            (**i)->Labels.insert(lbl);
          } else {
            (**i)->Labels.clear();
          }
        } else {
          // The old transition is a not-complement-set one, while the current one is. So:
          // 1) make the old one a complemented-set
          (**i)->Complement = true;

          // 2) create the new alphabet_t as: '*-{lbl}' if {lbl} is not already in (**i), otherwise just as: '*'
          if((**i)->Labels.contains(lbl)) {
            (**i)->Labels.clear();
          } else {
            (**i)->Labels.clear();
            (**i)->Labels.insert(lbl);
          }
        }
      } else {
        // if the transition we want to add is not complemented

        if((**i)->Complement)
          (**i)->Labels.remove(lbl); // remove the current label from the existing complement-set transition
        else
          (**i)->Labels.insert(lbl); // add the current label to the existing transition
      }

      // optimization successful! Return the old transition
      return TransIterator(m_Graph, *i);
    }
  }

  Transition *tr = new Transition(*this, ++m_TransCount, lbl, complement, pred);
  return AddTrans(from, to, tr);
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddTrans(StateIterator &from, StateIterator &to,  Alphabet_t &labels, bool complement, PType pred)
{
	assert((*from).Owner == this);
	assert((*to).Owner == this);
  
  	if(pred == NULL)
  	{
		// optimiziation: instead of forcefully adding a new transition, try and see if I can reuse another one
		typename Graph_t::arc_vector arcs_out = m_Graph.outputs((*from).Node);
		for(typename Graph_t::arc_vector::iterator i = arcs_out.begin();i != arcs_out.end();i++)
		{
			// see if this transition is ok for the optimization (same target state, same predicate, not an epsilon-transition, not related to an extended tr)
			if(m_Graph.arc_to(*i) == (*to).Node && (**i)->Predicate == pred && !(**i)->Epsilon && !(**i)->related_et)
			{
			  if(complement)
			  {
				// if the transition we want to add is a complement-set one
				if((**i)->Complement) 
				{
				  // the new alphabet_t will be the intersection of the set in '{labels}' and those in (**i)
				  (**i)->Labels.intersect_with(labels);
				} 
				else 
				{
				  // The old transition is a not-complement-set one, while the current one is. So:
				  // 1) make the old one a complemented-set
				  (**i)->Complement = true;

				  // 2) create the new alphabet_t as the set of symbols present in 'labels' but not in (**i)
				  Alphabet_t tmp = labels - (**i)->Labels;
				  (**i)->Labels = tmp;
				}
			  } 
			  else 
			  {
				// if the transition we want to add is not complemented
				if((**i)->Complement)
				  (**i)->Labels -= labels; // remove the current labels from the existing complement-set transition
				else
				  (**i)->Labels += labels; // add the current labels to the existing transition
			  }

			  // optimization successful! Return the old transition
			  return TransIterator(m_Graph, *i);
			}
		}
	}
  
  	Transition *tr = new Transition(*this, ++m_TransCount, labels, complement, pred);
  	return AddTrans(from, to, tr);
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddTrans(StateIterator from,
    StateIterator to, Transition &other)
{
  assert((*from).Owner == this);
  assert((*to).Owner == this);
  Transition *tr = new Transition(*this, other.Id, other.Labels, other.Complement, other.Epsilon, other.Predicate, other.related_et);
  return AddTrans(from, to, tr);
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::AddEpsilon(StateIterator &from, StateIterator &to, PType pred)
{
  assert((*from).Owner == this);
  assert((*to).Owner == this);
  Transition *tr = new Transition(*this, ++m_TransCount, pred);
  return AddTrans(from, to, tr);
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::ExtendedTransition* sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::addExtended 
(StateIterator &from, LType label, StateIterator &to, StateIterator &to_dfl, PType pred)
{
  //in my example case:
  //	-EncapStIterator_t form
  //	-EncapLabel_t	   label
  //	-EncapStIterator_t to
  //	-EncapStIterator_t to_dfl
  //	-PFLTermExpression pred
  
  assert((*from).Owner == this);
  assert((*to).Owner == this);
  assert((*to_dfl).Owner == this);
  Transition *tr_1 = new Transition(*this, ++m_TransCount, label, false, p_traits::One());
  AddTrans(from, to, tr_1);
  Transition *tr_2 = new Transition(*this, ++m_TransCount, label, false, p_traits::One());
  AddTrans(from, to_dfl, tr_2);
  ExtendedTransition *et= new ExtendedTransition(*this, ++m_TransCount, label,
                                                 &(*from), pred, &(*to), tr_1->Edge, 
                                                 &(*to_dfl), tr_2->Edge);
  tr_1->related_et = et;
  tr_2->related_et = et;
  return et;
}
 
template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::ExtendedTransition* sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::addExtended(StateIterator &from, LType label, StateIterator &to, StateIterator &to_dfl, VType value, RangeOperator_t op, AType attr)
{

  assert((*from).Owner == this);
  assert((*to).Owner == this);
  assert((*to_dfl).Owner == this);
  Transition *tr_1 = new Transition(*this, ++m_TransCount, label, false, p_traits::One());
  AddTrans(from, to, tr_1);
  Transition *tr_2 = new Transition(*this, ++m_TransCount, label, false, p_traits::One());
  AddTrans(from, to_dfl, tr_2);
  ExtendedTransition *et = new ExtendedTransition(*this, ++m_TransCount, label,
                                                 &(*from), value, op, attr, &(*to), tr_1->Edge, 
                                                 &(*to_dfl), tr_2->Edge);
  tr_1->related_et = et;
  tr_2->related_et = et;
  return et;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::ExtendedTransition* sftFsa<SType, LType, PType, AType, VType, SPType,  CType, SFType, BType>::addExtended(StateIterator &from, LType label, StateIterator &to, StateIterator &to_dfl, ExtendedTransition *other)
{
  //in my example case:
  //	-EncapStIterator_t form
  //	-EncapLabel_t	   label
  //	-EncapStIterator_t to
  //	-EncapStIterator_t to_dfl
  

  assert((*from).Owner == this);
  assert((*to).Owner == this);
  assert((*to_dfl).Owner == this);
  Transition *tr_1 = new Transition(*this, ++m_TransCount, label, false, p_traits::One());
  AddTrans(from, to, tr_1);
  Transition *tr_2 = new Transition(*this, ++m_TransCount, label, false, p_traits::One());
  AddTrans(from, to_dfl, tr_2);
  ExtendedTransition *et= new ExtendedTransition(*this, ++m_TransCount, label,
                                                &(*from), &(*to), tr_1->Edge, 
                                                 &(*to_dfl), tr_2->Edge,other);
  tr_1->related_et = et;
  tr_2->related_et = et;
  return et;
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetInitialState(StateIterator &st)
{
  assert((*st).Owner == this);

  if ( m_InitialState != NULL )
    m_InitialState->ResetInitial();

  m_InitialState = &(*st);
  m_InitialState->SetInitial();
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetExtraInfoToAction(list<SFType> ei)
{
	StateList_t actions = GetActionStates();
	nbASSERT(actions.size() == 1 || actions.size() == 0, "This situation could be easily managed. But, in my mind, this cannot happen. Then I think that there is a BUG somewhere");
	if(actions.size() != 0)
		(*(actions.begin()))->setExtraInfo(ei);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetExtraInfoPlusToAction(list<VType> eip)
{
	StateList_t actions = GetActionStates();
	nbASSERT(actions.size() == 1 || actions.size() == 0, "This situation could be easily managed. But, in my mind, this cannot happen. Then I think that there is a BUG somewhere");
	if(actions.size() != 0)
	(*(actions.begin()))->setExtraInfoPlus(eip);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetOtherInfoToAction(list<BType> oei)
{
	StateList_t actions = GetActionStates();
	nbASSERT(actions.size() == 1 || actions.size() == 0, "This situation could be easily managed. But, in my mind, this cannot happen. Then I think that there is a BUG somewhere");
	if(actions.size() != 0)
	(*(actions.begin()))->setOtherInfo(oei);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetExtraInfoToState(State *state, list<SFType> ei)
{
	nbASSERT(state->Owner == this,"There is a bug! The state belongs to another fsa!");
	if(ei.size() == 0)
		return;
	state->setExtraInfo(ei);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetExtraInfoPlusToState(State *state, list<VType> eip)
{
	nbASSERT(state->Owner == this,"There is a bug! The state belongs to another fsa!");
	if(eip.size() == 0)
		return;
	state->setExtraInfoPlus(eip);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SetOtherInfoToState(State *state, list<BType> oi)
{
	nbASSERT(state->Owner == this,"There is a bug! The state belongs to another fsa!");
	if(oi.size() == 0)
		return;
	state->setOtherInfo(oi);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::setAccepting(StateIterator &st)
{
  assert((*st).Owner == this);
  State *s = &(*st);
  s->setAccepting(true);
  m_AcceptingStates.insert(s);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::resetAccepting(StateIterator &st)
{
  assert((*st).Owner == this);
  State *s = &(*st);
  s->setAccepting(false);
  m_AcceptingStates.remove(s);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::setAction(StateIterator &st)
{
  assert((*st).Owner == this);
  State *s = &(*st);
  s->setAction(true);
  m_ActionStates.insert(s);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::resetAction(StateIterator &st)
{
  assert((*st).Owner == this);
  State *s = &(*st);
  s->setAction(false);
  m_ActionStates.remove(s);
}



template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType,SPType, CType, SFType, BType>::setFinal(StateIterator &st)
{
  assert((*st).Owner == this);
  State *s = &(*st);
  s->setFinal(true);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::resetFinal(StateIterator &st)
{
  assert((*st).Owner == this);
  State *s = &(*st);
  s->setFinal(false);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::GetInitialState(void)
{
  return StateIterator(m_Graph, m_InitialState->Node);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::FirstState(void)
{
  return StateIterator(m_Graph);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateIterator sftFsa<SType, LType, PType, AType, VType,SPType, CType, SFType, BType>::LastState(void)
{
  return StateIterator(m_Graph, true);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::FirstTrans(void)
{
  return TransIterator(m_Graph);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::TransIterator sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::LastTrans(void)
{
  return TransIterator(m_Graph, true);
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::ResetVisited(void)
{
  Node_t st_it;

  for ( st_it = m_Graph.begin(); st_it != m_Graph.end(); st_it++ )
    (*st_it)->Visited = false;
}

//this method returns all the states of the automaton
template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateList_t sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::GetAllStates(void)
{
	StateList_t stateList;
	typename StateSet_t::iterator it = m_AllStates.begin();
  	for(;it!=m_AllStates.end();it++)
  		stateList.push_back(&(*it));
	return stateList;
}



template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateList_t sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SortRevPostorder(void)
{
	StateList_t statelist;
	this->ResetVisited();

	typename StateSet_t::iterator i = m_AcceptingStates.begin();
	for (; i != m_AcceptingStates.end(); i++)
		this->DFSReversePostorder((*i), statelist);
	
	return statelist;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateList_t sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SortRevPostorderAction(void)
{
	StateList_t statelist;
	this->ResetVisited();

	typename StateSet_t::iterator i = m_ActionStates.begin();
	for (; i != m_ActionStates.end(); i++)
		this->DFSReversePostorder((*i), statelist);
	
	return statelist;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType> 
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateList_t sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SortPostorder(void)
{
	StateList_t statelist;
	this->ResetVisited();

	StateIterator initial = GetInitialState();
	this->DFSPostorder(*initial, statelist);
	return statelist;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType,  CType, SFType, BType>::StateList_t sftFsa<SType, LType, PType, AType, VType, SPType,  CType, SFType, BType>::SortPreorder(void)
{
	StateList_t statelist;
	this->ResetVisited();

	StateIterator initial = GetInitialState();
	this->DFSPreorder(*initial, statelist);
	return statelist;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::StateList_t sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::SortRevPreorder(void)
{
	StateList_t statelist;
	this->ResetVisited();

	typename StateSet_t::iterator i = m_AcceptingStates.begin();
	for (; i != m_AcceptingStates.end(); i++)
		this->DFSReversePreorder(*i, statelist);
	return statelist;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::DFSPreorder(State &state, StateList_t &sortedStates)
{
	if (state.HasBeenVisited())
		return;

	sortedStates.push_back(&state);

	state.SetVisited();

	typename Graph_t::arc_vector out = m_Graph.outputs(state.Node);
	typename Graph_t::arc_vector::iterator s = out.begin();

	for(; s != out.end(); s++)
	{
		State &successor(**(m_Graph.arc_to(*s)));
		if (!successor.HasBeenVisited())
		{
			DFSPreorder(successor, sortedStates);
		}
	}
}



template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::DFSPostorder(State &state, StateList_t &sortedStates)
{
	if (state.HasBeenVisited())
		return;

	state.SetVisited();

	typename Graph_t::arc_vector out = m_Graph.outputs(state.Node);
	typename Graph_t::arc_vector::iterator s = out.begin();

	for(; s != out.end(); s++)
	{
		State &successor(**(m_Graph.arc_to(*s)));
		if (!successor.HasBeenVisited())
		{
			DFSPostorder(successor, sortedStates);
		}
	}

	sortedStates.push_back(&state);
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::DFSReversePreorder(State &state, StateList_t &sortedStates)
{

	if (state.HasBeenVisited())
		return;

	sortedStates.push_back(&state);

	state.SetVisited();

	typename Graph_t::arc_vector in = m_Graph.inputs(state.Node);
	typename Graph_t::arc_vector::iterator p = in.begin();

	for(; p != in.end(); p++)
	{
		State &predecessor(**(m_Graph.arc_from(*p)));
		if (!predecessor.HasBeenVisited())
		{
			DFSReversePreorder(predecessor, sortedStates);
		}
	}
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::DFSReversePostorder(State &state, StateList_t &sortedStates)
{
	if (state.HasBeenVisited())
		return;

	state.SetVisited();
	typename Graph_t::arc_vector in = m_Graph.inputs(state.Node);
	typename Graph_t::arc_vector::iterator p = in.begin();

	for(; p != in.end(); p++)
	{
		
		State &predecessor(**(m_Graph.arc_from(*p)));
		
		if (!predecessor.HasBeenVisited())
		{
			DFSReversePostorder(predecessor, sortedStates);
		}
	}
	sortedStates.push_back(&state);
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
std::list<typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::Transition*> sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::getTransExitingFrom(State *state)
{
  std::list<Transition*> result;
  typename Graph_t::arc_vector arcs_out = m_Graph.outputs(state->Node);
  for(typename Graph_t::arc_vector::iterator i = arcs_out.begin();
      i != arcs_out.end();
      i++)
    result.push_back(**i);

  return result;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
std::list<typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::Transition*> sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::getTransEnteringTo(State *state)
{
  std::list<Transition*> result;
  typename Graph_t::arc_vector arcs_in = m_Graph.inputs(state->Node);
  for(typename Graph_t::arc_vector::iterator i = arcs_in.begin();
      i != arcs_in.end();
      i++)
    result.push_back(**i);

  return result;
}


template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
std::list<typename sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::Transition*> sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::getTransExitingFrom(State *state, LType label)
{
  std::list<Transition*> result;
  typename Graph_t::arc_vector arcs_out = m_Graph.outputs(state->Node);
  for(typename Graph_t::arc_vector::iterator i = arcs_out.begin();
      i != arcs_out.end();
      i++) {
    Transition *t = **i;
    if (t->Labels.contains(label) && !t->Complement)
      result.push_back(t);
  }

  return result;
}

template <class SType, class LType, class PType, class AType, class VType, class SPType, class CType, class SFType, class BType>
void sftFsa<SType, LType, PType, AType, VType, SPType, CType, SFType, BType>::compactStates(map<State*, State*> mappings,
                                                                      bool clearStateInfoIfConflicting)
{
  set<ExtendedTransition*> et_encountered;

  /* iterate on all the transitions */
  for(Edge_t t = m_Graph.arc_begin(); t != m_Graph.arc_end(); ++t)
  {
    State *from = *m_Graph.arc_from(t);
    State *to = *m_Graph.arc_to(t);

    State *from_new = (mappings.find(from) != mappings.end()?
                       mappings.find(from)->second : from);
    State *to_new = (mappings.find(to) != mappings.end()?
                     mappings.find(to)->second : to);

    // if the current transition should not be redrawn, skip it
    if(from_new == from && to_new == to)
      continue;

    // See if there is an equivalent transition already.
    // We might not need to redraw this.
    bool must_be_redrawn = true;
    vector<Edge_t> candidates = m_Graph.adjacent_arcs(from_new->Node, to_new->Node);
    for(typename vector<Edge_t>::iterator c_iter = candidates.begin();
        c_iter != candidates.end() && must_be_redrawn;
        ++c_iter)
    {
      Transition *orig = *t;
      Transition *c = **c_iter;
      // test for a 'quick' equivalence, I'm not going exaustively
      if (!orig->getIncludingET() &&
          orig->Epsilon == c->Epsilon &&
          orig->Complement == c->Complement &&
          orig->Labels == c->Labels)
        must_be_redrawn = false;
    }

    if(!must_be_redrawn)
      continue;

    // actually redraw the transition
    m_Graph.arc_move(t, from_new->Node, to_new->Node);
    ExtendedTransition *et = (*t)->getIncludingET();
    if(et != NULL) {
      if (et_encountered.find(et) != et_encountered.end())
        continue;

      et_encountered.insert(et);
      et->changeGateRefs(NULL,
                         (from_new != from? from_new : NULL),
                         mappings );
    }
  }

  /* delete all the compacted states; this will delete all the associated transitions too */
  for(typename map<State*, State*>::iterator i = mappings.begin();
      i != mappings.end();
      ++i) {
    if (i->first->Info != i->second->Info) {
      nbASSERT(clearStateInfoIfConflicting, "Compacted states have different stateInfo. This check is here to find bugs, but can be overridden by the caller in the source code.");
      i->second->Info = NULL;
    }
    removeState(i->first);
  }
}

#endif
