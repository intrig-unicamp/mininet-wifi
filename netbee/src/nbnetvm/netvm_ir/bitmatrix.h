/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "bitvectorset.h"

#include <string>	
#include <cassert>

using namespace std;





class nbBitMatrix
{

	uint32_t _NRows;
	uint32_t _NCols;

	nbBitVector **_Matrix;
	
	class Reference;
	friend class Reference;

	class Reference
	{
		friend class nbBitMatrix;
		
		uint32_t _Row;
		uint32_t _Col;
		nbBitMatrix *bitmatrix;

		//left undefined
		Reference(void);

	public:
		Reference(nbBitMatrix &bm, uint32_t row, uint32_t col)
		{
			bitmatrix = &bm;
			_Row = row;
			_Col = col;
		}
		
		~Reference() 
		{}


		  // for m(i,j) = x;
		Reference& operator=(bool x) 
		{
			if (x)
				bitmatrix->Set(_Row, _Col);
			else
				bitmatrix->Reset(_Row, _Col);

			return *this;
		}

		void Set(void)
		{
			bitmatrix->Set(_Row, _Col);
		}

		void Clear(void)
		{
			bitmatrix->Reset(_Row, _Col);
		}

		// for x = m(i,j);
		operator bool() const
		{ 
			return bitmatrix->Test(_Row, _Col); 
		}
		
	};

public:
	/*!
		\brief Constructor
	*/

	nbBitMatrix(uint32_t nRows, uint32_t nCols);

	/*!
		\brief Copy constructor
	*/

	nbBitMatrix(const nbBitMatrix& other);


	/*!
		\brief Destructor
	*/
	
	~nbBitMatrix();
	
	bool Test(uint32_t row, uint32_t col) const 
	{
		assert(row < _NRows && col < _NCols && "Index out of range!");
		//nbASSERT(row < _NRows && col < _NCols, "Indexes out of range!");
			return _Matrix[row]->Test(col);
	}

	nbBitMatrix& Set(uint32_t row, uint32_t col)
	{
		assert(row < _NRows && col < _NCols && "Index out of range!");
		//nbASSERT(row < _NRows && col < _NCols, "Indexes out of range!");
		_Matrix[row]->Set(col);
		return *this;
	}

	nbBitMatrix& Reset(uint32_t row, uint32_t col)
	{
		assert(row < _NRows && col < _NCols && "Index out of range!");
		//nbASSERT(row < _NRows && col < _NCols, "Indexes out of range!");
		_Matrix[row]->Reset(col);
		return *this;
	}

	nbBitMatrix& Flip(uint32_t row, uint32_t col)
	{
		assert(row < _NRows && col < _NCols && "Index out of range!");
		//nbASSERT(row < _NRows && col < _NCols, "Indexes out of range!");
		_Matrix[row]->Flip(col);
		return *this;
	}


	nbBitMatrix& Set(void)
	{
		for (uint32_t i = 0; i < _NRows; i++)
			_Matrix[i]->Set();
		return *this;
	}

	nbBitMatrix& Reset(void)
	{
		for (uint32_t i = 0; i < _NRows; i++)
			_Matrix[i]->Reset();
		return *this;
	}

	nbBitMatrix& Flip(void) 
	{
		for (uint32_t i = 0; i < _NRows; i++)
			_Matrix[i]->Flip();
		return *this;
	}

	nbBitMatrix& operator=(const nbBitMatrix& rval)
	{
		//printf("BitMatrix Operator= \n");
		assert(&rval != this && "cannot assign a matrix to itself!");
		//nbASSERT(&rval != this, "cannot assign a matrix to itself!");
		assert(_NRows == rval._NRows && _NCols == rval._NCols && "You are assigning a bitmatrix to one of different size");
		//nbASSERT(_NRows == rval._NRows && _NCols == rval._NCols, "You are assigning a bitmatrix to one of different size");
		for (uint32_t i = 0; i < _NRows; i++)
			*_Matrix[i] = *rval._Matrix[i];
		return *this;
	}

	nbBitMatrix operator~()
	{ 
		return nbBitMatrix(*this).Flip();
	}


	nbBitMatrix& operator&=(const nbBitMatrix& rval)
	{
		assert(_NRows == rval._NRows && _NCols == rval._NCols && "Cannot intersect a bitmatrix with one of different size");
		//nbASSERT(_NRows == rval._NRows && _NCols == rval._NCols, "Cannot intersect a bitmatrix with one of different size");
		for (uint32_t i = 0; i < _NRows; i++)
			*_Matrix[i] &= *rval._Matrix[i];
		return *this;
	}

	nbBitMatrix& operator|=(const nbBitMatrix& rval) 
	{
		assert(_NRows == rval._NRows && _NCols == rval._NCols && "Cannot union a bitmatrix to one of different size");
		//nbASSERT(_NRows == rval._NRows && _NCols == rval._NCols, "Cannot union a bitmatrix to one of different size");
		for (uint32_t i = 0; i < _NRows; i++)
			*_Matrix[i] |= *rval._Matrix[i];
		return *this;
	}

	nbBitMatrix& operator^=(const nbBitMatrix& rval) 
	{
		assert(_NRows == rval._NRows && _NCols == rval._NCols && "Cannot xor a bitmatrix to one of different size");
		//nbASSERT(_NRows == rval._NRows && _NCols == rval._NCols, "Cannot xor a bitmatrix to one of different size");
		for (uint32_t i = 0; i < _NRows; i++)
			*_Matrix[i] ^= *rval._Matrix[i];
		return *this;
	}

	Reference operator()(uint32_t row, uint32_t col)
	{ 
		assert(row < _NRows && col < _NCols && "Indexes out of range!");
		//nbASSERT(row < _NRows && col < _NCols, "Indexes out of range!");
		return Reference(*this, row, col); 
	}

	bool operator()(uint32_t row, uint32_t col) const 
	{ 
		assert(row < _NRows && col < _NCols && "Indexes out of range!");
		//nbASSERT(row < _NRows && col < _NCols, "Indexes out of range!");
		return _Matrix[row]->Test(col); 
	}
	
	string ToString(void)
	{
		string tmp("");
		for (uint32_t i = 0; i < _NRows; i++)
		{
			tmp.append(_Matrix[i]->ToString());
			tmp.append("\n");
		}

		return tmp;
	}

	uint32_t GetNCols(void)
	{
		return _NCols;
	}

	uint32_t GetNRows(void)
	{
		return _NRows;
	}


	friend const nbBitMatrix operator& (const nbBitMatrix& a, const nbBitMatrix& b);

	friend const nbBitMatrix operator| (const nbBitMatrix& a, const nbBitMatrix& b);

	friend const nbBitMatrix operator^ (const nbBitMatrix& a, const nbBitMatrix& b);

};

