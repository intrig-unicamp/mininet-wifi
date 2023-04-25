/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "hashtable.h"


/*!
	\brief This template class implements a generic Symbol Table as a specialization of the HashTable template
	class. 

	The single element of the symbol table associates an object of class T
	to an integer key.
*/


template <class T> 
class IntSymbolTable: public HashTable<unsigned int, T>
{

public:

	/*! 
		\brief The constructor

		\param caseMatch if true the keys are treated as case sensitive, so the string "hello" is not equal to "HELLO"
		The default value for this parameter is false (keys are not case sensitive)
		
	*/

	IntSymbolTable(bool ownsItems = 0)
		:HashTable<unsigned int, T>(ownsItems){}
	
	
	
	/*! 
		\brief Performs a lookup inside the table, eventually inserting an object
		
		\param key		The key to search

		\param value (input/output)	A reference to the object to insert into the table (input), 
		              or a reference to the object already associated to the key (output)

		\return true if the key is found inside the symbol table, false otherwise

		The default behaviour for this member function is lookup. If add is true, it performs
		a lookup + an insert. 
		
	*/

	bool LookUp(unsigned int key, T &item, bool add = 0);



};



template <class T>
bool IntSymbolTable<T>::LookUp(unsigned int key, T &value, bool add)
{


	return HashTable<unsigned int, T>::LookUp(key, value, add);

}

