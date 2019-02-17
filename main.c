//
// EFI Library Includes
//

#include <efi.h>
#include <efilib.h>





//
// LucidOS Standard Structures
//

//Object that has a width and height.
typedef struct {
	UINTN Width;
	UINTN Height;
} SIZE;

//Object that defines a screen size and mode.
typedef struct {
	SIZE Size;
	UINTN Mode;
} SCREEN;

//Object that contains all of the environment parameters.
typedef struct {
	EFI_HANDLE *Image;
	EFI_SYSTEM_TABLE *Table;
	SCREEN Screen;
} ENVIRONMENT;

//Object that has a x and y position.
typedef struct {
	UINTN X;
	UINTN Y;
} POINT;

//Object that has a x position, y position, width, and height.
typedef struct {
	UINTN X;
	UINTN Y;
	UINTN Width;
	UINTN Height;
} RECT;

//Object that represents a block of memory.
typedef struct {
	void *Start;
	UINTN Size;
} MEMBLOCK;





//
// Library Ports for EFI.
//

//MATH: Returns the smaller of two values.
UINTN min(UINTN a, UINTN b) {
	if (a < b) { return a; } else { return b; }
}

//MATH: Returns the larger of two values.
UINTN max(UINTN a, UINTN b) {
	if (a > b) { return a; } else { return b; }
}

//STDLIB: Allocates a block of memory with the specified size.
MEMBLOCK malloc(UINTN size) {
	MEMBLOCK result;
	EFI_STATUS status;
	void *handle;
	status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size + sizeof(UINTN), &handle);
	if (status == EFI_OUT_OF_RESOURCES) {
		result.Start = NULL;
		result.Size = 0;
	}
	else if (status == EFI_INVALID_PARAMETER) {
		result.Start = NULL;
		result.Size = 0;
	}
	else {
		result.Start = handle;
		result.Size = size;
	}
	return result;
}

//STDLIB: Allocates a block of memory for the specified number of items of the specified size.
MEMBLOCK calloc(UINTN num, UINTN size) {
	MEMBLOCK result = malloc(num * size);
	uefi_call_wrapper(BS->SetMem, 3, result.Start, result.Size, 0);
	return result;
}

//STDLIB: Deallocates the specified block of memory.
void free(MEMBLOCK *block) {
	uefi_call_wrapper(BS->FreePool, 1, block->Start);
	block->Start = NULL;
	block->Size = 0;
}

//STDLIB: Resizes the specified block of memory.
MEMBLOCK realloc(MEMBLOCK *block, UINTN size) {
	MEMBLOCK result = malloc(size);
	uefi_call_wrapper(BS->CopyMem, 3, result.Start, block->Start, size);
	free(block);
	return result;
}

//STDLIB: Creates a copy of the specified block of memory.
MEMBLOCK memcopy(MEMBLOCK *block) {
	MEMBLOCK result = malloc(block->Size);
	uefi_call_wrapper(BS->CopyMem, 3, result.Start, block->Start, block->Size);
	return result;
}





//
// LucidOS Standard Functions
//

//
// #Basic IO Functions#
//

//Clears the screen and returns the caret to the top left corner.
void ClearScreen(ENVIRONMENT *e) {
	e->Table->ConOut->ClearScreen(e->Table->ConOut);
}

//Waits for any key to be pressed.
void WaitForKey(ENVIRONMENT *e) {
	UINTN event;
	e->Table->ConIn->Reset(e->Table->ConIn, FALSE);
	e->Table->BootServices->WaitForEvent(1, &e->Table->ConIn->WaitForKey, &event);
}

//Prints the current system time.
void PrintTime(ENVIRONMENT *e, BOOLEAN newLine) {
	EFI_TIME time;
	EFI_TIME_CAPABILITIES caps;
	CHAR16 buf;
	e->Table->RuntimeServices->GetTime(&time, &caps);
	TimeToString(&buf, &time);
	Print(&buf);

	if (newLine)
	{
		Print(L"\n");
	}
}

//Sets the cursor position.
void SetPos(ENVIRONMENT *e, UINTN x, UINTN y) {
	e->Table->ConOut->SetCursorPosition(e->Table->ConOut, x, y);
}

//
// #Graphics Functions#
//

//Sets the screen color to print in.
void SetColor(ENVIRONMENT *e, UINT8 forecolor, UINT8 backcolor) {
	e->Table->ConOut->SetAttribute(e->Table->ConOut, EFI_TEXT_ATTR(forecolor, backcolor));
}

//Prints a full block character in the specified color.
void PrintColor(ENVIRONMENT *e, UINT8 color) {
	SetColor(e, color, EFI_BLACK);
	Print(L"█");
}

//Draw a horizontal bar that fills the screen.
void DrawBar(ENVIRONMENT *e, UINT8 color) {
	for (UINTN i = 0; i < e->Screen.Size.Width; i++) PrintColor(e, color);
}

//Fills the screen with a color.
void Clear(ENVIRONMENT *e, UINT8 color) {
	SetPos(e, 0, 0);
	for (UINTN i = 0; i < e->Screen.Size.Height; i++) {
		DrawBar(e, color);
	}
	SetPos(e, 0, 0);
}

//Fills a rectangle with the specified color.
void FillRect(ENVIRONMENT *e, RECT *r, UINT8 color) {
	for (UINTN y = 0; y < r->Height; y++) {
		for (UINTN x = 0; x < r->Width; x++) {
			SetPos(e, x + r->X, y + r->Y);
			PrintColor(e, color);
		}
	}
}

//Draws a rectangle with the specified color.
void DrawRect(ENVIRONMENT *e, RECT *r, UINT8 color) {
	for (UINTN i = 0; i < r->Width; i++) {
		SetPos(e, i + r->X, r->Y);
		PrintColor(e, color);
		SetPos(e, i + r->X, r->Y + r->Height);
		PrintColor(e, color);
	}
	for (UINTN i = 0; i < r->Height; i++) {
		SetPos(e, r->X, i + r->Y);
		PrintColor(e, color);
		SetPos(e, r->X + r->Width, i + r->Y);
		PrintColor(e, color);
	}
}

//
// #Environment Runtime Functions#
//

//Enters a new OS environment space.
void Environment(ENVIRONMENT *e) {
	ClearScreen(e);

	/*DrawBar(e, EFI_RED);
	DrawBar(e, EFI_YELLOW);
	DrawBar(e, EFI_GREEN);
	DrawBar(e, EFI_BLUE);
	DrawBar(e, EFI_MAGENTA);
	Print(L"\n");
	SetColor(e, EFI_WHITE, EFI_BLACK);

	PrintTime(e, TRUE);

	PrintColor(e, EFI_RED);
	PrintColor(e, EFI_YELLOW);
	PrintColor(e, EFI_GREEN);
	PrintColor(e, EFI_BLUE);
	PrintColor(e, EFI_MAGENTA);
	Print(L"\n\n");
	for (UINT8 b = 0; b < 16; b++) {
		for (UINT8 f = 0; f < 16; f++) {
			SetColor(e, f, b);
			Print(L"A");
		}
		Print(L"\n");
	}
	Print(L"\n\n");
	SetColor(e, EFI_WHITE, EFI_BLACK);

	Print(L"%NPress any key to continue...");
	WaitForKey(e);*/

	/*Clear(e, EFI_WHITE);
	RECT r; r.X = 3; r.Y = 3; r.Width = 10; r.Height = 5;
	FillRect(e, &r, EFI_RED);
	DrawRect(e, &r, EFI_BLUE);
	SetColor(e, EFI_BLACK, EFI_WHITE);*/

	MEMBLOCK m = malloc(1000);
	Print(L"Created block at %u\n", m.Start);
	Print(L"Block size is %u\n", m.Size);
	MEMBLOCK n = memcopy(&m);
	Print(L"Copied a block to %u\n", n.Start);
	Print(L"Block size is %u\n", n.Size);
	MEMBLOCK o = realloc(&m, 2000);
	Print(L"Reallocated block %u to %u\n", m.Start, o.Start);
	Print(L"Block size is now: %u\n", o.Size);
	free(&m);
	free(&n);
	free(&o);
	Print(L"All memory is now free.");

	Print(L"\n\nPress any key to exit.\n");
	WaitForKey(e);
}

//
// #Environment Initialization Functions#
//

//Configures the display to it's largest supported text mode size and returns the screen definition.
SCREEN ConfigureDisplay(EFI_SYSTEM_TABLE *table) {
	UINTN actualW = 0;
	UINTN actualH = 0;
	UINTN highestMode = 0;
	for (UINTN i = 0; i < 100; i++) {
		UINTN tempW = 0;
		UINTN tempH = 0;
		if (table->ConOut->QueryMode(table->ConOut, i, &tempW, &tempH) == EFI_SUCCESS) {
			actualW = tempW;
			actualH = tempH;
			highestMode = i;
		}
	}
	table->ConOut->SetMode(table->ConOut, highestMode);
	SIZE s;
	s.Width = actualW;
	s.Height = actualH;
	SCREEN scr;
	scr.Size = s;
	scr.Mode = highestMode;
	return scr;
}

//Initialize the environment using the system table and image handle.
void InitEnvironment(EFI_HANDLE *image, EFI_SYSTEM_TABLE *table) {
	table->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	ENVIRONMENT e;
	e.Image = image;
	e.Table = table;
	e.Screen = ConfigureDisplay(table);

	Environment(&e);
}

//
// #EFI Initialization Functions#
//

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
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
	
#if defined(_DEBUG)
	// If running in debug mode, use the EFI shut down call to close QEMU
	SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
#endif

	return EFI_SUCCESS;
}