/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "strutils.h"

#include "hashtable.h"
#include <string>
#include <utility>

using namespace std;

/*!
	\brief This template class implements a generic Symbol Table as a specialization of the HashTable template
	class. 

	The single element of the symbol table associates an object of class T
	to a string key.
*/


template <class T> 
class StrSymbolTable: public HashTable<string, T>
{
private:
	bool m_CaseMatch;

public:

	/*! 
		\brief The constructor

		\param caseMatch if true the keys are treated as case sensitive, so the string "hello" is not equal to "HELLO"
		The default value for this parameter is false (keys are not case sensitive)
		
	*/

	StrSymbolTable(bool ownsItems = 0, bool caseMatch =1)
		:HashTable<string, T>(ownsItems), m_CaseMatch(caseMatch){}
	
	
	
	/*! 
		\brief Performs a lookup inside the table, eventually inserting an object
		
		\param key		The string key to search

		\param value (input/output)	A reference to the object to insert into the table (input), 
		              or a reference to the object already associated to the key (output)

		\return true if the key is found inside the symbol table, false otherwise

		The default behaviour for this member function is lookup. If add is true, it performs
		a lookup + an insert. 
		
	*/

	bool LookUp(const string &key, T &item, bool add = 0);


	/*! 
		\brief Performs a lookup inside the table, eventually inserting an object
		
		\param key		The (const char *) key to search

		\param value (input/output)	A reference to the object to insert into the table (input), 
		              or a reference to the object already associated to the key (output)

		\return true if the key is found inside the symbol table, false otherwise

		The default behaviour for this member function is lookup. If add is true, it performs
		a lookup + an insert. 
		
	*/

	bool LookUp(const char *key, T &item, bool add = 0)
	{
		return LookUp(string(key), item, add);
	}

};



template <class T>
bool StrSymbolTable<T>::LookUp(const string &key, T &value, bool add)
{

	string keyStr;

	if (m_CaseMatch)
	{
		keyStr = key;
	}
	else
	{
		keyStr = ToLower(key);
	}


	return HashTable<string, T>::LookUp(keyStr, value, add);

}

