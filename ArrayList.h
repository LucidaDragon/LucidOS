#pragma once
#include "stdlib.h"

//Object that represents an array list.
typedef struct
{
	MemBlock Data;
	UINTN Length;
	UINTN Capacity;
} ArrayList;

//Create a new array list.
ArrayList New_ArrayList()
{
	ArrayList list;
	list.Length = 0;
	list.Capacity = 1024;
	list.Data = malloc(sizeof(void*) * list.Capacity);
	return list;
}

//Destroy an array list.
void Dispose_ArrayList(ArrayList* list)
{
	list->Length = 0;
	list->Capacity = 0;
	free(&list->Data);
}

//Get an element from an array list.
void* ArrayList_Get(ArrayList list, UINTN index)
{
	return ((void**)list.Data.Start)[index];
}

//Set an element in an array list.
void ArrayList_Set(ArrayList* list, UINTN index, void* element)
{
	((void**)list->Data.Start)[index] = element;
}

//Add an element to an array list.
void ArrayList_Add(ArrayList* list, void* element)
{
	if (list->Length >= list->Capacity)
	{
		list->Data = realloc(&list->Data, list->Capacity * 2);
	}

	((void**)list->Data.Start)[list->Length] = element;
	list->Length += 1;
}

//Insert an element into an array list.
void ArrayList_Insert(ArrayList* list, void* element, UINTN index)
{
	ArrayList_Add(list, element);

	for (UINTN i = index; i < list->Length - 1; i++)
	{
		((void**)list->Data.Start)[i + 1] = ((void**)list->Data.Start)[i];
	}

	((void**)list->Data.Start)[index] = element;
}

//Remove an element at the specified index from an array list.
void* ArrayList_RemoveAt(ArrayList* list, UINTN index)
{
	if (index < 0 || index >= list->Length) return 0;

	void* result = ((void**)list->Data.Start)[index];

	for (UINTN i = index; i < list->Length - 1; i++)
	{
		((void**)list->Data.Start)[i] = ((void**)list->Data.Start)[i + 1];
	}

	list->Length -= 1;
	
	if (list->Capacity > 1024 && list->Length < (list->Capacity / 2))
	{
		list->Data = realloc(&list->Data, list->Capacity / 2);
	}

	return result;
}

//Remove an element from an array list.
BOOLEAN ArrayList_Remove(ArrayList* list, void* element)
{
	BOOLEAN result = FALSE;

	for (UINTN i = 0; i < list->Length; i++)
	{
		void* current = ((void**)list->Data.Start)[i];

		if (current == element)
		{
			result = TRUE;

			for (UINTN j = i; j < list->Length - 1; j++)
			{
				((void**)list->Data.Start)[j] = ((void**)list->Data.Start)[j + 1];
			}

			list->Length -= 1;
			break;
		}
	}

	if (result && list->Capacity > 1024 && (list->Length < (list->Capacity / 2)))
	{
		list->Data = realloc(&list->Data, list->Capacity / 2);
	}

	return result;
}