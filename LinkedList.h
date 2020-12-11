#pragma once
#include "stdlib.h"

//Object that represents a linked list.
typedef struct LinkedListNode
{
	void* Element;
	struct LinkedListNode* Next;
	struct LinkedListNode* Previous;
} LinkedListNode;

//Create a new linked list node.
LinkedListNode* New_LinkedListNode()
{
	LinkedListNode* node = malloc(sizeof(LinkedListNode)).Start;
	node->Element = NULL;
	node->Next = NULL;
	node->Previous = NULL;
	return node;
}

//Destroy a linked list node.
void Dispose_LinkedListNode(LinkedListNode* node)
{
	LinkedListNode* next = node->Next;
	LinkedListNode* prev = node->Previous;

	node->Next = NULL;
	node->Previous = NULL;

	if (next != NULL)
	{
		next->Previous = NULL;
		Dispose_LinkedListNode(next);
	}

	if (prev != NULL)
	{
		prev->Next = NULL;
		Dispose_LinkedListNode(prev);
	}

	freeany(node);
}

//Get the first node of a linked list.
LinkedListNode* LinkedListNode_First(LinkedListNode* node)
{
	LinkedListNode* current = node;

	while (current->Previous != NULL) current = current->Previous;

	return current;
}

//Get the last node of a linked list.
LinkedListNode* LinkedListNode_Last(LinkedListNode* node)
{
	LinkedListNode* current = node;

	while (current->Next != NULL) current = current->Next;

	return current;
}

//Add a node next to the specified node.
void LinkedListNode_AddNext(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* next = node->Next;

	node->Next = child;
	child->Next = next;
	child->Previous = node;

	if (next != NULL) next->Previous = child;
}

//Add a node previous to the specified node.
void LinkedListNode_AddPrevious(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* prev = node->Previous;

	node->Previous = child;
	child->Previous = prev;
	child->Next = node;

	if (prev != NULL) prev->Next = child;
}

//Add a node at the start of a linked list.
void LinkedListNode_AddFirst(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* current = LinkedListNode_Last(node);

	child->Next = current;
	current->Previous = child;
}

//Add a node at the end of a linked list.
void LinkedListNode_AddLast(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* current = LinkedListNode_First(node);

	child->Previous = current;
	current->Next = child;
}

//Remove a node from a linked list.
void LinkedListNode_Remove(LinkedListNode* node)
{
	if (node->Previous != NULL) node->Previous->Next = node->Next;
	if (node->Next != NULL) node->Next->Previous = node->Previous;

	node->Next = NULL;
	node->Previous = NULL;

	Dispose_LinkedListNode(node);
}