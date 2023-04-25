/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file bitvectorset.h
 * \brief this file contains definition of the class that implements a bitvectorset
 */



#pragma once

#include <iostream>	
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INT2STR(num, base) std::string(num);

/*!
 * \brief this class implements a set by using a bit vector
 *
 * For low memory usage we represent a set with a vector of bits
 */
class nbBitVector
{
	typedef uint32_t Word_t;
	#define _BYTE_SIZE	8
	#define _WORD_SIZE	32

	uint32_t _NBits;
	uint32_t _NWords;
	Word_t *_Bits;

	class Reference;
	friend class Reference;

	class Reference
	{
		friend class nbBitVector;
		Word_t	*_Word;
		uint32_t	_Bit;
		
		//left undefined
		Reference(void);

	public:
		Reference(nbBitVector &bv, uint32_t pos)
		{
			_Word = &bv._GetWord(pos);
			_Bit = nbBitVector::_WhichBit(pos);
		}
		
		~Reference() 
		{}


		  // for b[i] = x;
		Reference& operator=(bool x) {
		if (x)
			*_Word |= nbBitVector::_MaskBit(_Bit);
		else
			*_Word &= ~nbBitVector::_MaskBit(_Bit);

		return *this;
		}

		Reference& operator=(uint32_t x) {
		if (x)
			*_Word |= nbBitVector::_MaskBit(_Bit);
		else
			*_Word &= ~nbBitVector::_MaskBit(_Bit);

		return *this;
		}

		 // for b[i] = b[j];
		Reference& operator=(const Reference& j) 
		{
			if (*(j._Word) & nbBitVector::_MaskBit(j._Bit))
				*_Word |= nbBitVector::_MaskBit(_Bit);
			else
				*_Word &= ~nbBitVector::_MaskBit(_Bit);
			return *this;
		}

		// flips the bit
		bool operator~() const
		{ 
			return (((*_Word) & nbBitVector::_MaskBit(_Bit)) == 0); 
		}

		// for x = b[i];
		operator bool() const
		{ 
			return (((*_Word) & nbBitVector::_MaskBit(_Bit)) != 0); 
		}

		// for b[i].flip();
		Reference& flip() 
		{
			*_Word ^= nbBitVector::_MaskBit(_Bit);
			return *this;
		}
		
	};

	static uint32_t _WhichWord(uint32_t pos)
	{
		return pos / _WORD_SIZE; 
	}

	static uint32_t _WhichBit(uint32_t pos)
    {
		return pos % _WORD_SIZE;
	}

	static Word_t _MaskBit(uint32_t pos )
    { 
		return (1 << _WhichBit(pos)); 
	}

	Word_t& _GetWord(uint32_t pos)       
	{ 
		return _Bits[_WhichWord(pos)]; 
	}

	Word_t _GetWord(uint32_t pos) const 
	{
		return _Bits[_WhichWord(pos)];
	}

	void _DropExtraBits(void)
	{
		_Bits[_NWords - 1] &= ~((~(Word_t)0) << (_NBits % _WORD_SIZE)); 
	}


public:
	nbBitVector(uint32_t numBits);
	nbBitVector(const nbBitVector& other);
	~nbBitVector();
	
	class iterator;
	friend class iterator;

	class iterator
	{
		public:

		typedef uint32_t value_type;  //!< type of the value of this iterator
		typedef uint32_t difference_type; 					 //!< type of the difference between to elements
		typedef uint32_t* pointer; 	 //!< type of a pointer to the value
		typedef uint32_t& reference;  //!< type of a refernce to a value
		typedef std::forward_iterator_tag iterator_category; //!< category of this iterator

		iterator(const nbBitVector& bitvector, bool end = false);
		iterator(const iterator& it);

		value_type operator*();
		value_type operator->();
		iterator& operator++();
		iterator& operator++(int);
		bool operator==(const iterator& other) const;
		bool operator!=(const iterator& other) const; 

		private:
		const nbBitVector& bitvector;
		uint32_t pos;
		Word_t actual_word;
		Word_t actual_mask;
	};

	typedef iterator const_iterator;

	iterator begin() const
	{
		return iterator(*this);
	}

	iterator end() const
	{
		return iterator(*this, true);
	}

	bool Test(uint32_t pos) const 
	{
		//nbASSERT(pos < _NBits, "Index out of range!");
		return (this->_GetWord(pos) & nbBitVector::_MaskBit(pos)) != 0;
	}

	bool empty() const;

	nbBitVector& Set(uint32_t pos) 
	{
		//nbASSERT(pos < _NBits, "Index out of range!");
		_GetWord(pos) |= _MaskBit(pos);
		return *this;
	}

	nbBitVector& Reset(uint32_t pos) 
	{
		//nbASSERT(pos < _NBits, "Index out of range!");
		_GetWord(pos) &= ~_MaskBit(pos);
		return *this;
	}

	nbBitVector& Flip(uint32_t pos) 
	{
		//nbASSERT(pos < _NBits, "Index out of range!");
		_GetWord(pos) ^= _MaskBit(pos);
		return *this;
	}


	nbBitVector& Set(void)
	{
		memset(_Bits, 0, _NWords * sizeof(Word_t));
		return Flip();
	}

	nbBitVector& Reset(void)
	{
		memset(_Bits, 0, _NWords * sizeof(Word_t));
		return *this;
	}

	nbBitVector& Flip(void) 
	{
		for (uint32_t i = 0; i < _NWords; i++)
			_Bits[i] = ~_Bits[i];
		_DropExtraBits();
		return *this;
	}

	bool operator==(const nbBitVector& rval) const
	{
		for (uint32_t i = 0; i < _NWords; i++)
			if (_Bits[i] != rval._Bits[i]) 
				return false;

		return true;
	}

	bool operator!=(const nbBitVector& rval) const
	{
		return !(*this == rval);
	}

	nbBitVector& operator=(const nbBitVector& rval)
	{
		//printf("Operator = \n");
		//nbASSERT(_NBits == rval._NBits, "You are assigning a bitvector to one of different size");
		for (uint32_t i = 0; i < _NWords; i++)
			_Bits[i] = rval._Bits[i];
		return *this;
	}

	nbBitVector operator~()
	{ 
		return nbBitVector(*this).Flip();
	}


	nbBitVector& operator&=(const nbBitVector& rval)
	{
		for (uint32_t i = 0; i < _NWords; i++)
			_Bits[i] &= rval._Bits[i];
		return *this;
	}

	nbBitVector& operator|=(const nbBitVector& rval) 
	{
		for (uint32_t i = 0; i < _NWords; i++)
			_Bits[i] |= rval._Bits[i];
		return *this;
	}

	nbBitVector& operator+=(const nbBitVector& rval)
	{
		for (uint32_t i = 0; i < _NWords; i++)
			_Bits[i] |= rval._Bits[i];
		return *this;
	}

	nbBitVector& operator^=(const nbBitVector& rval) 
	{
		for (uint32_t i = 0; i < _NWords; i++)
			_Bits[i] ^= rval._Bits[i];
		return *this;
	}

	//for b[i];
	Reference operator[](uint32_t pos) 
	{ 
		//nbASSERT(pos < _NBits, "Index out of range!");
		return Reference(*this, pos); 
	}

	bool operator[](uint32_t pos) const 
	{ 
		return Test(pos); 
	}

	std::string ToString(void)
	{
		std::string tmp("");
		for (uint32_t i = 0; i < _NBits; i++)
		{
			if (Test(i))
			{
				std::ostringstream _str;
				_str << i;
				tmp.append(_str.str());
				tmp.append(" ");
			}
		}
		return tmp;
	}

	friend const nbBitVector operator& (const nbBitVector& a, const nbBitVector& b);

	friend const nbBitVector operator| (const nbBitVector& a, const nbBitVector& b);

	friend const nbBitVector operator^ (const nbBitVector& a, const nbBitVector& b);

	friend const nbBitVector operator- (const nbBitVector& a, const nbBitVector& b);

	friend const nbBitVector operator+ (const nbBitVector& a, const nbBitVector& b);

	friend std::ostream& operator<<(std::ostream& os, const nbBitVector& );
};

inline uint32_t nbBitVector::iterator::operator*()
{
	return pos;
}

inline uint32_t nbBitVector::iterator::operator->()
{
	return pos;
}

inline nbBitVector::iterator& nbBitVector::iterator::operator++()
{
	pos++;
	actual_mask = actual_mask << 1;
	if(!actual_mask)
	{
		actual_word = bitvector._GetWord(pos);
		actual_mask = 1;
	}
	while(pos < bitvector._NBits && !(actual_word & actual_mask))
	{
		pos++;
		actual_mask = actual_mask << 1;
		if(!actual_mask)
		{
			actual_word = bitvector._GetWord(pos);
			actual_mask = 1;
		}

	}
	
	return *this;
}

inline nbBitVector::iterator& nbBitVector::iterator::operator++(int)
{
	pos++;
	actual_mask = actual_mask << 1;
	if(!actual_mask)
	{
		actual_word = bitvector._GetWord(pos);
		actual_mask = 1;
	}
	while(pos < bitvector._NBits && !(actual_word & actual_mask))
	{
		pos++;
		actual_mask = actual_mask << 1;
		if(!actual_mask)
		{
			actual_word = bitvector._GetWord(pos);
			actual_mask = 1;
		}
	}
	return *this;
}

inline bool nbBitVector::iterator::operator==(const nbBitVector::iterator& other) const
{
	return this->pos == other.pos;
}

inline bool nbBitVector::iterator::operator!=(const nbBitVector::iterator& other) const
{
	return this->pos != other.pos;
}

inline bool nbBitVector::empty() const
{
	for(uint32_t i = 0; i < _NWords; i++)
	{
		if(_Bits[i] != 0)
			return false;
	}

	return true;
}

