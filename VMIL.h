#pragma once
#include "VM.h"
#include "File.h"

EFI_STATUS LoadVMIL(EFI_FILE* source, UINTN id, VM* result)
{
	EFI_STATUS status;

	UINTN size;

	UINT64 length;
	size = sizeof(length);
	status = source->Read(source, &size, &length);
	if (EFI_ERROR(status)) return status;
	else if (size != sizeof(length)) return EFI_END_OF_FILE;

	Print(L"Length: %d\n", length);

	MemBlock mem = malloc(length);
	if (mem.Size == 0) return EFI_OUT_OF_RESOURCES;

	UINT64 vars;
	size = sizeof(vars);
	status = source->Read(source, &size, &vars);
	if (EFI_ERROR(status))
	{
		free(&mem);
		return status;
	}
	else if (size != sizeof(vars))
	{
		free(&mem);
		return EFI_END_OF_FILE;
	}
	else if ((vars * sizeof(UINT64)) > length)
	{
		free(&mem);
		return EFI_BAD_BUFFER_SIZE;
	}

	Print(L"Vars: %d\n", vars);

	UINT64 error;
	size = sizeof(error);
	status = source->Read(source, &size, &error);
	if (EFI_ERROR(status))
	{
		free(&mem);
		return status;
	}
	else if (size != sizeof(error))
	{
		free(&mem);
		return EFI_END_OF_FILE;
	}
	else if ((vars * sizeof(UINT64)) + error >= length)
	{
		free(&mem);
		return EFI_BAD_BUFFER_SIZE;
	}

	Print(L"Error: %d\n", error);

	UINT8* entry = (UINT8*)mem.Start + (vars * sizeof(UINT64));

	if (entry < (UINT8*)mem.Start || ((UINT8*)mem.Start + length) <= entry)
	{
		free(&mem);
		return EFI_BAD_BUFFER_SIZE;
	}

	*result = New_VM(mem, id, 1, (UINT64*)mem.Start, vars, entry, entry + error);

	UINT8* target = entry;
	while (1)
	{
		if (target < (UINT8*)mem.Start || ((UINT8*)mem.Start + length) <= target)
		{
			free(&mem);
			return EFI_BAD_BUFFER_SIZE;
		}

		UINT8 value;
		size = sizeof(value);
		status = source->Read(source, &size, &value);
		if (EFI_ERROR(status) || size != sizeof(value)) break;

		Print(L"%x ", value);

		*target = value;

		target++;
	}

	return EFI_SUCCESS;
}