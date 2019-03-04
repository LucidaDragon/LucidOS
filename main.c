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

//Object that represents a stack.
typedef struct {
	UINTN Value;
	void *Next;
} STACKNODE;





//
// Library Ports for EFI.
//

//MATH: Returns the smaller of two values.
UINTN min(UINTN a, UINTN b) {
	if (a < b) { return a; }
	else { return b; }
}

//MATH: Returns the larger of two values.
UINTN max(UINTN a, UINTN b) {
	if (a > b) { return a; }
	else { return b; }
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

//Gets the amount of available memory, rounded to the specified base.
UINTN memavail(UINTN base) {
	UINTN size = base;
	MEMBLOCK m = malloc(size);
	while (m.Start != NULL) {
		free(&m);
		m = malloc(size + base);
		if (m.Start != NULL) size += base;
	}
	return size;
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

//Waits for the specified key to be pressed.
void WaitForSpecificKey(ENVIRONMENT *e, CHAR16 key) {
	UINTN event;
	EFI_INPUT_KEY pressed;
	e->Table->ConIn->Reset(e->Table->ConIn, FALSE);
	while (TRUE)
	{
		e->Table->BootServices->WaitForEvent(1, &e->Table->ConIn->WaitForKey, &event);
		e->Table->ConIn->ReadKeyStroke(e->Table->ConIn, &pressed);
		if (pressed.UnicodeChar == key) {
			return;
		}
	}
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
// #Stack Functions#
//

//Create a stack node.
STACKNODE *CreateStack(UINTN value) {
	STACKNODE *s = malloc(sizeof(STACKNODE)).Start;
	s->Value = value;
	s->Next = NULL;
	return s;
}

//Checks to see if a stack is empty.
UINTN StackIsEmpty(STACKNODE *root) {
	return !root;
}

//Pushes to a stack.
void StackPush(STACKNODE **root, UINTN value) {
	STACKNODE *s = CreateStack(value);
	s->Next = *root;
	*root = s;
}

//Pops from a stack.
UINTN StackPop(STACKNODE **root) {
	if (StackIsEmpty(*root)) return 0;
	STACKNODE *temp = *root;
	*root = (*root)->Next;
	UINTN popped = temp->Value;
	MEMBLOCK m;
	m.Start = temp;
	free(&m);
	return popped;
}

//Peeks at a stack.
UINTN StackPeek(STACKNODE *root) {
	if (StackIsEmpty(root)) return 0;
	return root->Value;
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
// LucidOS Runtime Definitions
//

//
// #Structure Definitions#
//

//Represents a runtime array.
typedef struct
{
	UINTN Start;
} Array;

//Represents a runtime int in memory.
typedef struct
{
	BOOLEAN IsNull;
	INTN Value;
} Int;

//Special calls that do not run a user-defined function.
typedef enum
{
	Add = 1,
	Subtract = 2,
	Multiply = 3,
	Divide = 4
} SpecialCall;

//Runtime environment registers.
typedef struct
{
	INTN eax; //Return value
	INTN ebx; //Param 1
	INTN ecx; //Param 2
	INTN edx; //Param 3
	UINTN esi; //Source pointer
	UINTN edi; //Dest pointer
	UINTN ebp; //Stack frame pointer
	UINTN esp; //Stack top pointer

	UINTN inp; //Instruction pointer

	UINTN hpp; //Heap pointer
} Registers;

//Represents the runtime environment.
typedef struct
{
	Registers Registers;
	UINTN HeapLength;
	UINTN StackLength;
	Int *HeapMemory;
	Int *StackMemory;
} EE;

//
// #Array/Memory Functions#
//

//Get the length of a runtime array.
UINTN GetArrayLength(UINTN start, Int *source) {
	int i = 0;
	while (!source[start + i].IsNull)
	{
		i++;
	}
	return i;
}

//Get the element in a runtime array.
Int GetArrayElement(UINTN start, UINTN index, Int *source) {
	return source[start + index];
}

//Allocate runtime memory.
UINTN Malloc(EE *ee, UINTN size) {
	UINTN start = ee->Registers.hpp + 1;
	for (UINTN i = start; i < ee->Registers.hpp + 1 + size; i++) //Zero fill the new memory.
	{
		ee->HeapMemory[i].Value = 0;
		ee->HeapMemory[i].IsNull = FALSE;
	}
	//Update the heap size with an extra null entry to mark the end.
	ee->Registers.hpp += size + 1;
	ee->HeapMemory[ee->Registers.hpp].Value = 0;
	ee->HeapMemory[ee->Registers.hpp].IsNull = FALSE;
	return start;
}

//Allocate runtime memory with a specified element width.
UINTN Calloc(EE *ee, UINTN count, UINTN size) {
	return Malloc(ee, count * size);
}

//Free runtime memory.
void Free(EE *ee, UINTN start) {
	UINTN len = GetArrayLength(start, ee->HeapMemory);
	for (UINTN i = 0; i < len; i++) //Zero fill the old memory.
	{
		ee->HeapMemory[i + start].Value = 0;
		ee->HeapMemory[i + start].IsNull = TRUE;
	}
	for (UINTN i = start + len + 2; i < ee->Registers.hpp; i++) //Shift all of the memory after it down to take its place.
	{
		ee->HeapMemory[i - len - 2].Value = ee->HeapMemory[i].Value;
		ee->HeapMemory[i - len - 2].IsNull = FALSE;
	}
	for (UINTN i = ee->Registers.hpp; i < ee->HeapLength; i++) //Zero fill the remaining free space.
	{
		ee->HeapMemory[i].Value = 0;
		ee->HeapMemory[i].IsNull = TRUE;
	}
	ee->Registers.hpp -= len; //Update end of heap.
}

//Create an array in runtime memory.
Array CreateArray(EE *ee, UINTN length) {
	Array result;
	result.Start = Malloc(ee, length);
	return result;
}

//Remove an array from runtime memory.
void DestroyArray(EE *ee, Array arr) {
	Free(ee, arr.Start);
}

//
// #Stack Functions#
//

//Push to the runtime stack.
void Push(EE *ee, INTN value) {
	ee->StackMemory[ee->Registers.esp].IsNull = FALSE;
	ee->StackMemory[ee->Registers.esp].Value = value;
	ee->Registers.esp += 1;
}

//Pop from the runtime stack.
INTN Pop(EE *ee) {
	INTN value = ee->StackMemory[ee->Registers.esp].Value;
	ee->StackMemory[ee->Registers.esp].IsNull = TRUE;
	ee->StackMemory[ee->Registers.esp].Value = 0;
	ee->Registers.esp -= 1;
	return value;
}

//
// #Call Methods#
//

//Calls a special runtime operation.
void CallSpecial(EE *ee, SpecialCall call, Array args) {
	if (call == Add)
	{
		INTN result = 0;
		for (UINTN i = 0; i < GetArrayLength(args.Start, ee->HeapMemory); i++)
		{
			if (i == 0)
			{
				result = GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
			else
			{
				result += GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
		}
		Push(ee, result);
	}
	else if (call == Subtract)
	{
		INTN result = 0;
		for (UINTN i = 0; i < GetArrayLength(args.Start, ee->HeapMemory); i++)
		{
			if (i == 0)
			{
				result = GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
			else
			{
				result -= GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
		}
		Push(ee, result);
	}
	else if (call == Multiply)
	{
		INTN result = 0;
		for (UINTN i = 0; i < GetArrayLength(args.Start, ee->HeapMemory); i++)
		{
			if (i == 0)
			{
				result = GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
			else
			{
				result *= GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
		}
		Push(ee, result);
	}
	else if (call == Divide)
	{
		INTN result = 0;
		for (UINTN i = 0; i < GetArrayLength(args.Start, ee->HeapMemory); i++)
		{
			if (i == 0)
			{
				result = GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
			else
			{
				result /= GetArrayElement(args.Start, i, ee->HeapMemory).Value;
			}
		}
		Push(ee, result);
	}
}

//Calls a user-defined runtime operation.
void Call(EE *ee, UINTN location, Array args) {
	Push(ee, ee->Registers.eax);
	Push(ee, ee->Registers.ebx);
	Push(ee, ee->Registers.ecx);
	Push(ee, ee->Registers.edx);
	Push(ee, ee->Registers.esi);
	Push(ee, ee->Registers.edi);
	Push(ee, ee->Registers.ebp);
	Push(ee, ee->Registers.inp);
	ee->Registers.ebp = ee->Registers.esp;
	ee->Registers.inp = location;

	UINTN argLen = GetArrayLength(args.Start, ee->HeapMemory);
	Push(ee, argLen);
	for (UINTN i = 0; i < argLen; i++)
	{
		Push(ee, GetArrayElement(args.Start, i, ee->HeapMemory).Value);
	}
}

//
// #Function Info Methods#
//

//Gets the argument count for the current runtime function.
UINTN GetArgCount(EE *ee) {
	return ee->StackMemory[ee->Registers.ebp].Value;
}

//Gets an argument for the current runtime function.
INTN GetArg(EE *ee, UINTN i) {
	return ee->StackMemory[ee->Registers.ebp + i + 1].Value;
}

//Gets the local variable count for the current runtime function.
UINTN GetLocalCount(EE *ee) {
	return GetArrayLength(ee->Registers.ebp + GetArgCount(ee) + 1, ee->StackMemory);
}

//Gets a local value for the current runtime function.
INTN GetLocal(EE *ee, UINTN i) {
	return ee->StackMemory[ee->Registers.ebp + GetArgCount(ee) + 1 + i].Value;
}

//
// #Call Return Method#
//

//Returns a value from the current runtime function and puts it on the stack.
void Return(EE *ee) {
	INTN ret = ee->Registers.eax;
	while (ee->Registers.esp > ee->Registers.ebp)
	{
		Pop(ee);
	}
	ee->Registers.inp = Pop(ee);
	ee->Registers.ebp = Pop(ee);
	ee->Registers.edi = Pop(ee);
	ee->Registers.esi = Pop(ee);
	ee->Registers.edx = Pop(ee);
	ee->Registers.ecx = Pop(ee);
	ee->Registers.ebx = Pop(ee);
	ee->Registers.eax = Pop(ee);
	Push(ee, ret);
}

//
// #Runtime Environment Entrypoint#
//

//Begins execution in the specified runtime environment.
UINTN Main(EE *ee) {
	return 0;
}

//
// #Runtime Environment Constructor#
//

//Constructs a runtime environment and begins execution.
UINTN Enter(ENVIRONMENT *e, UINTN heapSize, UINTN stackSize) {
	EE env;
	env.HeapLength = heapSize;
	env.StackLength = stackSize;
	MEMBLOCK heap = calloc(env.HeapLength, sizeof(UINTN));
	MEMBLOCK stack = calloc(env.StackLength, sizeof(UINTN));
	env.HeapMemory = heap.Start; //Produces indirection warning, can be ignored.
	env.StackMemory = stack.Start; //Produces indirection warning, can be ignored.

	if (env.StackMemory == NULL || env.HeapMemory == NULL) {
		Print(L"Failed to assign the requested memory space for the program.\nSpace Requested: %u bytes\n", (heapSize + stackSize) * sizeof(UINTN));
		return -1;
	}

	Print(L"Stack Location: %x\n", env.StackMemory);
	Print(L"Heap Location: %x\n", env.HeapMemory);

	UINTN ret = Main(&env);

	Print(L"Exiting With Code: %u", ret);

	free(&heap);
	free(&stack);

	return ret;
}





//
// LucidOS Entry
//

//
// #Environment Runtime Functions#
//

//Enters a new OS environment.
void Environment(ENVIRONMENT *e) {
	ClearScreen(e);

	/*//Colors test
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

	Print(L"%NPress any key to continue...");
	WaitForKey(e);*/

	/*//Graphics test
	Clear(e, EFI_WHITE);
	RECT r; r.X = 3; r.Y = 3; r.Width = 10; r.Height = 5;
	FillRect(e, &r, EFI_RED);
	DrawRect(e, &r, EFI_BLUE);
	SetColor(e, EFI_BLACK, EFI_WHITE);
	WaitForKey(e);*/

	/*//Stacks test
	STACKNODE *s = CreateStack(0);
	StackPush(&s, 300);
	StackPush(&s, 200);
	StackPush(&s, 100);

	Print(L"%u\n", StackPop(&s));
	Print(L"%u\n", StackPop(&s));
	Print(L"%u\n", StackPop(&s));
	Print(L"%u\n", StackPop(&s));

	WaitForSpecificKey(e, L'A');*/

	Enter(e, 1000000000, 1000000);

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
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
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