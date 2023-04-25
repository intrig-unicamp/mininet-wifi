/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "bitmatrix.h"





nbBitMatrix::nbBitMatrix(uint32 nRows, uint32 nCols)
{
	//printf("nbBitMatrix _ctor called\n");
	nbASSERT((nRows > 0) && (nCols > 0), "You must provide a size greater than 0");

	_NRows = nRows;
	_NCols = nCols;

	_Matrix = new nbBitVector*[nRows];
	nbASSERT(_Matrix != NULL, "Memory Allocation Failure");
	for (uint32 i = 0; i < nRows; i++)
	{
		_Matrix[i] = new nbBitVector(nCols);
		nbASSERT(_Matrix[i] != NULL, "Memory Allocation Failure");
	}
}


nbBitMatrix::~nbBitMatrix()
{
	//printf("nbBitMatrix _dtor called\n");
	if (_Matrix)
	{
		for (uint32 i = 0; i < _NRows; i++)
			delete _Matrix[i];
		delete []_Matrix;
	}
}

//Copy constructor
nbBitMatrix::nbBitMatrix(const nbBitMatrix& other)
{
	//printf("nbBitMatrix Copy _ctor called\n");
	
	_NRows = other._NRows;
	_NCols = other._NCols;
	_Matrix = new nbBitVector*[_NRows];

	nbASSERT(_Matrix != NULL, "Memory Allocation Failure");

	for (uint32 i = 0; i < _NRows; i++)
	{
		_Matrix[i] = new nbBitVector(*other._Matrix[i]);
		nbASSERT(_Matrix[i] != NULL, "Memory Allocation Failure");
	}
}


const nbBitMatrix operator& (const nbBitMatrix& a, const nbBitMatrix& b)
{
	nbASSERT((a._NCols == b._NCols) && (a._NRows == b._NRows), "Cannot intersect 2 bitmatrixes with different sizes");
	
	nbBitMatrix lval(a._NRows, a._NCols);
	
	for (uint32 i = 0; i < lval._NCols; i++)
		*lval._Matrix[i] = (*a._Matrix[i]) & (*b._Matrix[i]);

	return lval;
}

const nbBitMatrix operator| (const nbBitMatrix& a, const nbBitMatrix& b)
{
	nbASSERT((a._NCols == b._NCols) && (a._NRows == b._NRows), "Cannot intersect 2 bitmatrixes with different sizes");
	
	nbBitMatrix lval(a._NRows, a._NCols);
	
	for (uint32 i = 0; i < lval._NCols; i++)
		*lval._Matrix[i] = (*a._Matrix[i]) | (*b._Matrix[i]);

	return lval;
}

const nbBitMatrix operator^ (const nbBitMatrix& a, const nbBitMatrix& b)
{
	nbASSERT((a._NCols == b._NCols) && (a._NRows == b._NRows), "Cannot intersect 2 bitmatrixes with different sizes");
	
	nbBitMatrix lval(a._NRows, a._NCols);
	
	for (uint32 i = 0; i < lval._NCols; i++)
		*lval._Matrix[i] = (*a._Matrix[i]) ^ (*b._Matrix[i]);

	return lval;
}

