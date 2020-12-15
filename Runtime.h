#pragma once
#include "VMIL.h"
#include "File.h"

typedef struct
{
	ArrayList Tasks;
	UINTN NextId;
} Runtime;

Runtime New_Runtime()
{
	Runtime result;
	result.Tasks = New_ArrayList();
	result.NextId = 0;
	return result;
}

EFI_STATUS Runtime_Launch(Runtime* rt, EFI_FILE* directory, EFI_FILE_INFO* file)
{
	VM* vm = (VM*)malloc(sizeof(VM)).Start;
	
	EFI_STATUS status = VMIL_Load(OpenEntry(directory, file), rt->NextId++, vm);

	if (EFI_ERROR(status))
	{
		freeany(vm);
		return status;
	}

	ArrayList_Add(&rt->Tasks, vm);

	return EFI_SUCCESS;
}

void Runtime_Execute(Runtime* rt)
{
	for (UINTN i = 0; i < rt->Tasks.Length; i++)
	{
		VM* task = (VM*)ArrayList_Get(rt->Tasks, i);

		if (task->Status == Active)
		{
			for (UINTN j = 0; j < task->Priority && task->Status == Active; j++)
			{
				VM_Execute(task);
			}
		}
		
		if (task->Status == Finished)
		{
			ArrayList_RemoveAt(&rt->Tasks, i);
			i--;
		}
	}
}