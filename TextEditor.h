#pragma once
#include "stdlib.h"
#include "Console.h"

typedef struct TextEditorBlock
{
	struct TextEditorBlock* Previous;
	struct TextEditorBlock* Next;
	INTN Width;
	INTN Height;
	CHAR16 Buffer[0];
} TextEditorBlock;

typedef struct
{
	Environment* Environment;
	INTN CursorX;
	INTN CursorY;
	TextEditorBlock* Page;
} TextEditor;

TextEditorBlock* New_TextEditorBlock(Environment* e)
{
	Size screenSize = e->Screen.Size;

	TextEditorBlock* result = (TextEditorBlock*)zmalloc(sizeof(TextEditorBlock) + ((screenSize.Width * screenSize.Height) * sizeof(CHAR16))).Start;

	if (result != 0)
	{
		result->Width = (INTN)screenSize.Width;
		result->Height = (INTN)(screenSize.Height - 1);
	}

	return result;
}

void Dispose_TextEditorBlock(TextEditorBlock* block)
{
	freeany(block);
}

INT64 TextEditorBlock_DistanceToStart(TextEditorBlock* block)
{
	INT64 result = 0;

	while (block->Previous != 0)
	{
		block = block->Previous;
		result++;
	}

	return result;
}

INT64 TextEditorBlock_DistanceToEnd(TextEditorBlock* block)
{
	INT64 result = 0;

	while (block->Next != 0)
	{
		block = block->Next;
		result++;
	}

	return result;
}

CHAR16* TextEditorBlock_ToString(TextEditorBlock* block)
{
	INT64* lineLengths = (UINT64*)malloc(block->Height).Start;

	for (INT64 i = 0; i < block->Height; i++)
	{
		int success = 0;

		for (INT64 j = 0; j < block->Width; j++)
		{
			if (block->Buffer[j + (i * block->Width)] == 0)
			{
				success = 1;
				lineLengths[i] = j;
				break;
			}
		}

		if (!success) lineLengths[i] = block->Width;
	}

	INT64 bufferSize = 0;

	for (INT64 i = 0; i < block->Height; i++)
	{
		bufferSize += lineLengths[i] + 1;
	}

	CHAR16* buffer = malloc(bufferSize * sizeof(CHAR16)).Start;
	INT64 position = 0;

	for (INT64 i = 0; i < block->Height; i++)
	{
		INT64 length = lineLengths[i];

		if (length > 0)
		{
			StrnCpy(&buffer[position], &block->Buffer[i * block->Width], length);
			position += length;
		}

		if (i + 1 < block->Height)
		{
			buffer[position] = '\n';
			position++;
		}
		else
		{
			buffer[position] = 0;
			break;
		}
	}

	freeany(lineLengths);

	return buffer;
}

TextEditorBlock* TextEditorBlock_AddPage(Environment* e, TextEditorBlock* block)
{
	while (block->Next != 0)
	{
		block = block->Next;
	}

	block->Next = New_TextEditorBlock(e);
	if (block->Next != 0) block->Next->Previous = block;
	return block->Next;
}

void TextEditorBlock_Insert(Environment* e, TextEditorBlock* block, UINT64 position, CHAR16 value)
{
	while (position >= (UINT64)(block->Width * block->Height))
	{
		position -= block->Width * block->Height;

		if (block->Next == 0)
		{
			block = TextEditorBlock_AddPage(e, block);
			if (block == 0) return;
		}
		else
		{
			block = block->Next;
		}
	}

	INT64 start = (position / block->Width) * block->Width;

	for (INT64 i = 1; i <= block->Width; i++)
	{
		INT64 index = (block->Width - i) + start;

		if (block->Buffer[index] != 0)
		{
			CHAR16 next;

			if (i == 1)
			{
				next = block->Buffer[index];
			}

			for (UINT64 j = index + 1; j > position; j--)
			{
				block->Buffer[j] = block->Buffer[j - 1];
			}

			block->Buffer[position] = value;

			for (INT64 j = position - 1; j >= start; j--)
			{
				if (block->Buffer[j] == 0)
				{
					block->Buffer[j] = L' ';
				}
				else
				{
					break;
				}
			}

			if (i == 1)
			{
				TextEditorBlock_Insert(e, block, index + 1, next);
			}

			break;
		}
		else if (i == block->Width)
		{
			block->Buffer[position] = value;

			for (INT64 j = position - 1; j >= start; j--)
			{
				block->Buffer[j] = L' ';
			}

			break;
		}
	}
}

void TextEditorBlock_Remove(Environment* e, TextEditorBlock* block, UINT64 position, CHAR16 value)
{
	while (position >= (UINT64)(block->Width * block->Height))
	{
		position -= block->Width * block->Height;

		if (block->Next == 0)
		{
			block = TextEditorBlock_AddPage(e, block);
			if (block == 0) return;
		}
		else
		{
			block = block->Next;
		}
	}

	UINT64 end = (((position / block->Width) + 1) * block->Width) - 1;

	for (UINT64 i = position; i < end; i++)
	{
		block->Buffer[i] = block->Buffer[i + 1];
	}

	for (UINT64 i = end; i > position; i--)
	{
		if (block->Buffer[i] == ' ')
		{
			block->Buffer[i] = 0;
		}
		else if (block->Buffer[i])
		{
			break;
		}
	}
}

void TextEditorBlock_Draw(TextEditor editor, TextEditorBlock* block)
{
	editor.Environment->Table->ConOut->EnableCursor(editor.Environment->Table->ConOut, 0);

	SetColor(editor.Environment, EFI_LIGHTGRAY, EFI_BLACK);

	ClearScreen(editor.Environment);

	for (INT64 y = 0; y < block->Height; y++)
	{
		SetPos(editor.Environment, 0, y);

		CHAR16* line = &block->Buffer[y * block->Width];

		Print(L"%.*s", StrnLen(line, block->Width), line);
	}

	SetColor(editor.Environment, EFI_BLACK, EFI_WHITE);

	SetPos(editor.Environment, 0, editor.Environment->Screen.Size.Height - 1);

	INT64 page = TextEditorBlock_DistanceToStart(block) + 1;
	INT64 total = TextEditorBlock_DistanceToEnd(block) + page;
	INT64 bytes = total * (sizeof(TextEditorBlock) + ((block->Width * block->Height) * sizeof(CHAR16)));

	Print(L" Page %d/%d - %d.%02d Kb ", page, total, bytes / 1000, (bytes % 1000) / 10);

	SetPos(editor.Environment, editor.CursorX, editor.CursorY);

	SetColor(editor.Environment, EFI_LIGHTGRAY, EFI_BLACK);

	editor.Environment->Table->ConOut->EnableCursor(editor.Environment->Table->ConOut, 1);
}

void TextEditor_Run(Environment* e)
{
	TextEditor editor;
	editor.CursorX = 0;
	editor.CursorY = 0;
	editor.Environment = e;
	editor.Page = New_TextEditorBlock(e);

	while (1)
	{
		TextEditorBlock_Draw(editor, editor.Page);

		CHAR16 scan;
		CHAR16 chr = WaitForKeyWithScanCode(e, &scan);

		if (chr == 0)
		{
			switch (scan)
			{
				case SCAN_UP:
					editor.CursorY -= 1;
					break;
				case SCAN_DOWN:
					editor.CursorY += 1;
					break;
				case SCAN_LEFT:
					editor.CursorX -= 1;
					break;
				case SCAN_RIGHT:
					editor.CursorX += 1;
					break;
				case SCAN_HOME:
					editor.CursorX = 0;
					break;
				case SCAN_END:
					editor.CursorX = editor.Page->Width - 1;
					break;
				case SCAN_PAGE_UP:
					editor.CursorY -= editor.Page->Height;
					break;
				case SCAN_PAGE_DOWN:
					editor.CursorY += editor.Page->Height;
					break;
				case SCAN_ESC:
					{
						SetPos(e, 0, 0);
						SetColor(e, EFI_BLACK, EFI_WHITE);
						Print(L" Save before closing? (Y/N/C) ");

						CHAR16 c = 0;

						while (!(c == L'Y' || c == L'y' || c == L'N' || c == L'n' || c == L'C' || c == L'c'))
						{
							c = WaitForKey(e);
						}

						if (c >= L'a') c -= 32;

						switch (c)
						{
							case 'Y':
								ClearScreen(editor.Environment);
								return;
							case 'N':
								ClearScreen(editor.Environment);
								return;
						}

						TextEditorBlock_Draw(editor, editor.Page);
					}
					break;
			}
		}
		else if (chr >= L' ' && chr <= L'~')
		{
			TextEditorBlock_Insert(e, editor.Page, editor.CursorX + (editor.CursorY * editor.Page->Width), chr);
			editor.CursorX += 1;
		}
		else if (chr == L'\n' || chr == L'\r')
		{
			editor.CursorX = 0;
			editor.CursorY += 1;
		}
		else if (chr == L'\b')
		{
			editor.CursorX -= 1;

			if (editor.CursorX >= 0)
			{
				TextEditorBlock_Remove(e, editor.Page, editor.CursorX + (editor.CursorY * editor.Page->Width), chr);
			}
		}

		if (editor.CursorX < 0)
		{
			editor.CursorX = 0;
		}

		while (editor.CursorX >= editor.Page->Width)
		{
			editor.CursorY += 1;
			editor.CursorX -= editor.Page->Width;
		}

		while (editor.CursorY < 0)
		{
			if (editor.Page->Previous)
			{
				editor.Page = editor.Page->Previous;
				editor.CursorY += editor.Page->Height;
			}
			else
			{
				editor.CursorY = 0;
			}
		}

		while (editor.CursorY >= editor.Page->Height)
		{
			if (editor.Page->Next)
			{
				editor.Page = editor.Page->Next;
				editor.CursorY -= editor.Page->Height;
			}
			else
			{
				TextEditorBlock_AddPage(editor.Environment, editor.Page);
			}
		}
	}
}