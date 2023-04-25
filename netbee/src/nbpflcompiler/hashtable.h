/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"

#include <utility>
using namespace std;

#if (defined(_WIN32) || defined(_WIN64))
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

enum 
{
	NO_OWN_ITEMS = false,
	OWN_ITEMS = true
};

/*!
	\brief This template class represents a generic element of the \ref HashTable

	The Entry class associates an Item to a Key. 
*/

template <class TKey, class TItem>
struct Entry
{
	TKey	Key;
	TItem	Item;

	//!\brief Constructor
	Entry(const TKey &key, const TItem &item)
		: Key(key), Item(item){}

	//!\brief Copy Constructor
	Entry(const Entry &sym)
		: Key(sym.Key), Item(sym.Item){}
};



/*!
	\brief This template class implements a generic Hash Table

	The single element of the table associates an object of class TItem
	to an object of class TKey. 
*/

template <class TKey, class TItem>
class HashTable
{

public:
	class Iterator; //fw decl
	friend class Iterator;

	class Iterator 
	{
		friend class HashTable;

		typename std::tr1::unordered_map<TKey, TItem>::iterator _i;
		HashTable	_ht;

		Entry<TKey, TItem> current() const 
		{ 
			return Entry<TKey, TItem>(_i->first, _i->second);
		}

	public:
		Iterator(HashTable& ht)
			: _i(ht.m_HashTable.begin()), _ht(ht) {}

		// To create the "end sentinel" Iterator:
		Iterator(HashTable& ht, bool) 
			: _i(ht.m_HashTable.end()), _ht(ht) {}

		Entry<TKey, TItem> operator++() 
		{ // Prefix
			// [ds] removed assertion, it failed in case of moving beyond the last element (this should be tested after the ++)
			//nbASSERT(_i != _ht.m_HashTable.end(), "Iterator moved out of range");
			_i++;
			return Entry<TKey, TItem>(_i->first, _i->second);
		}
	
		Entry<TKey, TItem> operator++(int) 
		{ // Postfix
			// [ds] removed assertion, it failed in case of moving beyond the last element (this should be tested after the ++)
			//nbASSERT(_i != _ht.m_HashTable.end(), "Iterator moved out of range");
			Entry<TKey, TItem> e(_i->first, _i->second);
			_i++;
			return e;
		}

		Entry<TKey, TItem> operator* () const 
		{ 
			return current();
		}

		// To see if you're at the end:
		bool operator==(const Iterator& rv) const 
		{
			return _i == rv._i;
		}

		bool operator!=(const Iterator& rv) const 
		{
			return _i != rv._i;
		}

	};


	/*! 
		\brief The constructor
		
	*/

	HashTable(bool ownsItems = NO_OWN_ITEMS)
		:m_OwnsItems(ownsItems){}
	
	

	/*! 
		\brief The destructor
	*/
	~HashTable()
	{
#if 0
		if (!m_OwnsItems)
			return;
		
		for (Iterator i = Begin(); i != End(); i++)
			delete i.current().Item;
#endif
	}
	

	 Iterator Begin() 
	 { 
		 return Iterator(*this); 
	 }
	 
	 // Create the "end sentinel":
	 Iterator End() 
	 { 
		 return Iterator(*this, true);
	 }
	
	/*! 
		\brief Inserts a entry into the table
		
		\param entry	The couple key-item to insert

		\return true if the key ht found inside the hash table, false otherwise
		
	*/

	bool Insert(Entry<TKey, TItem> &entry);


#define ENTRY_ADD		true
#define ENTRY_LOOKUP	false  


	/*! 
		\brief Performs a lookup inside the table, eventually inserting an object
		
		\param key		The key to search

		\param value	(input/output)	A reference to the item to insert into the table associating it to key (input), 
						or a reference to the object already associated to the key (output)

		\return			true if the key ht found inside the entry table, false otherwise

		The default behaviour for this member function ht lookup. If add ht true, it performs
		a lookup + an insert. 
		
	*/
	bool LookUp(TKey &key, TItem &item, bool add = ENTRY_LOOKUP);


	/*! 
		\brief Deletes an entry from the table
		
		\param key		The key to be deleted

		\return			the number of elements deleted
		
	*/
	uint32 Delete(TKey &key);


protected:

	bool					m_OwnsItems;	//<! if true the the Entry Table owns the contained items and *MUST* deallocate them
	std::tr1::unordered_map<TKey, TItem>	m_HashTable;	//<! The actual hash table that implements the entry table

};



template <class TKey, class TItem>
bool HashTable<TKey, TItem>::Insert(Entry<TKey, TItem> &entry)
{
	return LookUp(entry.Key, entry.Item, ENTRY_ADD);
}


template <class TKey, class TItem>
bool HashTable<TKey, TItem>::LookUp(TKey &key, TItem &item, bool add)
{

	if (add)
	{
		pair< typename std::tr1::unordered_map<TKey, TItem>::iterator, bool> result;
		/* result is a pair: 
			* the first element of the pair (result.first) is itself a pair <key, element>
			* the second element of the pair (result.second) is a boolean value that indicates if the key was already present or not
		*/
		result = m_HashTable.insert(make_pair<TKey, TItem>(key, item));
		(result.first)->second = item;
		return !result.second;
	}
	else
	{
		typename std::tr1::unordered_map<TKey, TItem>::iterator i;
		i = m_HashTable.find(key);
		if (i == m_HashTable.end())
		{
			return false;
		}
		else
		{
			item = i->second;
			return true;
		}

	}
}


template <class TKey, class TItem>
uint32 HashTable<TKey, TItem>::Delete(TKey &key)
{
	return m_HashTable.erase(key);
}

