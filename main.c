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
	EFI_HANDLE* Image;
	EFI_SYSTEM_TABLE* Table;
	SCREEN Screen;
} ENVIRONMENT;

//Object that has a x position, y position, width, and height.
typedef struct {
	UINTN X;
	UINTN Y;
	UINTN Width;
	UINTN Height;
} RECT;





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

//STDLIB Ext: Convert a memory pointer excluding size to a memory pointer including size.
void *msizei(void *ptr) {
	UINTN *h = ptr;
	return &h[-1];
}

//STDLIB Ext: Convert a memory pointer including size to a memory pointer excluding size.
void *msizee(void *ptr) {
	UINTN *h = ptr;
	return &h[1];
}

//STDLIB: Allocates the requested memory and returns a pointer to it.
void *malloc(UINTN poolSize) {
	EFI_STATUS status;
	void *handle;
	status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, poolSize + sizeof(UINTN), &handle);
	if (status == EFI_OUT_OF_RESOURCES) {
		return NULL;
	}
	else if (status == EFI_INVALID_PARAMETER) {
		return NULL;
	}
	else {
		UINTN *h = handle;
		h = poolSize;
		return &h[1];
	}
}

//STDLIB: Allocates the requested memory and returns a pointer to it.
void *calloc(UINTN num, UINTN size) {
	void *handle = NULL;
	UINTN numSize = num * size;
	if (numSize != 0) {
		handle = malloc(numSize);
		if (handle != NULL) {
			(VOID)ZeroMem(handle, numSize);
		}
	}
	return handle;
}

//STDLIB: Deallocates the memory previously allocated by a call to malloc or realloc.
void free(void *ptr) {
	uefi_call_wrapper(BS->FreePool, 1, (UINTN*)msizei(ptr));
}

//STDLIB Ext: Gets the size of memory at a pointer.
UINTN msize(void *ptr) {
	return ((UINTN*)ptr)[-1];
}

//STDLIB Ext: Gets the number of elements at a pointer.
UINTN csize(void *ptr, UINTN size) {
	return msize(ptr) / size;
}

//STRING: Copies bytes from source pointer to destination pointer.
void memcpy(void *dest, const void *source, UINTN count) {
	uefi_call_wrapper(BS->CopyMem, 3, dest, source, count);
}

//STDLIB: Reallocates the requested memory and returns a pointer to it. The old pointer is invalidated.
void *realloc(void *ptr, size_t newSize) {
	void *dest = malloc(newSize);
	memcpy(dest, ptr, min(msize(ptr), newSize));
	free(ptr);
	UINTN *h = dest;
	h = newSize;
	return &h[1];
}





//
// LucidOS Standard Functions
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

//Read a line of text from the user.
void ReadLine(ENVIRONMENT *e, CHAR16 *buffer) {
	UINTN event;
	BOOLEAN exit = FALSE;
	EFI_INPUT_KEY last;

	e->Table->ConIn->Reset(e->Table->ConIn, FALSE);
	while (exit == FALSE)
	{
		e->Table->BootServices->WaitForEvent(1, &e->Table->ConIn->WaitForKey, &event);
		e->Table->ConIn->ReadKeyStroke(e->Table->ConIn, &last);
		Print(&last.UnicodeChar);
		if (last.UnicodeChar == L'\n') {
			exit = TRUE;
		}
		else {
			StrCat(buffer, &last.UnicodeChar);
		}
	}
}

//Sets the cursor position.
void SetPos(ENVIRONMENT *e, UINTN x, UINTN y) {
	e->Table->ConOut->SetCursorPosition(e->Table->ConOut, x, y);
}

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

	Print(L"\n\nPress any key to exit.\n");
	WaitForKey(e);
}

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