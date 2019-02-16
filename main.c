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

//Enters a new OS environment space.
void Environment(ENVIRONMENT *e) {
	ClearScreen(e);

	DrawBar(e, EFI_RED);
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
	CHAR16 *buffer = L"Test";
	ReadLine(e, buffer);

	Print(L"%NPress any key to continue...");
	WaitForKey(e);

	Print(L"\n%EPress any key to exit.%N\n");
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