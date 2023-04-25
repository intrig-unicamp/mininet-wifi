/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef RANGE_HPP_INCLUDED
#define RANGE_HPP_INCLUDED

#include <map>
#include <set>
#include <string>
#include "./sft/librange/internals.hpp"
#include "./sft/librange/common.h"

/* == important declarations == */

/* KType specifies the type of the data associated with the range keys,
 * AType specifies the type of the action to which each range is associated
 */
template <class KType, class AType, class SPType>
class Range
{
  typedef AType(*merger_func_t)(const AType, const AType, void*);
  typedef void(*range_callback_func_t)(RangeOperator_t, KType, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, const std::map<KType,AType>&, void*);
  typedef void(*action_callback_func_t)(AType, void*);
  typedef void(*special_callback_func_t)(RangeOperator_t, SPType, void*);

public:
  Range(AType dfl_action);
  Range(const Range &other);
  Range(const Range *other);
  void addRange(RangeOperator_t op, KType key, AType action, SPType sp);
  AType find(KType key) const;
  std::set<AType> findAll() const;
  static Range intersect(Range a, Range b, merger_func_t merger, void *extra_info);
  static Range* intersect(Range *a, Range *b, merger_func_t merger, void *extra_info);
  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, special_callback_func_t special_callback, void *extra_info) const;
  void changeActions(const std::map<AType,AType> &mappings);
 
  /* ** helper methods ** */
  static std::string rangeOp2str(RangeOperator_t op) {
    switch (op) {
    case LESS_THAN: return std::string("less than");
    case LESS_EQUAL_THAN: return std::string("less than or equal to");
    case GREAT_THAN: return std::string("greater than");
    case GREAT_EQUAL_THAN: return std::string("greater than or equal to");
    case EQUAL: return std::string("equal to");
    case MATCH: return std::string("matches");
    case CONTAINS: return std::string("contains");
    case INVALID:
    default:
      ;
    }
    return std::string("");
  }
  static std::string rangeOp2shortStr(RangeOperator_t op) {
    switch (op) {
    case LESS_THAN: return std::string("<");
    case LESS_EQUAL_THAN: return std::string("<=");
    case GREAT_THAN: return std::string(">");
    case GREAT_EQUAL_THAN: return std::string(">=");
    case EQUAL: return std::string("=");
    case MATCH: return std::string("match");
    case CONTAINS: return std::string("contains");
    case INVALID:
    default:
      ;
    }
    return std::string("");
  }

private:
  AType default_action;
  OpNode<KType,AType, SPType> *tree;
};


/* == template implementation follows == */
template <class KType, class AType, class SPType>
Range<KType,AType,SPType>::Range(AType dfl_action)
  : default_action(dfl_action), tree(NULL)
{
}

template <class KType, class AType, class SPType>
Range<KType,AType,SPType>::Range(const Range<KType,AType,SPType> &other)
  : default_action(other.default_action)
{
  if (other.tree)
    this->tree = other.tree->clone();
  else
    this->tree = NULL;
}

template <class KType, class AType, class SPType>
Range<KType,AType,SPType>::Range(const Range<KType,AType,SPType> *other)
  : default_action(other->default_action)
{
  if (other->tree)
    this->tree = other->tree->clone();
  else 
    this->tree = NULL;
}

template <class KType, class AType, class SPType>
void Range<KType,AType,SPType>::addRange
(RangeOperator_t op, KType /*uint32*/ key, AType /*SubState_t*/ action, SPType sp /*string*/)
{
  if (tree == NULL)
    tree = OpNode<KType,AType, SPType>::buildOpNode(default_action, op, key, action, sp);
  else
    tree->addRange(op, key, action,sp);
}

/* returns the action associated with the provided key */
template <class KType, class AType, class SPType>
AType Range<KType,AType,SPType>::find(KType key) const
{
  if (tree == NULL)
    return default_action;

  return tree->find(key);
}

/* returns all the actions */
template <class KType, class AType, class SPType>
std::set<AType> Range<KType,AType,SPType>::findAll() const{
  std::set<AType> to_ret;

  to_ret.insert(default_action);
  if(tree)
    tree->grabAllActions(&to_ret);

  return to_ret;
}

template <class KType, class AType, class SPType>
Range<KType,AType,SPType> Range<KType,AType,SPType>::intersect(Range a, Range b, merger_func_t merger, void* extra_info)
{
  AType new_dfl = (*merger)(a.default_action, b.default_action, extra_info); //fa il merge delle default action (ovvero dei subsstates di destinazione) restituendo un nuovo stato di destinazione
  Range result(new_dfl);  //crea una nuova transizione con il nuovo stato di destinazione impostato come default

  result.tree = NULL;
  if(a.tree != NULL && b.tree != NULL) {
    TreeNode<KType,AType, SPType> *tmp = TreeMerger<KType,AType, SPType>::merge(a.tree, b.tree, merger, extra_info, NULL, false, NULL, false); //fa il merge delle azioni non di default restituendo un insieme di stati di destinazione
    // the following cast is legal, because by construction only an OpNode can be the root of the tree
    result.tree = dynamic_cast<OpNode<KType,AType, SPType>*>(tmp); 
    if(!result.tree) abort(); // something broke
  } else {
    // otherwise take the (possibly) not-NULL tree and merge it with the other default action
    if(a.tree) {
      ActionNode<KType,AType, SPType> *tmp_action=new ActionNode<KType,AType, SPType>(b.default_action);
      TreeNode<KType,AType, SPType> *tmp = TreeMerger<KType,AType, SPType>::merge(a.tree, tmp_action, merger, extra_info, NULL, false, NULL, false);
      result.tree = dynamic_cast<OpNode<KType,AType, SPType>*>(tmp);
      if(!result.tree) abort(); // something broke
      delete tmp_action;
    } else if (b.tree) {
      ActionNode<KType,AType, SPType> *tmp_action=new ActionNode<KType,AType, SPType>(a.default_action);
      TreeNode<KType,AType, SPType> *tmp = TreeMerger<KType,AType, SPType>::merge(b.tree, tmp_action, merger, extra_info, NULL, false, NULL, false);
      result.tree = dynamic_cast<OpNode<KType,AType, SPType>*>(tmp);
      if(!result.tree) abort(); // something broke
      delete tmp_action;
    }
  }

  return result;
}

template <class KType, class AType, class SPType>
Range<KType,AType,SPType>* Range<KType,AType,SPType>::intersect(Range *a, Range *b, merger_func_t merger, void* extra_info)
{
  AType new_dfl = (*merger)(a->default_action, b->default_action, extra_info);
  Range *result = new Range(new_dfl);

  result->tree = NULL;
  if(a->tree != NULL && b->tree != NULL) {
    TreeNode<KType,AType, SPType> *tmp = TreeMerger<KType,AType, SPType>::merge(a->tree, b->tree, merger, extra_info, NULL, false, NULL, false);
    // the following cast is legal, because by construction only an OpNode can be the root of the tree
    result->tree = dynamic_cast<OpNode<KType,AType, SPType>*>(tmp);
    if(!result->tree) abort(); // something broke
  } else {
    // otherwise take the (possibly) not-NULL tree and merge it with the other default action
    if(a->tree) {
      ActionNode<KType,AType, SPType> *tmp_action=new ActionNode<KType,AType, SPType>(b->default_action);
      TreeNode<KType,AType, SPType> *tmp = TreeMerger<KType,AType, SPType>::merge(a->tree, tmp_action, merger, extra_info, NULL, false, NULL, false);
      result->tree = dynamic_cast<OpNode<KType,AType, SPType>*>(tmp);
      if(!result->tree) abort(); // something broke
      delete tmp_action;
    } else if (b->tree) {
      ActionNode<KType,AType, SPType> *tmp_action=new ActionNode<KType,AType, SPType>(a->default_action);
      TreeNode<KType,AType, SPType> *tmp = TreeMerger<KType,AType, SPType>::merge(b->tree, tmp_action, merger, extra_info, NULL, false, NULL, false);
      result->tree = dynamic_cast<OpNode<KType,AType, SPType>*>(tmp);
      if(!result->tree) abort(); // something broke
      delete tmp_action;
    }
  }

  return result;
}

template <class KType, class AType, class SPType>
void Range<KType,AType,SPType>::traverse
(range_callback_func_t range_callback,
 punt_callback_func_t punt_callback,
 action_callback_func_t action_callback,
 special_callback_func_t special_callback,
 void* extra_info) const
{
  if (tree)
    tree->traverse(range_callback, punt_callback, action_callback, special_callback, extra_info);
  else if (action_callback)
    (*action_callback)(default_action, extra_info);
}


template <class KType, class AType, class SPType>
void Range<KType,AType,SPType>::changeActions(const std::map<AType,AType> &mappings)
{
  typename std::map<AType, AType>::const_iterator i = mappings.find(default_action);

  if (i != mappings.end())
    default_action = i->second;
  
  if (tree)
  {  	
    TreeNode<KType,AType, SPType> *new_root = tree->changeActions(mappings);
    if(new_root->getType() == ACTION) {
      // The tree was compacted in a single ActionNode, therefore just
      // extract the default action and check that it is equal to
      // 'default_action' (it should be, because of the default semantics).
      ActionNode<KType,AType, SPType> *new_root_as_actnode = dynamic_cast<ActionNode<KType, AType, SPType>*>(new_root);
      if (!new_root_as_actnode) abort(); // something is wrong
      if (default_action != new_root_as_actnode->getAction()) abort(); // this is wrong as well
      tree = NULL;
    }
	else
	{
      tree = dynamic_cast<OpNode<KType, AType, SPType>*>(new_root);
      if (!tree) abort(); //something broke, the cast above should be legal
    }
  }
}

#endif /* RANGE_HPP_INCLUDED */
