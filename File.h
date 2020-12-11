#pragma once
#include <efi.h>
#include "ArrayList.h"

//Get all entries in a directory.
ArrayList GetEntries(EFI_FILE* directory)
{
	ArrayList result = New_ArrayList();
	MemBlock buffer;
	UINTN size;
	EFI_STATUS status;

	UINTN i = 0;

	while (1)
	{
		size = 1024;
		buffer = zmalloc(size);
		status = directory->Read(directory, &size, buffer.Start);

		if (EFI_ERROR(status) || size == 0)
		{
			free(&buffer);
			break;
		}

		EFI_FILE_INFO* file = (EFI_FILE_INFO*)buffer.Start;

		if (file->FileName[0] == L'.')
		{
			free(&buffer);
			continue;
		}

		ArrayList_Add(&result, buffer.Start);
	}

	return result;
}

//Get all entries with a specified attribute.
ArrayList GetEntriesWithType(EFI_FILE* directory, UINTN type, int invert)
{
	ArrayList result = New_ArrayList();

	ArrayList all = GetEntries(directory);

	for (UINTN i = 0; i < all.Length; i++)
	{
		void* elem = ArrayList_Get(all, i);

		UINTN isType = ((EFI_FILE_INFO*)elem)->Attribute & type;

		if (invert ? !isType : isType)
		{
			ArrayList_Add(&result, elem);
		}
		else
		{
			freeany(elem);
		}
	}

	Dispose_ArrayList(&all);

	return result;
}

//Get all files in a directory.
ArrayList GetFiles(EFI_FILE* directory)
{
	return GetEntriesWithType(directory, EFI_FILE_DIRECTORY, 1);
}

//Get all subdirectories in a directory.
ArrayList GetDirectories(EFI_FILE* directory)
{
	return GetEntriesWithType(directory, EFI_FILE_DIRECTORY, 0);
}

//Open a file in a directory.
EFI_FILE* OpenEntry(EFI_FILE* directory, EFI_FILE_INFO* info)
{
	EFI_FILE* child;
	EFI_STATUS status = directory->Open(directory, &child, info->FileName, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

	if (EFI_ERROR(status)) return 0;

	child->SetPosition(child, 0);
	return child;
}