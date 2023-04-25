/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file slinkedlst.c
 * \brief This file contains the functions that implement Singly Linked Lists management
 */



#include <stdio.h>
#include <stdlib.h>
#include "lists.h"
#include "../../nbee/globals/debug.h"




SLLstElement * SLLst_New_Item(SLinkedList * list)
{
	return CALLOC(1, sizeof(SLLstElement));
}



SLinkedList * New_SingLinkedList(uint32_t sorted)
{
	SLinkedList *list;

	list = CALLOC(1, sizeof(SLinkedList));
	if (list == NULL)
		return NULL;
	list->Head = NULL;
	list->Tail = NULL;
	list->NumElems = 0;
	list->Sorted = sorted;
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "New Singly Linked Lst 0x%p, Sorted: %u",list, list->Sorted);
#endif
	return list;
}





void Free_SingLinkedList(SLinkedList * list, void (*freeItem)(void*))
{
SLLstElement * currItem, *nextItem;

	if (list == NULL)
		return;

	currItem = list->Head;
	nextItem = currItem;

	while (nextItem != NULL)
	{
		nextItem = currItem->Next;
		if (freeItem != NULL)			//we may not want to free the linked items
			freeItem(currItem->Item);
		free (currItem);
		currItem = nextItem;
	}

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Freed Singly Linked Lst 0x%p, Num Elements: %u",list, list->NumElems);
#endif

	free(list);
}



uint32_t SLLst_Is_Empty(SLinkedList * list)
{
	if (list == NULL)
		return TRUE;
	else
		if (list->Head == NULL || list->NumElems == 0 || list->Tail == NULL)
		{
			NETVM_ASSERT(list->Head == NULL && list->Tail == NULL && list->NumElems == 0, __FUNCTION__ " the list may be empty, but has wrong member values!");
			return TRUE;
		}
		else 
			return FALSE;
}

SLLstElement * SLLst_Add_Tail(SLinkedList * list)
{
	SLLstElement * listItem;

	if (list == NULL)
		return NULL;

	listItem = SLLst_New_Item(list);
	if (listItem == NULL)
		return NULL;

	listItem->Item = NULL;
	listItem->Next = NULL;

	if (list->Head == NULL && list->Tail == NULL){
		list->Head = listItem;
		list->Tail = listItem;
	}
	else{
		list->Tail->Next = listItem;
		list->Tail = listItem;
	}

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element to tail of Single Linked Lst 0x%p, Num Elements: %u",list, list->NumElems);
#endif

	return listItem;
}



SLLstElement * SLLst_Add_Front(SLinkedList * list)
{
	SLLstElement * listItem;

	if (list == NULL)
		return NULL;

	listItem = SLLst_New_Item(list);;
	if (listItem == NULL)
		return NULL;

	listItem->Item = NULL;

	if (list->Head == NULL && list->Tail == NULL){
		list->Head = listItem;
		list->Tail = listItem;
		listItem->Next = NULL;
	}
	else{
		listItem->Next = list->Head;
		list->Head = listItem;
	}

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element in front of Single Linked Lst 0x%p, Num Elements: %u",list, list->NumElems);
#endif

	return listItem;

}


SLLstElement * SLLst_Ins_After(SLinkedList * list, SLLstElement * listItem)
{
	SLLstElement * newItem;

	if (list == NULL || listItem == NULL)
		return NULL;

	if (list->Head == NULL && list->Tail == NULL)
		return NULL;

	if (listItem->Next == NULL)
		return SLLst_Add_Tail(list);

	newItem = SLLst_New_Item(list);;
	if (newItem == NULL)
		return NULL;

	newItem->Item = NULL;
	newItem->Next = listItem->Next;
	listItem->Next = newItem;

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element after element 0x%p of Singly Linked Lst 0x%p, Num Elements: %u", listItem, list, list->NumElems);
#endif

	return newItem;
}



SLLstElement * SLLst_Del_Item(SLinkedList * list, SLLstElement * listItem)
{
SLLstElement * prevItem;

	if (list == NULL || listItem == NULL)
		return NULL;

	if (SLLst_Is_Empty(list))
	{	// empty list
		NETVM_ASSERT(0 == 1, __FUNCTION__ " trying to delete an element from an empty list");
		return NULL;
	}

	prevItem = list->Head;
	
	while (prevItem != NULL && prevItem != listItem)
		prevItem = prevItem->Next;

	if (prevItem == NULL)
		return NULL;		// item not found!

	if (listItem->Next == NULL)
		list->Tail = prevItem;

	prevItem->Next = listItem->Next;
	free(listItem);
	list->NumElems--;

	NETVM_ASSERT(((int32_t)list->NumElems) >= 0, __FUNCTION__ " number of elements in the list is < 0!");

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Deleted element 0x%p from Singly linked list %p, Num elements now: %u, Head: 0x%p, Tail: 0x%p", listItem, list, list->NumElems);
#endif

	return prevItem;
}



int32_t SLLst_Add_Item_Tail(SLinkedList * list, void* item)
{
	SLLstElement * listItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	listItem = SLLst_Add_Tail(list);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error adding item to tail of Singly linked list\n");
		return nvmJitFAILURE;
	}

	listItem->Item = item;

	return nvmJitSUCCESS;
}

int32_t SLLst_Add_Item_Front(SLinkedList * list, void* item)
{
	SLLstElement * listItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	listItem = SLLst_Add_Front(list);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error adding item in front of Singly linked list\n");
		return nvmJitFAILURE;
	}

	listItem->Item = item;

	return nvmJitSUCCESS;
}

int32_t SLLst_Ins_Item_After(SLinkedList * list, SLLstElement * listItem, void* item)
{
	SLLstElement * insertedItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	insertedItem = SLLst_Ins_After(list, listItem);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error inserting item after an element of doubly linked list\n");
		return nvmJitFAILURE;
	}

	insertedItem->Item = item;

	return nvmJitSUCCESS;
}




SLLstElement * SLLst_Find_Item_Sorted(SLinkedList * list, void *item, int32_t (*Cmp_Item)(void*, void*))
{
SLLstElement * currItem;

	if (list == NULL || Cmp_Item == NULL || item == NULL)
		return NULL;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	NETVM_ASSERT(list->Sorted, __FUNCTION__ " cannot be used with a non-sorted list");

	if (!list->Sorted)
		return NULL;
	
	currItem = list->Head;

	while (currItem != NULL && Cmp_Item(item, currItem->Item) > 0)	
		currItem = currItem->Next;

	if (currItem == NULL)
		return NULL;

	if (Cmp_Item(item, currItem->Item) == 0)
		return currItem;

	return NULL;		//Item not found
}



SLLstElement * SLLst_Find_Item(SLinkedList * list, void *item, int32_t (*Cmp_Item)(void*, void*))
{
SLLstElement * currItem;

	if (list == NULL || Cmp_Item == NULL)
		return NULL;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (list->Sorted)
		return SLLst_Find_Item_Sorted(list, item, Cmp_Item);

	currItem = list->Head;

	while (currItem != NULL && Cmp_Item(item, currItem->Item) != 0)	
		currItem = currItem->Next;
	
	if (currItem == NULL)
		return NULL;	


	return currItem;		

}





SLLstElement * SLLst_Insert_Item_Sorted(SLinkedList * list, void *item, uint32_t duplicates, int32_t (*Cmp_Item)(void*, void*))
{
SLLstElement * prevItem, *currItem, *newItem;
	
	if (list == NULL || Cmp_Item == NULL || item == NULL)
		return NULL;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	NETVM_ASSERT(list->Sorted, __FUNCTION__ " cannot be used with a non-sorted list");

	if (!list->Sorted)
		return NULL;

	currItem = list->Head;
	prevItem = NULL;
	if (currItem == NULL){
		newItem = SLLst_Add_Tail(list);
		if (newItem == NULL)
			return NULL;
		newItem->Item = item;
		return newItem;
	}
	while (currItem != NULL && Cmp_Item(item, currItem->Item) > 0){
		prevItem = currItem;
		currItem = currItem->Next;
	}
	
	if (currItem == list->Head){
		newItem = SLLst_Add_Front(list);
		if (newItem == NULL)
			return NULL;
		newItem->Item = item;
		return newItem;
	}

	if (currItem == NULL){
		newItem = SLLst_Add_Tail(list);
		if (newItem == NULL)
			return NULL;
		newItem->Item = item;
		return newItem;
	}
	else if ((Cmp_Item(item, currItem->Item) == 0) && !duplicates)
		return currItem;		//TODO: verificare!!! se ritornare currItem o prevItem

	newItem = SLLst_Ins_After(list, prevItem);
	if (newItem == NULL)
		return NULL;
	newItem->Item = item;
	return newItem;
	
}






int32_t SLLst_Iterate_1Arg(SLinkedList * list, int32_t (*funct)(void *, void*), void *arg)
{
SLLstElement * listItem, *next;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (list == NULL)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__ "Null List... returning");
#endif
		return nvmJitSUCCESS;
	}

	if (funct == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}

	listItem = list->Head;

	while (listItem != NULL){
		next = listItem->Next;
		if(funct(listItem->Item, arg) < 0){
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;	
	}

	return nvmJitSUCCESS;
}



int32_t SLLst_Iterate_2Args(SLinkedList * list, int32_t (*funct)(void *, void*, void*), void *arg1, void *arg2)
{
SLLstElement * listItem, *next;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (list == NULL)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__ "Null List... returning");
#endif
		return nvmJitSUCCESS;
	}

	if (funct == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}

	listItem = list->Head;

	while (listItem != NULL){
		next = listItem->Next;
		if(funct(listItem->Item, arg1, arg2) < 0){
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;	
	}

	return nvmJitSUCCESS;
}


int32_t SLLst_Iterate_3Args(SLinkedList * list, int32_t (*funct)(void *, void*, void*, void*), void *arg1, void *arg2, void *arg3)
{
SLLstElement * listItem, *next;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (list == NULL)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__ "Null List... returning");
#endif
		return nvmJitSUCCESS;
	}

	if (funct == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}

	listItem = list->Head;

	while (listItem != NULL)
	{
		next = listItem->Next;
		if(funct(listItem->Item, arg1, arg2, arg3) < 0)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;	
	}

	return nvmJitSUCCESS;
}


int32_t SLLst_Iterate_4Args(SLinkedList * list, int32_t (*funct)(void *, void*, void*, void*, void*), void *arg1, void *arg2, void *arg3, void *arg4)
{
SLLstElement * listItem, *next;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (list == NULL)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__ "Null List... returning");
#endif
		return nvmJitSUCCESS;
	}

	if (funct == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}

	listItem = list->Head;

	while (listItem != NULL)
	{
		next = listItem->Next;
		if(funct(listItem->Item, arg1, arg2, arg3, arg4) < 0)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;	
	}

	return nvmJitSUCCESS;
}


int32_t SLLst_Copy(SLinkedList * dstList, SLinkedList * srcList)
{
SLLstElement * cursSrc;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (dstList == NULL || srcList == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null ptr arguments\n");
		return nvmJitFAILURE;
	}

	cursSrc = srcList->Head;

	while (cursSrc != NULL)
	{
		if (SLLst_Add_Item_Tail(dstList, cursSrc->Item) < 0)
			return nvmJitFAILURE;
		cursSrc = cursSrc->Next;
	}

	return nvmJitSUCCESS;
}



int32_t SLLst_Union_Set(SLinkedList * dstList, SLinkedList * srcList, int32_t (*Cmp_Item)(void*, void*))
{
SLLstElement * cursDst, *cursSrc, *prevDst;
int32_t cmpResult;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (dstList == NULL || srcList == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null ptr arguments\n");
		return nvmJitFAILURE;
	}

	if (Cmp_Item == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}


	NETVM_ASSERT(dstList->Sorted && srcList->Sorted, __FUNCTION__ " cannot be used with a non-sorted list");

	if (!dstList->Sorted || !srcList->Sorted)
		return nvmJitFAILURE;

	cursDst = dstList->Head;
	cursSrc = srcList->Head;
	prevDst = NULL;

	while (cursDst != NULL && cursSrc != NULL)
	{
		cmpResult = Cmp_Item(cursDst->Item, cursSrc->Item);

		if (cmpResult == 0)
		{
			cursSrc = cursSrc->Next;
		}
		else
			if (cmpResult > 0)
			{
				if (prevDst == NULL)
				{
					if (SLLst_Add_Item_Front(dstList, cursSrc->Item) < 0)
						return nvmJitFAILURE;
				}
				else{
					if (SLLst_Ins_Item_After(dstList, prevDst, cursSrc->Item) < 0)
						return nvmJitFAILURE;
				}
				cursSrc = cursSrc->Next;
			}

		prevDst = cursDst;
		cursDst = cursDst->Next;
	}

	while (cursSrc != NULL)
	{
		if (SLLst_Add_Item_Tail(dstList, cursSrc->Item) < 0)
			return nvmJitFAILURE;
		cursSrc = cursSrc->Next;
	}
	return nvmJitSUCCESS;
}
