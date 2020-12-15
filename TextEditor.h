#pragma once
#include "stdlib.h"
#include "Console.h"

typedef struct TextEditorBlock
{
	TextEditorBlock* Previous;
	TextEditorBlock* Next;
	UINT64 Width;
	UINT64 Height;
	CHAR16 Buffer[0];
} TextEditorBlock;

TextEditorBlock* New_TextEditorBlock(Environment* e)
{
	Size screenSize = e->Screen.Size;

	TextEditorBlock* result = (TextEditorBlock*)zmalloc(sizeof(TextEditorBlock) + ((screenSize.Width * screenSize.Height) * sizeof(CHAR16))).Start;

	result->Width = screenSize.Width;
	result->Height = screenSize.Height;

	return result;
}

void Dispose_TextEditorBlock(TextEditorBlock* block)
{
	freeany(block);
}

CHAR16* TextEditorBlock_ToString(TextEditorBlock* block)
{
	UINT64* lineLengths = (UINT64*)malloc(block->Height).Start;

	for (UINT64 i = 0; i < block->Height; i++)
	{
		int success = 0;

		for (UINT64 j = 0; j < block->Width; j++)
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

	UINT64 bufferSize = 0;

	for (UINT64 i = 0; i < block->Height; i++)
	{
		bufferSize += lineLengths[i] + 1;
	}

	CHAR16* buffer = malloc(bufferSize * sizeof(CHAR16)).Start;
	UINT64 position = 0;

	for (UINT64 i = 0; i < block->Height; i++)
	{
		UINT64 length = lineLengths[i];

		if (length > 0)
		{
			StrnCpy(&buffer[position], block->Buffer[i * block->Width], length);
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

void TextEditorBlock_AddPage(Environment* e, TextEditorBlock* block)
{
	while (block->Next != 0)
	{
		block = block->Next;
	}

	block->Next = New_TextEditorBlock(e);
	block->Next->Previous = block;
}