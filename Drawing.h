#pragma once
#include "Console.h"

//Object that has a x and y position.
typedef struct
{
	UINTN X;
	UINTN Y;
} Point;

//Object that has a x position, y position, width, and height.
typedef struct
{
	UINTN X;
	UINTN Y;
	UINTN Width;
	UINTN Height;
} Rect;

//Sets the screen color to print in.
void SetColor(Environment* e, UINT8 forecolor, UINT8 backcolor)
{
	e->Table->ConOut->SetAttribute(e->Table->ConOut, EFI_TEXT_ATTR(forecolor, backcolor));
}

//Prints a full block character in the specified color.
void PrintColor(Environment* e, UINT8 color)
{
	SetColor(e, color, EFI_BLACK);
	Print(L"█");
}

//Draw a horizontal bar that fills the screen.
void DrawBar(Environment* e, UINT8 color)
{
	for (UINTN i = 0; i < e->Screen.Size.Width; i++) PrintColor(e, color);
}

//Fills the screen with a color.
void Clear(Environment* e, UINT8 color)
{
	SetPos(e, 0, 0);
	for (UINTN i = 0; i < e->Screen.Size.Height; i++)
	{
		DrawBar(e, color);
	}
	SetPos(e, 0, 0);
}

//Fills a rectangle with the specified color.
void FillRect(Environment* e, Rect* r, UINT8 color)
{
	for (UINTN y = 0; y < r->Height; y++)
	{
		for (UINTN x = 0; x < r->Width; x++)
		{
			SetPos(e, x + r->X, y + r->Y);
			PrintColor(e, color);
		}
	}
}

//Draws a rectangle with the specified color.
void DrawRect(Environment* e, Rect* r, UINT8 color)
{
	for (UINTN i = 0; i < r->Width; i++)
	{
		SetPos(e, i + r->X, r->Y);
		PrintColor(e, color);
		SetPos(e, i + r->X, r->Y + r->Height);
		PrintColor(e, color);
	}
	for (UINTN i = 0; i <= r->Height; i++)
	{
		SetPos(e, r->X, i + r->Y);
		PrintColor(e, color);
		SetPos(e, r->X + r->Width, i + r->Y);
		PrintColor(e, color);
	}
}