#pragma once
#include <efi.h>

//Object that has a width and height.
typedef struct
{
	UINTN Width;
	UINTN Height;
} Size;

//Object that defines a screen size and mode.
typedef struct
{
	Size Size;
	UINTN Mode;
} Screen;

//Object that contains all of the environment parameters.
typedef struct
{
	EFI_HANDLE* Image;
	EFI_SYSTEM_TABLE* Table;
	EFI_FILE* RootDirectory;
	Screen Screen;
} Environment;

//Clears the screen and returns the caret to the top left corner.
void ClearScreen(Environment* e)
{
	e->Table->ConOut->ClearScreen(e->Table->ConOut);
}

//Waits for any key to be pressed.
void WaitForKey(Environment* e)
{
	UINTN event;
	e->Table->ConIn->Reset(e->Table->ConIn, FALSE);
	e->Table->BootServices->WaitForEvent(1, &e->Table->ConIn->WaitForKey, &event);
}

//Waits for the specified key to be pressed.
void WaitForSpecificKey(Environment* e, CHAR16 key)
{
	UINTN event;
	EFI_INPUT_KEY pressed;
	e->Table->ConIn->Reset(e->Table->ConIn, FALSE);
	while (TRUE)
	{
		e->Table->BootServices->WaitForEvent(1, &e->Table->ConIn->WaitForKey, &event);
		e->Table->ConIn->ReadKeyStroke(e->Table->ConIn, &pressed);
		if (pressed.UnicodeChar == key)
		{
			return;
		}
	}
}

//Sets the cursor position.
void SetPos(Environment* e, UINTN x, UINTN y)
{
	e->Table->ConOut->SetCursorPosition(e->Table->ConOut, x, y);
}