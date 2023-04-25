/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file bitvectorset.cpp
 * \brief this file contains implementation of the nbBitVector class
 */
#include "bitvectorset.h"

std::ostream& operator<<(std::ostream& os, const nbBitVector& bv)
{
	for (uint32_t i = 0; i < bv._NBits; i++)
	{
		if (bv.Test(i))
		{
			os << i << " ";
		}
	}

	return os;
}

nbBitVector::nbBitVector(uint32_t numBits)
{
	//printf("nbBitVector _ctor called\n");
	_Bits = 0;
	//nbASSERT(numBits > 0, "You must provide a size greater than 0");

	_NWords = (numBits + _WORD_SIZE - 1) / _WORD_SIZE;
	_NBits = numBits;

	_Bits = new Word_t[_NWords];
	//nbASSERT(_Bits != NULL, "Memory Allocation Failure");
	Reset();
}


nbBitVector::~nbBitVector()
{
	//printf("nbBitVector _dtor called\n");
	if (_Bits)
		delete []_Bits;
}

//Copy constructor
nbBitVector::nbBitVector(const nbBitVector& other)
{
	//printf("nbBitVector Copy _ctor called\n");
	_Bits = 0;
	_NWords = other._NWords;
	_NBits = other._NBits;
	_Bits = new Word_t[_NWords];
	//nbASSERT(_Bits != NULL, "Memory Allocation Failure");
	for (uint32_t i = 0; i < _NWords; i++)
		_Bits[i] = other._Bits[i];
}

const nbBitVector operator+ (const nbBitVector& a, const nbBitVector& b)
{
	return a | b ;
}

const nbBitVector operator- (const nbBitVector& a, const nbBitVector& b)
{
	//nbASSERT(a._NBits == b._NBits, "Cannot intersect 2 bitvectors with different sizes");
	nbBitVector lval(a._NBits);
	for (uint32_t i = 0; i < lval._NWords; i++)
		lval._Bits[i] = a._Bits[i] & ~b._Bits[i];
	return lval;
}

const nbBitVector operator& (const nbBitVector& a, const nbBitVector& b)
{
	//nbASSERT(a._NBits == b._NBits, "Cannot intersect 2 bitvectors with different sizes");
	nbBitVector lval(a._NBits);
	for (uint32_t i = 0; i < lval._NWords; i++)
		lval._Bits[i] = a._Bits[i] & b._Bits[i];
	return lval;
}

const nbBitVector operator| (const nbBitVector& a, const nbBitVector& b)
{
	//nbASSERT(a._NBits == b._NBits, "Cannot union 2 bitvectors with different sizes");
	nbBitVector lval(a._NBits);
	for (uint32_t i = 0; i < lval._NWords; i++)
		lval._Bits[i] = a._Bits[i] | b._Bits[i];
	return lval;
}

const nbBitVector operator^ (const nbBitVector& a, const nbBitVector& b)
{
	//nbASSERT(a._NBits == b._NBits, "Cannot xor 2 bitvectors with different sizes");
	nbBitVector lval(a._NBits);
	for (uint32_t i = 0; i < lval._NWords; i++)
		lval._Bits[i] = a._Bits[i] ^ b._Bits[i];
	return lval;
}

nbBitVector::iterator::iterator(const nbBitVector& bitvector, bool end)
	:
		bitvector(bitvector)
{ 
	if(end)
	{
		pos = bitvector._NBits;
		actual_word = bitvector._GetWord(pos);
		actual_mask = nbBitVector::_MaskBit(pos);
	}
	else
	{
		pos = 0;
		actual_word = bitvector._GetWord(pos);
		actual_mask = nbBitVector::_MaskBit(pos);
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
	}
}

nbBitVector::iterator::iterator(const nbBitVector::iterator& it)
	:
		bitvector(it.bitvector),
		pos(it.pos), 
		actual_word(it.bitvector._GetWord(pos)),
		actual_mask(nbBitVector::_MaskBit(pos))
{ }


