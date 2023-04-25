#ifndef _SFT_TOOLS_H
#define _SFT_TOOLS_H


#include <list>
#include "sft_defs.h"
#include "sft.hpp"


template <class SType, class LType, class PType = bool>
class sftTools {

private:

  typedef sftFsa<SType, LType, PType> Fsa_t;
  typedef typename sftFsa<SType, LType, PType>::State State_t;
  typedef typename sftFsa<SType, LType, PType>::Transition Transition_t;
  typedef typename sftFsa<SType, LType, PType>::StateIterator StIterator_t;
  typedef typename sftFsa<SType, LType, PType>::TransIterator TrIterator_t;
  typedef stlplus::digraph<State_t*, Transition_t*> Graph_t;

  static void AddStates(StIterator_t Old_node, StIterator_t New_node,
                        std::map<uint32, State_t*> &st_map, Fsa_t &Src, Fsa_t &Dest);

public:

  static void Complement(Fsa_t &Fsa);
  static Fsa_t* ComplementAndCopy(Fsa_t &Fsa);
  static Fsa_t* Merge(Fsa_t &First, Fsa_t &Second);
  //static Fsa_t* Determinize(Fsa_t &Nfa);

};


template <class SType, class LType, class PType>
void sftTools<SType, LType, PType>::AddStates(StIterator_t Old_node, StIterator_t New_node,
                                              std::map<uint32, State_t*> &st_map, Fsa_t &Src, Fsa_t &Dest)
{
  typename Graph_t::arc_vector in_vec;
  typename Graph_t::arc_vector::iterator arc_vec_it;
  typename Graph_t::arc_iterator arc_it;
  typename Graph_t::iterator st_it;
  State_t *newst;

  
  //StIterator_t Thisnode(Dest.AddState(*Old_node));
  in_vec = Src.m_Graph.outputs((*Old_node).Node);
  for ( arc_vec_it = in_vec.begin(); arc_vec_it != in_vec.end(); arc_vec_it++ ) {
    arc_it = *arc_vec_it;
    st_it = Src.m_Graph.arc_to(arc_it);
    if ( (*st_it)->HasBeenVisited() ) {
      newst = st_map[(*st_it)->Id];
      Dest.AddTrans(New_node, typename Fsa_t::StateIterator(Dest.m_Graph, newst->Node),
                    **arc_it);
    }
    else {
      (*st_it)->SetVisited();
      StIterator_t Next_node = Dest.AddState(**st_it);
      st_map[(*st_it)->Id] = &(*Next_node);
      Dest.AddTrans(New_node, Next_node, **arc_it);
      AddStates(StIterator_t(Src.m_Graph, (*st_it)->Node), Next_node, st_map, Src, Dest);
    }
  }
}

template <class SType, class LType, class PType>
void sftTools<SType, LType, PType>::Complement(Fsa_t &Fsa)
{
  typename sftFsa<SType, LType, PType>::StateIterator s_it( Fsa.FirstState() );

  for ( ; s_it != Fsa.LastState(); s_it++ )
    (*s_it).Accept = !(*s_it).Accept;

  Fsa.m_AcceptingStates.complement( Fsa.m_AllStates );

  return;
}


template <class SType, class LType, class PType>
sftFsa<SType, LType, PType>* sftTools<SType, LType, PType>::ComplementAndCopy(Fsa_t &Fsa)
{
  Fsa_t *NewFsa = new sftFsa<SType, LType, PType>(Fsa);
  Complement(*NewFsa);

  return NewFsa;
}


template <class SType, class LType, class PType>
sftFsa<SType, LType, PType>* sftTools<SType, LType, PType>::Merge(Fsa_t &First, Fsa_t &Second)
{
  Fsa_t *NewFsa;
  std::map<uint32, State_t*> st_map;

  // Check if the two FSAs are "compatible"
  if ( First.m_Alphabet != Second.m_Alphabet )
    return NULL;

  NewFsa = new Fsa_t(First.m_Alphabet);  
  StIterator_t start = NewFsa->AddState();
  NewFsa->SetInitialState(start);
  StIterator_t OldFirstState = First.GetInitialState();
  StIterator_t NewFirstState = NewFsa->AddState( *OldFirstState );
  st_map[(*OldFirstState).Id] = &(*NewFirstState);
  NewFsa->AddEpsilon(start, NewFirstState);
  (*OldFirstState).SetVisited();
  First.ResetVisited();
  AddStates(OldFirstState, NewFirstState, st_map, First, *NewFsa);

  st_map.clear();
  StIterator_t OldSecondState = Second.GetInitialState();
  StIterator_t NewSecondState = NewFsa->AddState( *OldSecondState );
  st_map[(*OldSecondState).Id] = &(*NewSecondState);
  NewFsa->AddEpsilon(start, NewSecondState);
  (*OldSecondState).SetVisited();
  Second.ResetVisited();
  AddStates(OldSecondState, NewSecondState, st_map, Second, *NewFsa);

  return NewFsa;
}


// template <class SType, class LType, class PType>
//     sftFsa<SType, LType, PType>* sftTools<SType, LType, PType>::Determinize(Fsa_t &Nfa)
// {
//   
// }

#endif
