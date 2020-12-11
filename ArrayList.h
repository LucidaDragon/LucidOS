#pragma once
#include "stdlib.h"

//Object that represents an array list.
typedef struct
{
	MemBlock Data;
	UINTN Length;
	UINTN Capacity;
} ArrayList;

ArrayList New_ArrayList()
{
	ArrayList list;
	list.Length = 0;
	list.Capacity = 1024;
	list.Data = malloc(sizeof(void*) * list.Capacity);
	return list;
}

void Dispose_ArrayList(ArrayList* list)
{
	list->Length = 0;
	list->Capacity = 0;
	free(&list->Data);
}

void* ArrayList_Get(ArrayList list, UINTN index)
{
	return ((void**)list.Data.Start)[index];
}

void ArrayList_Set(ArrayList* list, UINTN index, void* element)
{
	((void**)list->Data.Start)[index] = element;
}

void ArrayList_Add(ArrayList* list, void* element)
{
	if (list->Length >= list->Capacity)
	{
		list->Data = realloc(&list->Data, list->Capacity * 2);
	}

	((void**)list->Data.Start)[list->Length] = element;
	list->Length += 1;
}

void ArrayList_Insert(ArrayList* list, void* element, UINTN index)
{
	ArrayList_Add(list, element);

	for (UINTN i = index; i < list->Length - 1; i++)
	{
		((void**)list->Data.Start)[i + 1] = ((void**)list->Data.Start)[i];
	}

	((void**)list->Data.Start)[index] = element;
}

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

	if (result && (list->Length < (list->Capacity / 2)))
	{
		list->Data = realloc(&list->Data, list->Capacity / 2);
	}

	return result;
}