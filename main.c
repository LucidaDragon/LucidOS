#pragma once
#include <efi.h>
#include <efilib.h>
#include "stdlib.h"
#include "Drawing.h"
#include "ArrayList.h"
#include "File.h"

//
// #Environment Runtime Functions#
//

void PrintEntry(EFI_FILE* directory, EFI_FILE_INFO* entry, int indentation)
{
	for (int i = 0; i < indentation; i++) Print(L"  ");
	Print(L"%s %s\r\n", (entry->Attribute & EFI_FILE_DIRECTORY) ? L"DIR" : L"FIL", entry->FileName);

	if (entry->Attribute & EFI_FILE_DIRECTORY)
	{
		EFI_FILE* self = OpenEntry(directory, entry);

		ArrayList entries = GetEntries(self);

		for (UINTN i = 0; i < entries.Length; i++)
		{
			PrintEntry(self, (EFI_FILE_INFO*)ArrayList_Get(entries, i), indentation + 1);
		}
	}
}

#include "Runtime.h"
#include "TextEditor.h"

//Enters a new OS environment.
void EnterEnvironment(Environment* e)
{
	TextEditor_Run(e);

	ArrayList entries = GetEntries(e->RootDirectory);
	EFI_FILE_INFO* kernel = 0;

	for (UINTN i = 0; i < entries.Length; i++)
	{
		EFI_FILE_INFO* entry = (EFI_FILE_INFO*)ArrayList_Get(entries, i);

		if (StrLen(entry->FileName) == 10 &&
			entry->FileName[0] == 'k' &&
			entry->FileName[1] == 'e' &&
			entry->FileName[2] == 'r' &&
			entry->FileName[3] == 'n' &&
			entry->FileName[4] == 'e' &&
			entry->FileName[5] == 'l' &&
			entry->FileName[6] == '.' &&
			entry->FileName[7] == 'b' &&
			entry->FileName[8] == 'i' &&
			entry->FileName[9] == 'n')
		{
			kernel = entry;
			break;
		}
	}

	if (kernel == 0)
	{
		Print(L"Kernel was not found.\n");
		Print(L"Press any key to continue...");
		WaitForKey(e);
		return;
	}

	Runtime rt = New_Runtime();

	EFI_STATUS status = Runtime_Launch(&rt, e->RootDirectory, kernel);

	Print(L"\nPress any key to continue...\n");
	WaitForKey(e);

	if (EFI_ERROR(status))
	{
		Print(L"Error occured while launching kernel. %r\n", status);
		Print(L"Press any key to continue...");
		WaitForKey(e);
		return;
	}

	while (rt.Tasks.Length > 0)
	{
		Runtime_Execute(&rt);
	}
}

//
// #Environment Initialization Functions#
//

//Configures the display to it's largest supported text mode size and returns the screen definition.
Screen ConfigureDisplay(EFI_SYSTEM_TABLE* table)
{
	UINTN actualW = 0;
	UINTN actualH = 0;
	UINTN highestMode = 0;
	for (UINTN i = 0; i < 100; i++)
	{
		UINTN tempW = 0;
		UINTN tempH = 0;
		if (table->ConOut->QueryMode(table->ConOut, i, &tempW, &tempH) == EFI_SUCCESS)
		{
			actualW = tempW;
			actualH = tempH;
			highestMode = i;
		}
	}
	table->ConOut->SetMode(table->ConOut, highestMode);
	Size s;
	s.Width = actualW;
	s.Height = actualH;
	Screen scr;
	scr.Size = s;
	scr.Mode = highestMode;
	return scr;
}

//Initialize the environment using the system table and image handle.
void InitEnvironment(EFI_HANDLE* image, EFI_SYSTEM_TABLE* table)
{
	table->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	Environment* e = malloc(sizeof(Environment)).Start;
	e->Image = image;
	e->Table = table;
	e->Screen = ConfigureDisplay(table);

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	table->BootServices->HandleProtocol(image, &gEfiLoadedImageProtocolGuid, &LoadedImage);
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
	table->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, &FileSystem);
	FileSystem->OpenVolume(FileSystem, &e->RootDirectory);

	EnterEnvironment(e);
}

//
// #EFI Initialization Functions#
//

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif

	/*
	 * In addition to the standard %-based flags, Print() supports the following:
	 *   %N       Set output attribute to normal
	 *   %H       Set output attribute to highlight
	 *   %E       Set output attribute to error
	 *   %B       Set output attribute to blue color
	 *   %V       Set output attribute to green color
	 *   %r       Human readable version of a status code
	 */
	Print(L"%HLoaded%N");

	InitEnvironment(ImageHandle, SystemTable);

	SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}