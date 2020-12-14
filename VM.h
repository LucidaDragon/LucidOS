#pragma once
#include "ArrayList.h"

typedef enum
{
	Active,
	Idle,
	Finished
} VMStatus;

typedef struct
{
	VMStatus Status;
	UINTN Id;
	UINT8 Priority;

	MemBlock Memory;

	ArrayList Stack;

	UINT64* Variables;
	UINTN VarCount;

	UINT8* Start;
	UINT8* Current;
	UINT8* Error;
} VM;

typedef enum
{
	HLT,
	BRK,

	PUSH,
	POP,
	LDSTACK,

	LDVAR,
	STVAR,

	ADD,
	SUB,
	MUL,
	IMUL,
	DIV,
	IDIV,
	MOD,
	IMOD,

	AND,
	OR,
	XOR,
	NOT,

	EQU,
	NEQ,
	ABV,
	BEL,
	GTR,
	LES,

	JMP,
	JIF
} OpCode;

VM New_VM(MemBlock memory, UINTN id, UINT8 priority, UINT64* variables, UINT64 varCount, UINT8* start, UINT8* error)
{
	VM vm;
	vm.Status = Active;
	vm.Id = id;
	vm.Priority = priority;
	vm.Memory = memory;
	vm.Stack = New_ArrayList();
	vm.Variables = variables;
	vm.VarCount = varCount;
	vm.Start = start;
	vm.Current = start;
	vm.Error = error;
	return vm;
}

inline int VM_ValidPointer(VM* vm)
{
	return vm->Current >= (UINT8*)vm->Memory.Start && vm->Current < ((UINT8*)vm->Memory.Start + vm->Memory.Size);
}

inline int VM_ValidOperand(VM* vm)
{
	return vm->Current >= (UINT8*)vm->Memory.Start && (vm->Current + 8) < ((UINT8*)vm->Memory.Start + vm->Memory.Size);
}

void VM_PushStack(VM* vm, UINT64 operand)
{
	ArrayList_Add(&vm->Stack, (void*)operand);
}

int VM_PopStack(VM* vm, UINT64* value)
{
	if (vm->Stack.Length > 0)
	{
		*value = (UINT64)ArrayList_RemoveAt(&vm->Stack, vm->Stack.Length - 1);
		return 1;
	}
	else
	{
		vm->Current = vm->Error;
		return 0;
	}
}

void VM_Execute(VM* vm)
{
	if (!VM_ValidPointer(vm))
	{
		vm->Current = vm->Error;
		return;
	}

	UINT8 op = *vm->Current;
	UINT64 operand = 0;
	UINT64 value = 0;
	vm->Current++;

	if (op == PUSH || op == LDVAR || op == STVAR)
	{
		if (!VM_ValidOperand(vm))
		{
			vm->Current = vm->Error;
			return;
		}

		operand = *((UINT64*)vm->Current);
		vm->Current += 8;
	}

	switch (op)
	{
		case HLT:
			vm->Status = Finished;
			return;
		case BRK:
			vm->Status = Idle;
			return;
		case PUSH:
			VM_PushStack(vm, operand);
			return;
		case POP:
			VM_PopStack(vm, &operand);
			return;
		case LDSTACK:
			VM_PushStack(vm, vm->Stack.Length);
			return;
		case LDVAR:
			if (operand < vm->VarCount)
			{
				VM_PushStack(vm, vm->Variables[operand]);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case STVAR:
			if (operand < vm->VarCount && VM_PopStack(vm, &value))
			{
				vm->Variables[operand] = value;
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case ADD:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value + operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case SUB:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value - operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case MUL:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value * operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case IMUL:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, (INT64)value * (INT64)operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case DIV:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value / operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case IDIV:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, (INT64)value / (INT64)operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case MOD:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value % operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case IMOD:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, (INT64)value % (INT64)operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case AND:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value & operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case OR:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value | operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case XOR:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value ^ operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case NOT:
			if (VM_PopStack(vm, &operand))
			{
				VM_PushStack(vm, ~operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case EQU:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value == operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case NEQ:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value != operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case ABV:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value > operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case BEL:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, value < operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case GTR:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, (INT64)value > (INT64)operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case LES:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				VM_PushStack(vm, (INT64)value < (INT64)operand);
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case JMP:
			if (VM_PopStack(vm, &operand))
			{
				vm->Current = vm->Current + (INT64)operand;

				if (!VM_ValidPointer(vm))
				{
					vm->Current = vm->Error;
				}
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
		case JIF:
			if (VM_PopStack(vm, &operand) && VM_PopStack(vm, &value))
			{
				if (value)
				{
					vm->Current = vm->Current + (INT64)operand;

					if (!VM_ValidPointer(vm))
					{
						vm->Current = vm->Error;
					}
				}
			}
			else
			{
				vm->Current = vm->Error;
			}
			return;
	}
}