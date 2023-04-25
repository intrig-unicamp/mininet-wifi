/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include <stdint.h>
#include "../jit/netvmjitglobals.h"		//FULVIo 13/01/2011 che senso ha includere questo file solo x TRUE,FALSE???


/** @file lists.h
 * \brief This file contains prototypes, definitions and data structures for
 *  the implementation of singly and doubly linked lists broadly used in the
 *  internal data structures of the NetVM JIT (cfg, edges, instruction sequences...)
 */


//! Aliases for TRUE and FALSE that tell if a list is sorted or not
enum
{
	SORTED_LST		=	TRUE,
	NOT_SORTED_LST	=	FALSE
};




#ifdef __cplusplus
extern "C" {
#endif



/*!
	\defgroup NetVMJitLists NetVMJit - Lists Management
*/


/*! \addtogroup NetVMJitLists
	\{

	<p>Linked Lists are the data structures more widely employed in the NetVM Jit compiler, so
	this implementation is as much generic as possible.<br>
	We have two types of lists: Singly Linked Lists and Doubly Linked Lists. The firsts can
	be traversed in only one direction, from Head to Tail, while the seconds can be traversed
	in two directions: from Head to Tail and vice versa.<br>
	List elements are generic and each one contain a (void) pointer to the item's data.
	Many functions are provided for lists manipulation, such as functions for insertion, deletion and search
	as well	as iterators over the lists elements.<br>
	Iterators accept as arguments a pointer to a client defined function to apply
	to the items in the list, and from one to four arguments to pass to such function.<br>
	Here is an example of this kind of design. Let's suppose that we have a singly linked list of integers
	and we want the sum of all the elements. We have to define a function like this:
	<pre>
		int32_t Sum_Elems(int32_t *item, int32_t *sum)
		{
			(*sum) += (*item);
			return 0;
		}

	</pre>
	<p>and in a client procedure we call the iterator:
	<pre>
		void function(void)
		{
			int32_t sum = 0;
			SLinkedList myList;
			[...]
			SLLst_Iterate_1Arg(myList, Sum_Elems, &sum);
			[...]
		}
	</pre>
	<p>The iterator will apply the function Sum_Elems to every integer item of the list. <br>
	It's always up to the client functions to determine the semantic of the items contained into the list.
*/


/*!
  \brief Singly Linked List Element
 */

typedef struct SLLstElement
{
	void		*Item;			//!< Pointer to a generic item
	void		*Next;			//!< Pointer to the next element in the list
} SLLstElement;



/*!
  \brief Doubly Linked List Element
 */

typedef struct DLLstElement
{
	void		*Item;			//!< Pointer to a generic item
	void		*Next;			//!< Pointer to the next element in the list
	void		*Prev;			//!< Pointer to the previous element in the list
} DLLstElement;



/*!
  \brief Singly Linked List
 */

typedef struct SLinkedList
{
	SLLstElement	*Head;		//!< Pointer to the first element of the list
	SLLstElement	*Tail;		//!< Pointer to the last element of the list
	uint32_t		NumElems;	//!< Number of elements in the list
	uint32_t		Sorted;		//!< TRUE if the list is sorted
}SLinkedList;



/*!
  \brief Doubly Linked List
 */

typedef struct DLinkedList
{
	DLLstElement	*Head;		//!< Pointer to the first element of the list
	DLLstElement	*Tail;		//!< Pointer to the last element of the list
	uint32_t		NumElems;	//!< Number of elements in the list
	uint32_t		Sorted;		//!< TRUE if the list is sorted
}DLinkedList;





/*-------------------------------------------*
*                                            *
*            DOUBLY LINKED LISTS:            *
*                                            *
*--------------------------------------------*/



/*!
  \brief Creates a new Doubly Linked List

  \param sorted		tells if the list to be created will be sorted or not

  \return a pointer to a newly created DLinkedList structure
*/

DLinkedList * New_DoubLinkedList(uint32_t sorted);



/*!
  \brief Frees a Doubly Linked List

  \param list		pointer to a DLinkedList

  \param freeItem	pointer to a function defined by the client to free the items in the list (can be NULL if the items have not to be freed)

  \return nothing
*/
typedef void (nvmFreeFunct)(void *);

void Free_DoubLinkedList(DLinkedList * list, void (*freeItem)(void*));


/*!
  \brief Tells if a Doubly Linked List is empty

  \param list		pointer to a DLinkedList

  \return TRUE if the list is empty, FALSE otherwise
*/

uint32_t DLLst_Is_Empty(DLinkedList * list);



/*!
  \brief Adds an element to the tail of a list

  \param list	pointer to a DLinkedList

  \return a pointer to the added element (NULL if it fails)
*/

DLLstElement * DLLst_Add_Tail(DLinkedList * list);



/*!
  \brief Adds an element to the head of a list

  \param list	pointer to a DLinkedList

  \return a pointer to the added element (NULL if it fails)
*/

DLLstElement * DLLst_Add_Front(DLinkedList * list);



/*!
  \brief Inserts an element after another element in a list

  \param list		pointer to a DLinkedList

  \param listItem	pointer to the element after which to insert the new element

  \return a pointer to the added element (NULL if it fails)
*/

DLLstElement * DLLst_Ins_After(DLinkedList * list, DLLstElement * listItem);



/*!
  \brief Inserts an element before another element in a list

  \param list	pointer to a DLinkedList

  \param listItem	pointer to the element before which to insert the new element

  \return a pointer to the added element (NULL if it fails)
*/

DLLstElement * DLLst_Ins_Before(DLinkedList * list, DLLstElement * listItem);



/*!
  \brief Adds an item to the Head of a list

  \param list	pointer to a DLinkedList

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/


int32_t DLLst_Add_Item_Front(DLinkedList * list, void* item);



/*!
  \brief Adds an item to the Tail of a list

  \param list	pointer to a DLinkedList

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t DLLst_Add_Item_Tail(DLinkedList * list, void* item);



/*!
  \brief Inserts an item before an element in a list

  \param list		pointer to a DLinkedList

  \param listItem	pointer to the element before which to insert the new element

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t DLLst_Ins_Item_Before(DLinkedList * list, DLLstElement * listItem, void* item);



/*!
  \brief Inserts an item after an element in a list

  \param list		pointer to a DLinkedList

  \param listItem	pointer to the element after which to insert the new element

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t DLLst_Ins_Item_After(DLinkedList * list, DLLstElement * listItem, void* item);



/*!
  \brief Deletes an element from a list

  \param list		pointer to a DLinkedList

  \param listItem	pointer to the element to delete

   \param freeItem	pointer to a function defined by the client to free the linked item (can be NULL if the item has not to be freed)

  \return a pointer to the element that precede the deleted element in the list (NULL if the deleted element was the Head)
*/

DLLstElement * DLLst_Del_Item(DLinkedList * list, DLLstElement * listItem, void (*freeItem)(void*));



/*!
  \brief Inserts an item in a sorted list

  \param list		pointer to a DLinkedList

  \param item		pointer to the item to add

  \param duplicates if TRUE multiple identical items are allowed, if FALSE duplicates are not allowed

  \param Cmp_Item	a client defined function to compare items in the list
					(this function MUST return a value > 0 if the first item is greater than the second,
					 a value == 0 if the two items are equals and a value < 0 in the last case)

  \return a pointer to the added element (NULL if it fails)
*/

DLLstElement * DLLst_Insert_Item_Sorted(DLinkedList * list, void *item, uint32_t duplicates, int32_t (*Cmp_Item)(void*, void*));




/*!
  \brief Iterates over the list from Head to Tail applying to elements a 2 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg		the extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/
typedef int32_t (nvmIteratefunct1Arg)(void *, void *);

int32_t DLLst_Iterate_Forward_1Arg(DLinkedList * list, int32_t (*funct)(void *, void*), void *arg);



/*!
  \brief Iterates over the list from Tail to Head applying to elements a 2 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg		the extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t DLLst_Iterate_Reverse_1Arg(DLinkedList * list, int32_t (*funct)(void *, void*), void *arg);


/*!
  \brief Iterates over the list from Head to Tail applying to elements a 3 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

typedef int32_t (nvmIteratefunct2Args)(void *, void *, void *);
int32_t DLLst_Iterate_Forward_2Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*), void *arg1, void *arg2);



/*!
  \brief Iterates over the list from Tail to Head applying to elements a 3 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t DLLst_Iterate_Reverse_2Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*), void *arg1, void *arg2);



/*!
  \brief Iterates over the list from Head to Tail applying to elements a 4 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \param arg3		the third extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

typedef int32_t (nvmIteratefunct3Args)(void *, void *, void *, void *);
int32_t DLLst_Iterate_Forward_3Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*), void *arg1, void *arg2, void *arg3);



/*!
  \brief Iterates over the list from Tail to Head applying to elements a 4 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \param arg3		the third extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t DLLst_Iterate_Reverse_3Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*), void *arg1, void *arg2, void *arg3);



/*!
  \brief Iterates over the list from Head to Tail applying to elements a 5 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \param arg3		the third extra argument that has to be passed to the function

  \param arg4		the fourth extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

typedef int32_t (nvmIteratefunct4Args)(void *, void *, void *, void *, void *);
int32_t DLLst_Iterate_Forward_4Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*, void*), void *arg1, void *arg2, void *arg3, void* arg4);



/*!
  \brief Iterates over the list from Tail to Head applying to elements a 5 args client defined function

  \param list		pointer to a DLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \param arg3		the third extra argument that has to be passed to the function

  \param arg4		the fourth extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t DLLst_Iterate_Reverse_4Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*, void*), void *arg1, void *arg2, void *arg3, void* arg4);



/*!
  \brief Sorts items in a list

  \param list		pointer to a DLinkedList

  \param CmpFunct	a client defined function to compare items in the list
					(this function MUST return a value > 0 if the first item is greater than the second,
					 a value == 0 if the two items are equals and a value < 0 in the last case)

  \param updtItem	a client defined function for updating elements in the list to reflect the sorting effects on items

  \return nvmJitSUCCESS if the sort operation succeeds, nvmJitFAILURE if it fails
*/

int32_t DLLst_Sort_List(DLinkedList * list, int32_t (*CmpFunct)(void*, void*), void (*updtItem)(DLLstElement *));






/*-------------------------------------------*
*                                            *
*            SINGLY LINKED LISTS:            *
*                                            *
*--------------------------------------------*/
/*!
  \brief Creates a new Singly Linked List

  \param sorted		tells if the list to be created will be sorted or not

  \return a pointer to a newly created SLinkedList structure
*/

SLinkedList * New_SingLinkedList(uint32_t sorted);


/*!
  \brief Frees a Singly Linked List

  \param list		pointer to a SLinkedList

  \param freeItem	pointer to a function defined by the client to free the items in the list (can be NULL if the items have not to be freed)

  \return nothing
*/
typedef void (nvmFreefunctSLL)(void *);

void Free_SingLinkedList(SLinkedList * list, void (*freeItem)(void*));


/*!
  \brief Tells if a Singly Linked List is empty

  \param list		pointer to a SLinkedList

  \return TRUE if the list is empty, FALSE otherwise
*/

uint32_t SLLst_Is_Empty(SLinkedList * list);

SLLstElement * SLLst_Del_Item(SLinkedList * list, SLLstElement * listItem);



/*!
  \brief Adds an item to the Head of a list

  \param list	pointer to a SLinkedList

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t SLLst_Add_Item_Front(SLinkedList * list, void* item);



/*!
  \brief Adds an item to the Tail of a list

  \param list	pointer to a SLinkedList

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t SLLst_Add_Item_Tail(SLinkedList * list, void* item);



/*!
  \brief Inserts an item after an element in a list

  \param list		pointer to a SLinkedList

  \param listItem	pointer to the element after which to insert the new element

  \param item	pointer to the item to add

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t SLLst_Ins_Item_After(SLinkedList * list, SLLstElement * listItem, void* item);



/*!
  \brief Copies one list into another

  \param dstList	pointer to the destination SLinkedList

  \param srcList	pointer to the source SLinkedList

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t SLLst_Copy(SLinkedList * dstList, SLinkedList * srcList);



/*!
  \brief Unions together two sorted lists (i.e. sets)

  \param dstList	pointer to the destination SLinkedList

  \param srcList	pointer to the source SLinkedList

  \param Cmp_Item	a client defined function to compare items in the list
					(this function MUST return a value > 0 if the first item is greater than the second,
					 a value == 0 if the two items are equals and a value < 0 in the last case)

  \return nvmJitSUCCESS if succeeds, nvmJitFAILURE otherwise
*/

int32_t SLLst_Union_Set(SLinkedList * dstList, SLinkedList * srcList, int32_t (*Cmp_Item)(void*, void*));



/*!
  \brief Iterates over the list from Head to Tail applying to elements a 2 args client defined function

  \param list		pointer to a SLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg		the extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t SLLst_Iterate_1Arg(SLinkedList * list, int32_t (*funct)(void*, void*), void *arg);



/*!
  \brief Iterates over the list from Head to Tail applying to elements a 3 args client defined function

  \param list		pointer to a SLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/


int32_t SLLst_Iterate_2Args(SLinkedList * list, int32_t (*funct)(void *, void*, void*), void *arg1, void *arg2);



/*!
  \brief Iterates over the list from Head to Tail applying to elements a 4 args client defined function

  \param list		pointer to a SLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \param arg3		the third extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t SLLst_Iterate_3Args(SLinkedList * list, int32_t (*funct)(void *, void*, void*, void*), void *arg1, void *arg2, void *arg3);



/*!
  \brief Iterates over the list from Head to Tail applying to elements a 5 args client defined function

  \param list		pointer to a SLinkedList

  \param funct		pointer to the function to apply to elements

  \param arg1		the first extra argument that has to be passed to the function

  \param arg2		the second extra argument that has to be passed to the function

  \param arg3		the third extra argument that has to be passed to the function

  \param arg4		the fourth extra argument that has to be passed to the function

  \return nvmJitSUCCESS if the iteration succeeds, nvmJitFAILURE if it fails
*/

int32_t SLLst_Iterate_4Args(SLinkedList * list, int32_t (*funct)(void *, void*, void*, void*, void*), void *arg1, void *arg2, void *arg3, void *arg4);



/*!
  \brief Finds an item in a sorted list

  \param list		pointer to a SLinkedList

  \param item		pointer to the item to find

  \param Cmp_Item	a client defined function to compare items in the list
					(this function MUST return a value > 0 if the first item is greater than the second,
					 a value == 0 if the two items are equals and a value < 0 in the last case)

  \return a pointer to the element containing the found item, NULL if no item is found
*/

SLLstElement * SLLst_Find_Item_Sorted(SLinkedList * list, void *item, int32_t (*Cmp_Item)(void*, void*));



/*!
  \brief Finds an item in a sorted list

  \param list		pointer to a SLinkedList

  \param item		pointer to the item to find

  \param Cmp_Item	a client defined function to compare items in the list
					(this function MUST return a value > 0 if the first item is greater than the second,
					 a value == 0 if the two items are equals and a value < 0 in the last case)

  \return a pointer to the element containing the found item, NULL if no item is found
*/

SLLstElement * SLLst_Find_Item(SLinkedList * list, void *item, int32_t (*Cmp_Item)(void*, void*));



/*!
  \brief Inserts an item in a sorted list

  \param list		pointer to a SLinkedList

  \param item		pointer to the item to add

  \param duplicates if TRUE multiple identical items are allowed, if FALSE duplicates are not allowed

  \param Cmp_Item	a client defined function to compare items in the list
					(this function MUST return a value > 0 if the first item is greater than the second,
					 a value == 0 if the two items are equals and a value < 0 in the last case)

  \return a pointer to the added element (NULL if it fails)
*/

SLLstElement * SLLst_Insert_Item_Sorted(SLinkedList * list, void *item, uint32_t duplicates, int32_t (*Cmp_Item)(void*, void*));

/*!
	\}
*/

#ifdef __cplusplus
}
#endif

