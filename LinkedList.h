#pragma once
#include "stdlib.h"

//Object that represents a linked list.
typedef struct LinkedListNode
{
	void* Element;
	struct LinkedListNode* Next;
	struct LinkedListNode* Previous;
} LinkedListNode;

LinkedListNode* New_LinkedListNode()
{
	LinkedListNode* node = malloc(sizeof(LinkedListNode)).Start;
	node->Element = NULL;
	node->Next = NULL;
	node->Previous = NULL;
	return node;
}

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

LinkedListNode* LinkedListNode_First(LinkedListNode* node)
{
	LinkedListNode* current = node;

	while (current->Previous != NULL) current = current->Previous;

	return current;
}

LinkedListNode* LinkedListNode_Last(LinkedListNode* node)
{
	LinkedListNode* current = node;

	while (current->Next != NULL) current = current->Next;

	return current;
}

void LinkedListNode_AddNext(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* next = node->Next;

	node->Next = child;
	child->Next = next;
	child->Previous = node;

	if (next != NULL) next->Previous = child;
}

void LinkedListNode_AddPrevious(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* prev = node->Previous;

	node->Previous = child;
	child->Previous = prev;
	child->Next = node;

	if (prev != NULL) prev->Next = child;
}

void LinkedListNode_AddFirst(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* current = LinkedListNode_Last(node);

	child->Next = current;
	current->Previous = child;
}

void LinkedListNode_AddLast(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* current = LinkedListNode_First(node);

	child->Previous = current;
	current->Next = child;
}

void LinkedListNode_Remove(LinkedListNode* node)
{
	if (node->Previous != NULL) node->Previous->Next = node->Next;
	if (node->Next != NULL) node->Next->Previous = node->Previous;

	node->Next = NULL;
	node->Previous = NULL;

	Dispose_LinkedListNode(node);
}