#pragma once
#include <efi.h>
#include <efilib.h>

//Object that represents a block of memory.
typedef struct
{
	void* Start;
	UINTN Size;
} MemBlock;

//Allocates a block of memory with the specified size.
MemBlock malloc(UINTN size)
{
	MemBlock result;
	result.Start = NULL;
	result.Size = 0;

	if (size == 0) return result;

	EFI_STATUS status;
	void* handle;
	status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size, &handle);
	if (status == EFI_OUT_OF_RESOURCES)
	{
		result.Start = NULL;
		result.Size = 0;
	}
	else if (status == EFI_INVALID_PARAMETER)
	{
		result.Start = NULL;
		result.Size = 0;
	}
	else
	{
		result.Start = handle;
		result.Size = size;
	}
	return result;
}

//Allocates a block of memory for the specified number of items of the specified size.
MemBlock calloc(UINTN num, UINTN size)
{
	MemBlock result = malloc(num * size);
	uefi_call_wrapper(BS->SetMem, 3, result.Start, result.Size, 0);
	return result;
}

//Allocates a block of memory with the specified size and zeros it out.
MemBlock zmalloc(UINTN size)
{
	MemBlock result = malloc(size);

	for (UINTN i = 0; i < result.Size; i++)
	{
		((UINT8*)result.Start)[i] = 0;
	}

	return result;
}

//Deallocates the specified block of memory.
void free(MemBlock* block)
{
	uefi_call_wrapper(BS->FreePool, 1, block->Start);
	block->Start = NULL;
	block->Size = 0;
}

//Deallocates the memory at the specified pointer.
void freeany(void* ptr)
{
	uefi_call_wrapper(BS->FreePool, 1, ptr);
}

void memcopy(MemBlock dest, MemBlock src, UINTN size)
{
	for (UINTN i = 0; i < size; i++)
	{
		((UINT8*)dest.Start)[i] = ((UINT8*)src.Start)[i];
	}
}

//Resizes the specified block of memory.
MemBlock realloc(MemBlock* block, UINTN size)
{
	MemBlock result = malloc(size);
	memcopy(result, (*block), block->Size < size ? block->Size : size);
	free(block);
	return result;
}

//Creates a copy of the specified block of memory.
MemBlock memdup(MemBlock* block)
{
	MemBlock result = malloc(block->Size);
	memcopy(result, (*block), block->Size);
	return result;
}