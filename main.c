//
// EFI Library Includes
//

#include <efi.h>
#include <efilib.h>





//
// LucidOS Standard Structures
//

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
	EFI_FILE* File;
	Screen Screen;
} Environment;

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

//Object that represents a block of memory.
typedef struct
{
	void* Start;
	UINTN Size;
} MemBlock;

//Object that represents a stack.
typedef struct
{
	void* Value;
	void* Next;
} StackNode;

//Object that represents a linked list.
typedef struct LinkedListNode
{
	void* Element;
	struct LinkedListNode* Next;
	struct LinkedListNode* Previous;
} LinkedListNode;

//Object that represents an array list.
typedef struct
{
	MemBlock Data;
	UINTN Length;
	UINTN Capacity;
} ArrayList;





//
// Library Ports for EFI.
//

//MATH: Returns the smaller of two values.
UINTN min(UINTN a, UINTN b)
{
	if (a < b) return a;
	else return b;
}

//MATH: Returns the larger of two values.
UINTN max(UINTN a, UINTN b)
{
	if (a > b) return a;
	else return b;
}

//STDLIB: Allocates a block of memory with the specified size.
MemBlock malloc(UINTN size)
{
	MemBlock result;
	EFI_STATUS status;
	void* handle;
	status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size, &handle);
	if (status == EFI_OUT_OF_RESOURCES)
	{
		result.Start = NULL;
		result.Size = 0;
	}
	else if (status == EFI_INVALID_PARAMETER)
	{
		result.Start = NULL;
		result.Size = 0;
	}
	else
	{
		result.Start = handle;
		result.Size = size;
	}
	return result;
}

//STDLIB: Allocates a block of memory for the specified number of items of the specified size.
MemBlock calloc(UINTN num, UINTN size)
{
	MemBlock result = malloc(num * size);
	uefi_call_wrapper(BS->SetMem, 3, result.Start, result.Size, 0);
	return result;
}

//STDLIB: Deallocates the specified block of memory.
void free(MemBlock* block)
{
	uefi_call_wrapper(BS->FreePool, 1, block->Start);
	block->Start = NULL;
	block->Size = 0;
}

void freeany(void* ptr)
{
	uefi_call_wrapper(BS->FreePool, 1, ptr);
}

//STDLIB: Resizes the specified block of memory.
MemBlock realloc(MemBlock* block, UINTN size)
{
	MemBlock result = malloc(size);
	uefi_call_wrapper(BS->CopyMem, 3, result.Start, block->Start, size);
	free(block);
	return result;
}

//STDLIB: Creates a copy of the specified block of memory.
MemBlock memcopy(MemBlock* block)
{
	MemBlock result = malloc(block->Size);
	uefi_call_wrapper(BS->CopyMem, 3, result.Start, block->Start, block->Size);
	return result;
}

//
// #Linked List Functions#
//

LinkedListNode* New_LinkedListNode()
{
	LinkedListNode* node = malloc(sizeof(LinkedListNode)).Start;
	node->Element = NULL;
	node->Next = NULL;
	node->Previous = NULL;
	return node;
}

void Dispose_LinkedListNode(LinkedListNode* node)
{
	LinkedListNode* next = node->Next;
	LinkedListNode* prev = node->Previous;

	node->Next = NULL;
	node->Previous = NULL;

	if (next != NULL)
	{
		next->Previous = NULL;
		Dispose_LinkedListNode(next);
	}

	if (prev != NULL)
	{
		prev->Next = NULL;
		Dispose_LinkedListNode(prev);
	}

	freeany(node);
}

LinkedListNode* LinkedListNode_First(LinkedListNode* node)
{
	LinkedListNode* current = node;

	while (current->Previous != NULL) current = current->Previous;

	return current;
}

LinkedListNode* LinkedListNode_Last(LinkedListNode* node)
{
	LinkedListNode* current = node;

	while (current->Next != NULL) current = current->Next;

	return current;
}

void LinkedListNode_AddNext(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* next = node->Next;

	node->Next = child;
	child->Next = next;
	child->Previous = node;

	if (next != NULL) next->Previous = child;
}

void LinkedListNode_AddPrevious(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* prev = node->Previous;

	node->Previous = child;
	child->Previous = prev;
	child->Next = node;

	if (prev != NULL) prev->Next = child;
}

void LinkedListNode_AddFirst(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* current = LinkedListNode_Last(node);

	child->Next = current;
	current->Previous = child;
}

void LinkedListNode_AddLast(LinkedListNode* node, LinkedListNode* child)
{
	LinkedListNode* current = LinkedListNode_First(node);

	child->Previous = current;
	current->Next = child;
}

void LinkedListNode_Remove(LinkedListNode* node)
{
	if (node->Previous != NULL) node->Previous->Next = node->Next;
	if (node->Next != NULL) node->Next->Previous = node->Previous;

	node->Next = NULL;
	node->Previous = NULL;

	Dispose_LinkedListNode(node);
}

//
// #Array List Functions#
//

void New_ArrayList(ArrayList* list, UINTN elementWidth)
{
	list->Length = 0;
	list->Capacity = 1;
	list->Data = malloc(sizeof(void*));
}

void Dispose_ArrayList(ArrayList* list)
{
	list->Length = 0;
	list->Capacity = 0;
	free(&list->Data);
}

void ArrayList_Add(ArrayList* list, void* element)
{
	if (list->Length >= list->Capacity)
	{
		list->Data = realloc(&list->Data, list->Capacity * 2);
	}

	((void**)list->Data.Start)[list->Length] = element;
	list->Length += 1;
}

void ArrayList_Insert(ArrayList* list, void* element, UINTN index)
{
	ArrayList_Add(list, element);

	for (UINTN i = index; i < list->Length - 1; i++)
	{
		((void**)list->Data.Start)[i + 1] = ((void**)list->Data.Start)[i];
	}

	((void**)list->Data.Start)[index] = element;
}

BOOLEAN ArrayList_Remove(ArrayList* list, void* element)
{
	BOOLEAN result = FALSE;

	for (UINTN i = 0; i < list->Length; i++)
	{
		void* current = ((void**)list->Data.Start)[i];

		if (current == element)
		{
			result = TRUE;

			for (UINTN j = i; j < list->Length - 1; j++)
			{
				((void**)list->Data.Start)[j] = ((void**)list->Data.Start)[j + 1];
			}

			list->Length -= 1;
			break;
		}
	}

	if (result && (list->Length < (list->Capacity / 2)))
	{
		list->Data = realloc(&list->Data, list->Capacity / 2);
	}

	return result;
}





//
// LucidOS Standard Functions
//

//
// #Basic IO Functions#
//

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

//Prints the current system time.
void PrintTime(Environment* e, BOOLEAN newLine)
{
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
void SetPos(Environment* e, UINTN x, UINTN y)
{
	e->Table->ConOut->SetCursorPosition(e->Table->ConOut, x, y);
}

//
// #Stack Functions#
//

//Create a stack node.
StackNode* New_StackNode(void* initialElement)
{
	StackNode* s = malloc(sizeof(StackNode)).Start;
	s->Value = initialElement;
	s->Next = NULL;
	return s;
}

//Checks to see if a stack is empty.
BOOLEAN StackNode_IsEmpty(StackNode* root)
{
	return !root;
}

//Pushes to a stack.
void StackNode_Push(StackNode** root, void* element)
{
	StackNode* s = New_StackNode(element);
	s->Next = *root;
	*root = s;
}

//Pops from a stack.
void* StackNode_Pop(StackNode** root)
{
	if (StackNode_IsEmpty(*root)) return NULL;
	StackNode* node = *root;
	*root = (*root)->Next;
	void* element = node->Value;
	freeany(node);
	return element;
}

//Peeks at a stack.
void* StackNode_Peek(StackNode* root)
{
	if (StackNode_IsEmpty(root)) return 0;
	return root->Value;
}

//
// #Graphics Functions#
//

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
	for (UINTN i = 0; i < e->Screen.Size.Height; i++) {
		DrawBar(e, color);
	}
	SetPos(e, 0, 0);
}

//Fills a rectangle with the specified color.
void FillRect(Environment* e, Rect* r, UINT8 color)
{
	for (UINTN y = 0; y < r->Height; y++) {
		for (UINTN x = 0; x < r->Width; x++) {
			SetPos(e, x + r->X, y + r->Y);
			PrintColor(e, color);
		}
	}
}

//Draws a rectangle with the specified color.
void DrawRect(Environment* e, Rect* r, UINT8 color)
{
	for (UINTN i = 0; i < r->Width; i++) {
		SetPos(e, i + r->X, r->Y);
		PrintColor(e, color);
		SetPos(e, i + r->X, r->Y + r->Height);
		PrintColor(e, color);
	}
	for (UINTN i = 0; i <= r->Height; i++) {
		SetPos(e, r->X, i + r->Y);
		PrintColor(e, color);
		SetPos(e, r->X + r->Width, i + r->Y);
		PrintColor(e, color);
	}
}

//
// #Disk IO Functions#
//

void GetEntries(Environment* e, EFI_FILE* directory, MemBlock* memory, EFI_FILE_INFO** results, UINTN* resultCount)
{
	UINTN size = 0;
	directory->GetInfo(directory, &gEfiFileInfoGuid, &size, NULL);
	MemBlock mem = malloc(size);
	EFI_FILE_INFO* buffer = (EFI_FILE_INFO*)mem.Start;
	directory->GetInfo(directory, &gEfiFileInfoGuid, &size, buffer);
	*results = buffer;
	*memory = mem;
}

//Load the specified executable into memory.
MemBlock Load(Environment* e, CHAR16* name)
{
	EFI_FILE* current;
	e->File->Open(e->File, &current, name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

	UINTN size;
	EFI_FILE_INFO info;
	current->GetInfo(current, &gEfiFileInfoGuid, &size, &info);
	MemBlock m = malloc(size);

	current->Read(e->File, &size, m.Start);

	e->File->Close(current);
	return m;
}





//
// LucidOS Entry
//

//
// #Environment Runtime Functions#
//

//Enters a new OS environment.
void EnterEnvironment(Environment* e)
{
	ClearScreen(e);

	//Colors test
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
	WaitForKey(e);

	//Graphics test
	Clear(e, EFI_WHITE);
	Rect r; r.X = 3; r.Y = 3; r.Width = 10; r.Height = 5;
	FillRect(e, &r, EFI_RED);
	DrawRect(e, &r, EFI_BLUE);
	SetColor(e, EFI_BLACK, EFI_WHITE);
	WaitForKey(e);

	//Stacks test
	UINTN vals[4] = { 100, 300, 200, 0 };
	StackNode* s = New_StackNode(&vals[3]);
	StackNode_Push(&s, &vals[1]);
	StackNode_Push(&s, &vals[2]);
	StackNode_Push(&s, &vals[0]);

	Print(L"%u\n", *(UINTN*)StackNode_Pop(&s));
	Print(L"%u\n", *(UINTN*)StackNode_Pop(&s));
	Print(L"%u\n", *(UINTN*)StackNode_Pop(&s));
	Print(L"%u\n", *(UINTN*)StackNode_Pop(&s));

	WaitForSpecificKey(e, L'A');

	Print(L"\n\nPress any key to exit.\n");
	WaitForKey(e);
}

//
// #Environment Initialization Functions#
//

//Configures the display to it's largest supported text mode size and returns the screen definition.
Screen ConfigureDisplay(EFI_SYSTEM_TABLE* table)
{
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
	Size s;
	s.Width = actualW;
	s.Height = actualH;
	Screen scr;
	scr.Size = s;
	scr.Mode = highestMode;
	return scr;
}

//Initialize the environment using the system table and image handle.
void InitEnvironment(EFI_HANDLE* image, EFI_SYSTEM_TABLE* table)
{
	table->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	Environment e;
	e.Image = image;
	e.Table = table;
	e.Screen = ConfigureDisplay(table);

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	table->BootServices->HandleProtocol(image, &gEfiLoadedImageProtocolGuid, &LoadedImage);
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
	table->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, &FileSystem);
	FileSystem->OpenVolume(FileSystem, &e.File);

	EnterEnvironment(&e);
}

//
// #EFI Initialization Functions#
//

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
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