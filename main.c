/*
 * UEFI:SIMPLE - UEFI development made easy
 * Copyright © 2014-2018 Pete Batard <pete@akeo.ie> - Public Domain
 * See COPYING for the full licensing terms.
 */
#include <efi.h>
#include <efilib.h>

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




 //Clears the screen and returns the caret to the top left corner.
void ClearScreen(EFI_SYSTEM_TABLE *table) {
	table->ConOut->ClearScreen(table->ConOut);
}

//Waits for any key to be pressed.
void WaitForKey(EFI_SYSTEM_TABLE *table) {
	UINTN event;
	table->ConIn->Reset(table->ConIn, FALSE);
	table->BootServices->WaitForEvent(1, &table->ConIn->WaitForKey, &event);
}

//Prints the current system time.
void PrintTime(EFI_SYSTEM_TABLE *table, BOOLEAN newLine) {
	EFI_TIME time;
	EFI_TIME_CAPABILITIES caps;
	CHAR16 buf;
	table->RuntimeServices->GetTime(&time, &caps);
	TimeToString(&buf, &time);
	Print(&buf);

	if (newLine)
	{
		Print(L"\n");
	}
}

//Read a line of text from the user.
void ReadLine(EFI_SYSTEM_TABLE *table, CHAR16 *buffer) {
	UINTN event;
	BOOLEAN exit = FALSE;
	EFI_INPUT_KEY last;

	table->ConIn->Reset(table->ConIn, FALSE);
	while (exit == FALSE)
	{
		table->BootServices->WaitForEvent(1, &table->ConIn->WaitForKey, &event);
		table->ConIn->ReadKeyStroke(table->ConIn, &last);
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
void SetPos(EFI_SYSTEM_TABLE *table, UINTN x, UINTN y) {
	table->ConOut->SetCursorPosition(table->ConOut, x, y);
}

//Sets the screen color to print in.
void SetColor(EFI_SYSTEM_TABLE *table, UINT8 forecolor, UINT8 backcolor) {
	table->ConOut->SetAttribute(table->ConOut, EFI_TEXT_ATTR(forecolor, backcolor));
}

//Prints a full block character in the specified color.
void PrintColor(EFI_SYSTEM_TABLE *table, UINT8 color) {
	SetColor(table, color, EFI_BLACK);
	Print(L"█");
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

//Draw a horizontal bar that fills the screen.
void DrawBar(ENVIRONMENT *e, UINT8 color) {
	for (UINTN i = 0; i < e->Screen.Size.Width; i++) PrintColor(e->Table, color);
}

//Enters a new OS environment space.
void Environment(ENVIRONMENT *e) {
	ClearScreen(e->Table);

	DrawBar(e, EFI_RED);
	DrawBar(e, EFI_YELLOW);
	DrawBar(e, EFI_GREEN);
	DrawBar(e, EFI_BLUE);
	DrawBar(e, EFI_MAGENTA);
	Print(L"\n");
	SetColor(e->Table, EFI_WHITE, EFI_BLACK);

	PrintTime(e->Table, TRUE);

	PrintColor(e->Table, EFI_RED);
	PrintColor(e->Table, EFI_YELLOW);
	PrintColor(e->Table, EFI_GREEN);
	PrintColor(e->Table, EFI_BLUE);
	PrintColor(e->Table, EFI_MAGENTA);
	Print(L"\n\n");
	for (UINT8 b = 0; b < 16; b++) {
		for (UINT8 f = 0; f < 16; f++) {
			SetColor(e->Table, f, b);
			Print(L"A");
		}
		Print(L"\n");
	}
	Print(L"\n\n");
	SetColor(e->Table, EFI_WHITE, EFI_BLACK);
	CHAR16 *buffer = L"Test";
	ReadLine(e->Table, buffer);

	Print(L"%NPress any key to continue...");
	WaitForKey(e->Table);

	Print(L"\n%EPress any key to exit.%N\n");
	WaitForKey(e->Table);
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