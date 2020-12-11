#pragma once
#include <efi.h>

typedef struct
{
	UINT8* Start;
	UINT8* Position;
	UINT64 Length;
} Emitter;

//Create a new emitter.
Emitter New_Emitter(void* ptr, UINTN length)
{
	Emitter result;
	result.Start = ptr;
	result.Position = ptr;
	result.Length = length;
	return result;
}

//Jump to the emitter code.
void Emitter_Execute(Emitter emit)
{
	((void (*)(void))emit.Start)();
}

//Emit the specified bytes.
EFI_STATUS Emitter_EmitBytes(Emitter emit, UINT8* value, UINTN length)
{
	if ((emit.Position + length) >= (emit.Start + emit.Length) && emit.Position >= emit.Start)
	{
		return EFI_OUT_OF_RESOURCES;
	}
	else
	{
		for (UINTN i = 0; i < length; i++)
		{
			emit.Position[i] = value[i];
		}

		emit.Position += length;

		return EFI_SUCCESS;
	}
}

//Emit a short relative jump.
EFI_STATUS Emitter_EmitShortRelativeJump(Emitter emit, INT8 rel)
{
	UINT8 buffer[2] = { 0xEB, (UINT8)rel };

	return Emitter_EmitBytes(emit, buffer, 2);
}