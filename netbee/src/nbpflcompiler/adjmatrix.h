/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "bitvectorset.h"

class nbAdjacenceMatrix
{

/*
	The adjacence matrix is a possible representation for a graph.
	It is a square matrix with n^2 elements, where n is the number of vertex of the represented graph.
	A value of 1 at the element (i,j) implies the presence of an edge between the vertex i and the 
	vertex j.
	If the graph is a directed one, (i,j) == 1 means that there is an edge directed from i to j and
	(i,j) is in general not equal to (j,i), except if there is a loop between i and j.
	If the graph is not directed, (i,j) is always equal to (j,i) and the matrix can be reduced to a 
	triangular one, moreover the elements on the main diagonal are all 0 (the graph is acyclic).
	This implementation uses an array of bitvectors.
	
*/


private:
	nbBitVectorSet *m_Matrix;	//! an array of bitvectors

public:

	nbAdjacenceMatrix(uint32 size);
	
	void Set(uint32 i, uint32 j);
	void Clear(uint32 i, uint32 j);
	bool Test(uint32 i, uint32 j);


};

