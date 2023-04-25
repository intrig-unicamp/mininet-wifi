/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef _SFT_GENERIC_SET
#define _SFT_GENERIC_SET


#include "sft_defs.h"
#include "sft_basic_iterator.hpp"
#include <set>
#include <algorithm>



//FW declarations
template <class IType> class sft_generic_set;
template <class IType> sft_generic_set<IType> SetUnion( sft_generic_set<IType> &a,  sft_generic_set<IType> &b);
template <class IType> sft_generic_set<IType> SetIntersection( sft_generic_set<IType> &a,  sft_generic_set<IType> &b);
template <class IType> sft_generic_set<IType> SetDifference( sft_generic_set<IType> &a,  sft_generic_set<IType> &b);


template <class IType>
class sft_generic_set
{
	typedef typename std::set<IType> InternalSet_t;
	typedef typename InternalSet_t::iterator InternalSetIterator_t;
	typedef typename std::insert_iterator<InternalSet_t> InsertIterator_t;
	InternalSet_t	m_Set;

	friend sft_generic_set SetUnion<>( sft_generic_set &a,  sft_generic_set &b);
	friend sft_generic_set SetIntersection<>( sft_generic_set &a,  sft_generic_set &b);
	friend sft_generic_set SetDifference<>( sft_generic_set &a,  sft_generic_set &b);


public:
	
	typedef sft_basic_iterator<InternalSet_t, InternalSetIterator_t, IType> iterator;
	
	
	sft_generic_set()
	{
	}
	
	sft_generic_set(const sft_generic_set &other);
	
	sft_generic_set &operator=(sft_generic_set &other);

	sft_generic_set operator-(sft_generic_set &other);

	sft_generic_set &operator-=(sft_generic_set &other);

	sft_generic_set &operator+=(sft_generic_set &other);
	
	sft_generic_set &union_with( sft_generic_set &other);
	
	sft_generic_set &intersect_with( sft_generic_set &other);
	
	sft_generic_set &complement( sft_generic_set &universe);
	
	bool operator==(const sft_generic_set &other);
	
	bool operator!=(const sft_generic_set &other);
	
	void insert(IType item);
	
	void remove(IType item);
	
	bool contains(IType item);
	
	void clear(void);
	
	iterator begin(void);
	
	iterator end(void);
	
	uint32 size(void);
	
	bool empty(void);
	
};

template <class IType>
sft_generic_set<IType> SetUnion( sft_generic_set<IType> &a,  sft_generic_set<IType> &b)
{
	sft_generic_set<IType> newSet;
	typename sft_generic_set<IType>::InsertIterator_t ii(newSet.m_Set, newSet.m_Set.begin());
	std::set_union(a.m_Set.begin(), a.m_Set.end(), b.m_Set.begin(), b.m_Set.end(), ii);
	return newSet;
}

template <class IType>
sft_generic_set<IType> SetIntersection( sft_generic_set<IType> &a,  sft_generic_set<IType> &b)
{
	sft_generic_set<IType> newSet;
	typename sft_generic_set<IType>::InsertIterator_t ii(newSet.m_Set, newSet.m_Set.begin());
	std::set_intersection(a.m_Set.begin(), a.m_Set.end(), b.m_Set.begin(), b.m_Set.end(), ii);
	return newSet;
}

template <class IType>
sft_generic_set<IType> SetDifference( sft_generic_set<IType> &a,  sft_generic_set<IType> &b)
{
	sft_generic_set<IType> newSet;
	typename sft_generic_set<IType>::InsertIterator_t ii(newSet.m_Set, newSet.m_Set.begin());
	std::set_difference(a.m_Set.begin(), a.m_Set.end(), b.m_Set.begin(), b.m_Set.end(), ii);
	return newSet;
}

template <class IType>
sft_generic_set<IType>::sft_generic_set(const sft_generic_set<IType> &other)
	:m_Set(other.m_Set){}

template <class IType>
sft_generic_set<IType> &sft_generic_set<IType>::operator=(sft_generic_set<IType> &other)
{
	m_Set = other.m_Set;
	return *this;
}

template <class IType>
sft_generic_set<IType> sft_generic_set<IType>::operator-(sft_generic_set<IType> &other)
{
  sft_generic_set<IType> ret = SetDifference(*this, other);
  return ret;
}

template <class IType>
sft_generic_set<IType> &sft_generic_set<IType>::operator-=(sft_generic_set<IType> &other)
{
  sft_generic_set<IType> tmp = SetDifference(*this, other);
  *this = tmp;
  return *this;
}

template <class IType>
sft_generic_set<IType> &sft_generic_set<IType>::operator+=(sft_generic_set<IType> &other)
{
  union_with(other);
  return *this;
}
	
template <class IType>
sft_generic_set<IType>& sft_generic_set<IType>::union_with( sft_generic_set<IType> &other)
{
  for (typename InternalSet_t::iterator i = other.m_Set.begin();
       i != other.m_Set.end();
       ++i)
    m_Set.insert(*i);

  return *this;
}

template <class IType>
sft_generic_set<IType>& sft_generic_set<IType>::intersect_with( sft_generic_set<IType> &other)
{
	sft_generic_set<IType> newSet = SetIntersection(*this, other);
	(*this) = newSet;
	return (*this);
}

template <class IType>
sft_generic_set<IType>& sft_generic_set<IType>::complement( sft_generic_set<IType> &universe)
{
	sft_generic_set<IType> newSet = SetIntersection(universe, *this);
	(*this) = newSet;
	return (*this);
}

template <class IType>
bool sft_generic_set<IType>::operator==(const sft_generic_set<IType> &other)
{
	return m_Set == other.m_Set;
}

template <class IType>
bool sft_generic_set<IType>::operator!=(const sft_generic_set<IType> &other)
{
	return !((*this) == other);
}

template <class IType>
void sft_generic_set<IType>::insert(IType item)
{
	m_Set.insert(item);
}
	
template <class IType>
void sft_generic_set<IType>::remove(IType item)
{
	m_Set.erase(item);
}

template <class IType>
bool sft_generic_set<IType>::contains(IType item)
{
	typename InternalSet_t::iterator i = m_Set.find(item);
	return (i != m_Set.end());
}


template <class IType>
void sft_generic_set<IType>::clear()
{
	m_Set.clear();
}

template <class IType>
typename sft_generic_set<IType>::iterator sft_generic_set<IType>::begin(void)
{
	return iterator(m_Set);
}


template <class IType>
typename sft_generic_set<IType>::iterator sft_generic_set<IType>::end(void)
{
	return iterator(m_Set, true);
}


template <class IType>
uint32 sft_generic_set<IType>::size(void)
{
	return m_Set.size();
}

template <class IType>
bool sft_generic_set<IType>::empty(void)
{
	return m_Set.empty();
}


#endif
