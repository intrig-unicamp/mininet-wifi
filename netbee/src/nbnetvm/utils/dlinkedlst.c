/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file dlinkedlst.c
 * \brief This file contains the functions that implement DoubLy Linked Lists management
 */

#include <stdio.h>
#include <stdlib.h>
#include "lists.h"
#include "../../nbee/globals/debug.h"




DLLstElement * DLLst_New_Item( DLinkedList * list)
{
	return CALLOC(1, sizeof(DLLstElement));
}



DLinkedList * New_DoubLinkedList(uint32_t sorted)
{
	DLinkedList *list;

	list = CALLOC(1, sizeof(DLinkedList));
	if (list == NULL)
		return NULL;
	list->Head = NULL;
	list->Tail = NULL;
	list->NumElems = 0;
	list->Sorted = sorted;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "New Doubly Linked Lst 0x%p, Sorted: %u",list, list->Sorted);
#endif

	return list;
}



void Free_DoubLinkedList(DLinkedList * list, void (*freeItem)(void*))
{
	DLLstElement *currItem, *nextItem;

	if (list == NULL)
		return;

	currItem = list->Head;
	nextItem = list->Head;

	while (nextItem != NULL){
		nextItem = currItem->Next;
		if (freeItem != NULL)			//we may not want to free the linked items
			freeItem(currItem->Item);
		free (currItem);
		currItem = nextItem;
	}
	
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Freed Doubly Linked Lst 0x%p, Num Elements: %u",list, list->NumElems);
#endif

	free(list);
}




uint32_t DLLst_Is_Empty(DLinkedList * list)
{
	if (list == NULL)
		return TRUE;

	else if (list->Head == NULL || list->NumElems == 0 || list->Tail == NULL){
		NETVM_ASSERT(list->Head == NULL && list->Tail == NULL && list->NumElems == 0, __FUNCTION__ "The list may be empty, but has wrong member values!");
		return TRUE;
	}

	else 
		return FALSE;
}




DLLstElement *DLLst_Add_Tail(DLinkedList * list)
{
	DLLstElement *listItem;

	if (list == NULL)
		return NULL;

	listItem = DLLst_New_Item(list);
	if (listItem == NULL)
		return NULL;

	listItem->Item = NULL;
	listItem->Next =  NULL;

	if (list->Head == NULL && list->Tail == NULL){
		list->Head = listItem;
		list->Tail = listItem;
		listItem->Prev = NULL;
	}
	else{
		list->Tail->Next = listItem;
		listItem->Prev = list->Tail;
		list->Tail = listItem;
	}

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element to tail of Doubly Linked Lst 0x%p, Num Elements: %u",list, list->NumElems);
#endif

	return listItem;
}



DLLstElement * DLLst_Add_Front(DLinkedList * list)
{
	DLLstElement *listItem;

	if (list == NULL)
		return NULL;

	listItem = DLLst_New_Item(list);
	if (listItem == NULL)
		return NULL;

	listItem->Item = NULL;
	listItem->Prev = NULL;

	if (list->Head == NULL && list->Tail == NULL)
	{
		list->Head = listItem;
		list->Tail = listItem;
		listItem->Next =  NULL;
	}
	else
	{
		listItem->Next =  list->Head;
		list->Head->Prev = listItem;
		list->Head = listItem;
	}

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element to front of Doubly Linked Lst 0x%p, Num Elements: %u",list, list->NumElems);
#endif

	return listItem;
}


DLLstElement * DLLst_Ins_After(DLinkedList * list, DLLstElement * listItem)
{
	DLLstElement *newItem, *nextItem;

	if (list == NULL || listItem == NULL)
		return NULL;

	if (DLLst_Is_Empty(list))
		return NULL;
	
	nextItem = listItem->Next;

	if (nextItem == NULL)
		return DLLst_Add_Tail(list);

	newItem = DLLst_New_Item(list);
	if (newItem == NULL)
		return NULL;

	newItem->Item = NULL;
	
	nextItem->Prev = newItem;
	newItem->Next = nextItem;
	listItem->Next =  newItem;
	newItem->Prev = listItem;

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element after element 0x%p of Doubly Linked Lst 0x%p, Num Elements: %u", listItem, list, list->NumElems);
#endif

	return newItem;

}


DLLstElement * DLLst_Ins_Before(DLinkedList * list, DLLstElement * listItem)
{
	DLLstElement *newItem, *prevItem;

	if (list == NULL || listItem == NULL)
		return NULL;

	if (list->Head == NULL && list->Tail == NULL)
		return NULL;
	
	prevItem = listItem->Prev;

	if (prevItem == NULL)
		return DLLst_Add_Front(list);

	newItem = DLLst_New_Item(list);
	if (newItem == NULL)
		return NULL;

	newItem->Item = NULL;
	
	prevItem->Next = newItem;
	newItem->Prev = prevItem;
	newItem->Next = listItem;
	listItem->Prev = newItem;

	list->NumElems++;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Added an element before element 0x%p of Doubly Linked Lst 0x%p, Num Elements: %u", listItem, list, list->NumElems);
#endif

	return newItem;
}


DLLstElement * DLLst_Del_Item(DLinkedList * list, DLLstElement * listItem, void (*freeItem)(void*))
{
	DLLstElement *prevItem, *nextItem;


	if (list == NULL || listItem == NULL)			
		return NULL;

	if (DLLst_Is_Empty(list)){	// empty list
		NETVM_ASSERT(0 == 1, __FUNCTION__ " trying to delete an element from an empty list");
		return NULL;
	}

	prevItem = listItem->Prev;
	nextItem = listItem->Next;
	

	if (prevItem == NULL){							// first Item
		list->Head = nextItem;
	}
	else{
		prevItem->Next = nextItem;
	}

	if (nextItem == NULL){							// last Item
		list->Tail = prevItem;
	}
	else{
		nextItem->Prev = prevItem;
	}

	if (freeItem != NULL)
		freeItem(listItem->Item);		//we may not want to free the linked item

	free(listItem);
	list->NumElems--;
	
	NETVM_ASSERT(((int32_t)list->NumElems) >= 0, __FUNCTION__ " number of elements in the list is < 0!");

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, "Deleted element 0x%p from doubly linked list %p, Num elements now: %u, Head: 0x%p, Tail: 0x%p", listItem, list, list->NumElems);
#endif

	return prevItem;
}





int32_t DLLst_Add_Item_Tail(DLinkedList * list, void* item)
{
	DLLstElement *listItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	listItem = DLLst_Add_Tail(list);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error adding item to tail of Doubly linked list\n");
		return nvmJitFAILURE;
	}

	listItem->Item = item;

	return nvmJitSUCCESS;
}



int32_t DLLst_Add_Item_Front(DLinkedList * list, void* item)
{
	DLLstElement *listItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	listItem = DLLst_Add_Front(list);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error adding item in front of Doubly linked list\n");
		return nvmJitFAILURE;
	}

	listItem->Item = item;

	return nvmJitSUCCESS;
}

int32_t DLLst_Ins_Item_After(DLinkedList * list, DLLstElement * listItem, void* item)
{
	DLLstElement *insertedItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	insertedItem = DLLst_Ins_After(list, listItem);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error inserting item after an element of doubly linked list\n");
		return nvmJitFAILURE;
	}

	insertedItem->Item = item;

	return nvmJitSUCCESS;
}


int32_t DLLst_Ins_Item_Before(DLinkedList * list, DLLstElement * listItem, void* item)
{
	DLLstElement *insertedItem;
	
	if (list == NULL || item == NULL)
		return nvmJitFAILURE;

	insertedItem = DLLst_Ins_Before(list, listItem);

	if (listItem == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error inserting item before an element of doubly linked list\n");
		return nvmJitFAILURE;
	}
	insertedItem->Item = item;

	return nvmJitSUCCESS;
}


DLLstElement * DLLst_Find_Item_Sorted(DLinkedList * list, void *item, int32_t (*Cmp_Item)(void*, void*))
{
	DLLstElement *currItem;

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




DLLstElement * DLLst_Find_Item(DLinkedList * list, void *item, int32_t (*Cmp_Item)(void*, void*))
{
	DLLstElement *currItem;

	if (list == NULL || Cmp_Item == NULL || item == NULL)
		return NULL;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	if (list->Sorted)
		return DLLst_Find_Item_Sorted(list, item, Cmp_Item);

	currItem = list->Head;

	while (currItem != NULL && Cmp_Item(item, currItem->Item) != 0)	
		currItem = currItem->Next;
	
	if (currItem == NULL)
		return NULL;


	return currItem;		

}






DLLstElement * DLLst_Insert_Item_Sorted(DLinkedList * list, void *item, uint32_t duplicates, int32_t (*Cmp_Item)(void*, void*))
{
	DLLstElement *currItem, *newItem;
	
	if (list == NULL || Cmp_Item == NULL || item == NULL)
		return NULL;
	
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	NETVM_ASSERT(list->Sorted, __FUNCTION__ " cannot be used with a non-sorted list");

	if (!list->Sorted)
		return NULL;


	currItem = list->Head;
	if (currItem == NULL){
		newItem = DLLst_Add_Tail(list);
		if (newItem == NULL)
			return NULL;
		newItem->Item = item;
		return newItem;
	}
	while (currItem != NULL && Cmp_Item(item, currItem->Item) > 0){
		currItem = currItem->Next;
	}
	
	if (currItem == list->Head){
		newItem = DLLst_Add_Front(list);
		if (newItem == NULL)
			return NULL;
		newItem->Item = item;
		return newItem;
	}

	if (currItem == NULL){
		newItem = DLLst_Add_Tail(list);
		if (newItem == NULL)
			return NULL;
		newItem->Item = item;
		return newItem;
	}
	else if ((Cmp_Item(item, currItem->Item) == 0) && !duplicates)
		return currItem;

	newItem = DLLst_Ins_Before(list, currItem);
	if (newItem == NULL)
		return NULL;
	newItem->Item = item;
	return newItem;
	
}



int32_t DLLst_Iterate_Forward_1Arg(DLinkedList * list, int32_t (*funct)(void *, void*), void *arg)
{
DLLstElement * listItem, *next;

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

	if (funct == NULL){
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}


	listItem = list->Head;

	while (listItem != NULL){
		next = listItem->Next;
		if (funct(listItem->Item, arg) < 0){
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;
	}

	return nvmJitSUCCESS;
}


int32_t DLLst_Iterate_Reverse_1Arg(DLinkedList * list, int32_t (*funct)(void*, void*), void *arg)
{
DLLstElement * listItem, *prev;

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

	listItem = list->Tail;

	while (listItem != NULL){
		prev = listItem->Prev;
		if (funct(listItem->Item, arg) < 0){
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = prev;
	}

	return nvmJitSUCCESS;
}




int32_t DLLst_Iterate_Forward_2Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*), void *arg1, void *arg2)
{
	DLLstElement * listItem, *next;

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
		if (funct(listItem->Item, arg1, arg2) < 0)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;
	}

	return nvmJitSUCCESS;
}


int32_t DLLst_Iterate_Reverse_2Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*), void *arg1, void *arg2)
{
DLLstElement * listItem, *prev;

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

	listItem = list->Tail;

	while (listItem != NULL)
	{
		prev = listItem->Prev;
		if (funct(listItem->Item, arg1, arg2) < 0)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = prev;
	}

	return nvmJitSUCCESS;
}


int32_t DLLst_Iterate_Forward_3Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*), void *arg1, void *arg2, void *arg3)
{
DLLstElement * listItem, *next;

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
		if (funct(listItem->Item, arg1, arg2, arg3) < 0){
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;
	}

	return nvmJitSUCCESS;
}


int32_t DLLst_Iterate_Reverse_3Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*), void *arg1, void *arg2, void *arg3)
{
DLLstElement * listItem, *prev;

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

	listItem = list->Tail;

	while (listItem != NULL)
	{
		prev = listItem->Prev;
		if (funct(listItem->Item, arg1, arg2, arg3) < 0)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = prev;
	}

	return nvmJitSUCCESS;
}



int32_t DLLst_Iterate_Forward_4Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*, void*), void *arg1, void *arg2, void *arg3, void* arg4)
{
DLLstElement * listItem, *next;

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
		if (funct(listItem->Item, arg1, arg2, arg3, arg4) < 0)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = next;
	}

	return nvmJitSUCCESS;
}


int32_t DLLst_Iterate_Reverse_4Args(DLinkedList * list, int32_t (*funct)(void *, void*, void*, void*, void*), void *arg1, void *arg2, void *arg3, void* arg4)
{
DLLstElement * listItem, *prev;

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

	listItem = list->Tail;

	while (listItem != NULL){
		prev = listItem->Prev;
		if (funct(listItem->Item, arg1, arg2, arg3, arg4) < 0){
			errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: applying function on items\n");
			return nvmJitFAILURE;
		}
		listItem = prev;
	}

	return nvmJitSUCCESS;
}


void DLLst_Swap_Items(DLLstElement * item1, DLLstElement * item2, void (*updtItem)(DLLstElement *))
{
void *temp;
	
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_LISTS_DEBUG_LVL, __FUNCTION__);
#endif

	temp = item1->Item;
	item1->Item = item2->Item;
	item2->Item = temp;
	if (updtItem != NULL)
	{
		updtItem(item1);
		updtItem(item2);
	}
}




int32_t DLLst_Sort_List(DLinkedList * list, int32_t (*CmpFunct)(void*, void*), void (*updtItem)(DLLstElement *))
{
DLLstElement * currItem, *nextItem;

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

	if (updtItem == NULL)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, " Error: null function ptr argument\n");
		return nvmJitFAILURE;
	}
	
	currItem = list->Head;

	while (currItem != NULL)
	{
		nextItem = currItem->Next;

		while (nextItem != NULL)
		{
			if (CmpFunct(currItem->Item, nextItem->Item) > 0)
				DLLst_Swap_Items(currItem, nextItem, updtItem);
			nextItem = nextItem->Next;
		}
		currItem = currItem->Next;
	}
	list->Sorted = TRUE;
	return nvmJitSUCCESS;
}

