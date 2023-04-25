/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef _SFT_BASIC_ITERATOR
#define _SFT_BASIC_ITERATOR

#include <assert.h>

/* AType: Container type
   IType: iterator on container
   CType: "real" type behind the iterator
 */

template <typename AType, typename IType, typename CType>
struct stl_generic
{

	typedef CType deref_type;
	static IType begin(AType &a)
	{
		return a.begin();
	}

	static IType end(AType &a)
	{
		return a.end();
	}

	static IType next(AType &a, IType &it)
	{
		it++;
		return it;
	}

	static bool isend(AType &a, IType it)
	{
		return it == a.end();
	}

	static deref_type deref(IType &it)
	{
		return (*it);
	}

};

//partial specialization for pointer contained types
template <typename AType, typename IType, typename CType>
struct stl_generic<AType, IType, CType*>: public stl_generic<AType, IType, CType>
{
	typedef CType& deref_type;
	static deref_type deref(IType &it)
	{
		//(*it) returns a CType*
		//*(*it) returns a CType&
		return *(*it);
	}

};


template <class AType, class IType, class CType, class traits = stl_generic<AType, IType, CType> >
class sft_basic_iterator
{
	AType 	&_aggregate; 	//!< Aggregate on which we iterate
	IType 	_i; 			//!< Iterating Element

public:

	/*!
	 * \brief contructor of a sft_basic_iterator
	 */
	sft_basic_iterator(AType &a)
		:_aggregate(a), _i(traits::begin(_aggregate)) {}

	/*!
	 * \brief contructor of a sft_basic_iterator
	 */
	sft_basic_iterator(AType &a, IType &i)
		:_aggregate(a), _i(i) {}

	/*!
	 * \brief copy contructor
	 */
	sft_basic_iterator(const sft_basic_iterator &i)
		:_aggregate(i._aggregate), _i(i._i) {}


	/*!
	 * \brief create the "end" sentinel
	 */
	sft_basic_iterator(AType &a, bool)
		:_aggregate(a), _i(traits::end(_aggregate)){}



	typename traits::deref_type operator*()
	{
		assert(!traits::isend(_aggregate, _i));
		return traits::deref(_i);
	}

	sft_basic_iterator& operator++()
	{
		assert(!traits::isend(_aggregate, _i));
		_i = traits::next(_aggregate, _i);
		return *this;
	}

	sft_basic_iterator operator++(int)
	{ // Postfix
		assert(!traits::isend(_aggregate, _i));
		sft_basic_iterator k(*this);
		_i = traits::next(_aggregate, _i);
		return k;
	}

	bool operator==(const sft_basic_iterator& other) const
	{
		assert(&_aggregate == &(other._aggregate));
		return _i == other._i;
	}

	bool operator!=(const sft_basic_iterator& other) const
	{
		assert(&_aggregate == &(other._aggregate));
		return _i != other._i;
	}

};


#endif
