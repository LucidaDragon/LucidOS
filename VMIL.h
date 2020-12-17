#pragma once
#include "VM.h"
#include "File.h"

typedef struct
{
	UINT8 Operation;
	UINT64 Operand;
} VMInstruction;

EFI_STATUS VMIL_FromInstruction(UINT8* data, UINT64* position, UINT64 length, VMInstruction inst)
{
	if (*position >= length) return EFI_INVALID_PARAMETER;

	data[*(position++)] = inst.Operation;

	if (inst.Operation & IMMEDIATE)
	{
		if ((*position + 8) > length)
		{
			return EFI_INVALID_PARAMETER;
		}
		else
		{
			*(UINT64*)&data[*position] = inst.Operand;
			*position += 8;
		}
	}

	return EFI_SUCCESS;
}

EFI_STATUS VMIL_OpcodeFromString(CHAR16* buffer, UINT64 bufferSize, UINT8* result)
{
	if (!StrnCmp(L"HLT", buffer, bufferSize))
	{
		*result = HLT;
	}
	else if (!StrnCmp(L"BRK", buffer, bufferSize))
	{
		*result = BRK;
	}
	else if (!StrnCmp(L"PUSH", buffer, bufferSize))
	{
		*result = PUSH;
	}
	else if (!StrnCmp(L"DUP", buffer, bufferSize))
	{
		*result = DUP;
	}
	else if (!StrnCmp(L"POP", buffer, bufferSize))
	{
		*result = POP;
	}
	else if (!StrnCmp(L"LDSTACK", buffer, bufferSize))
	{
		*result = LDSTACK;
	}
	else if (!StrnCmp(L"LDVAR", buffer, bufferSize))
	{
		*result = LDVAR;
	}
	else if (!StrnCmp(L"LDINDVAR", buffer, bufferSize))
	{
		*result = LDINDVAR;
	}
	else if (!StrnCmp(L"STVAR", buffer, bufferSize))
	{
		*result = STVAR;
	}
	else if (!StrnCmp(L"ADD", buffer, bufferSize))
	{
		*result = ADD;
	}
	else if (!StrnCmp(L"SUB", buffer, bufferSize))
	{
		*result = SUB;
	}
	else if (!StrnCmp(L"MUL", buffer, bufferSize))
	{
		*result = MUL;
	}
	else if (!StrnCmp(L"IMUL", buffer, bufferSize))
	{
		*result = IMUL;
	}
	else if (!StrnCmp(L"DIV", buffer, bufferSize))
	{
		*result = DIV;
	}
	else if (!StrnCmp(L"IDIV", buffer, bufferSize))
	{
		*result = IDIV;
	}
	else if (!StrnCmp(L"MOD", buffer, bufferSize))
	{
		*result = MOD;
	}
	else if (!StrnCmp(L"IMOD", buffer, bufferSize))
	{
		*result = IMOD;
	}
	else if (!StrnCmp(L"AND", buffer, bufferSize))
	{
		*result = AND;
	}
	else if (!StrnCmp(L"OR", buffer, bufferSize))
	{
		*result = OR;
	}
	else if (!StrnCmp(L"XOR", buffer, bufferSize))
	{
		*result = XOR;
	}
	else if (!StrnCmp(L"NOT", buffer, bufferSize))
	{
		*result = NOT;
	}
	else if (!StrnCmp(L"EQU", buffer, bufferSize))
	{
		*result = EQU;
	}
	else if (!StrnCmp(L"NEQ", buffer, bufferSize))
	{
		*result = NEQ;
	}
	else if (!StrnCmp(L"ABV", buffer, bufferSize))
	{
		*result = ABV;
	}
	else if (!StrnCmp(L"BEL", buffer, bufferSize))
	{
		*result = BEL;
	}
	else if (!StrnCmp(L"GTR", buffer, bufferSize))
	{
		*result = GTR;
	}
	else if (!StrnCmp(L"LES", buffer, bufferSize))
	{
		*result = LES;
	}
	else if (!StrnCmp(L"JMP", buffer, bufferSize))
	{
		*result = JMP;
	}
	else if (!StrnCmp(L"JIF", buffer, bufferSize))
	{
		*result = JIF;
	}
	else
	{
		return EFI_INVALID_PARAMETER;
	}

	return EFI_SUCCESS;
}

EFI_STATUS VMIL_IntFromString(CHAR16* buffer, UINT64 bufferSize, UINT64* result)
{
	if (bufferSize == 0) return EFI_BAD_BUFFER_SIZE;

	int negate = 0;
	*result = 0;

	for (UINT64 i = 0; i < bufferSize; i++)
	{
		if (i == 0 && buffer[i] == L'-')
		{
			negate = 1;
		}
		else if (buffer[i] >= L'0' && buffer[i] <= L'9')
		{
			*result = (*result * 10) + (buffer[i] - L'0');
		}
		else
		{
			return EFI_INVALID_PARAMETER;
		}
	}

	if (negate) *result = ~(*result) + 1;

	return EFI_SUCCESS;
}

EFI_STATUS VMIL_FromStringLine(UINT8* data, UINT64* position, UINT64 length, CHAR16* buffer, UINT64 bufferSize)
{
	if (*position >= length) return EFI_INVALID_PARAMETER;

	int state = 0;
	UINT64 opcodeStart = 0;
	UINT64 opcodeLength = 0;
	UINT64 operandStart = 0;
	UINT64 operandLength = 0;

	for (UINT64 i = 0; i < bufferSize; i++)
	{
		if (buffer[i] == 0)
		{
			if (state == 1)
			{
				opcodeLength = i - opcodeStart;
			}
			else if (state == 3)
			{
				operandLength = i - operandStart;
			}

			break;
		}

		switch (state)
		{
			case 0:
				if (buffer[i] > ' ' && buffer[i] < 127)
				{
					state = 1;
					opcodeStart = i;
					i--;
				}
				break;
			case 1:
				if (buffer[i] <= ' ' || buffer[i] >= 127)
				{
					state = 2;
					opcodeLength = i - opcodeStart;
				}
				break;
			case 2:
				if (buffer[i] > ' ' && buffer[i] < 127)
				{
					state = 3;
					operandStart = i;
					i--;
				}
				break;
			case 3:
				if (buffer[i] <= ' ' || buffer[i] >= 127)
				{
					state = 4;
					operandLength = i - operandStart;
				}
				break;
		}
	}

	VMInstruction result;

	EFI_STATUS status = VMIL_OpcodeFromString(&buffer[opcodeStart], opcodeLength, &result.Operation);

	if (EFI_ERROR(status)) return status;

	if (result.Operation & IMMEDIATE)
	{
		if (operandLength == 0) return EFI_INVALID_PARAMETER;

		status = VMIL_IntFromString(&buffer[operandStart], operandLength, &result.Operand);

		if (EFI_ERROR(status)) return status;
	}

	return VMIL_FromInstruction(data, position, length, result);
}

EFI_STATUS VMIL_FromString(UINT8* data, UINT64* position, UINT64 length, CHAR16* buffer, UINT64 bufferSize, UINT64* errorStart, UINT64* errorLength)
{
	UINT64 lineStart = 0;

	for (UINT64 i = 0; i < bufferSize; i++)
	{
		if (buffer[i] == '\n' || buffer[i] == 0)
		{
			UINT64 lineLength = i - lineStart;

			if (lineLength != 0)
			{
				EFI_STATUS status = VMIL_FromStringLine(data, position, length, &buffer[lineStart], lineLength);

				if (EFI_ERROR(status))
				{
					*errorStart = lineStart;
					*errorLength = lineLength;
					return status;
				}
			}

			lineStart = i + 1;
		}
	}

	return EFI_SUCCESS;
}

EFI_STATUS VMIL_Load(EFI_FILE* source, UINTN id, VM* result)
{
	EFI_STATUS status;

	UINTN size;

	UINT64 length;
	size = sizeof(length);
	status = source->Read(source, &size, &length);
	if (EFI_ERROR(status)) return status;
	else if (size != sizeof(length)) return EFI_END_OF_FILE;

	MemBlock mem = malloc(length);
	if (mem.Size == 0) return EFI_OUT_OF_RESOURCES;

	UINT64 vars;
	size = sizeof(vars);
	status = source->Read(source, &size, &vars);
	if (EFI_ERROR(status))
	{
		free(&mem);
		return status;
	}
	else if (size != sizeof(vars))
	{
		free(&mem);
		return EFI_END_OF_FILE;
	}
	else if ((vars * sizeof(UINT64)) > length)
	{
		free(&mem);
		return EFI_BAD_BUFFER_SIZE;
	}

	UINT64 error;
	size = sizeof(error);
	status = source->Read(source, &size, &error);
	if (EFI_ERROR(status))
	{
		free(&mem);
		return status;
	}
	else if (size != sizeof(error))
	{
		free(&mem);
		return EFI_END_OF_FILE;
	}
	else if ((vars * sizeof(UINT64)) + error >= length)
	{
		free(&mem);
		return EFI_BAD_BUFFER_SIZE;
	}

	UINT8* entry = (UINT8*)mem.Start + (vars * sizeof(UINT64));

	if (entry < (UINT8*)mem.Start || ((UINT8*)mem.Start + length) <= entry)
	{
		free(&mem);
		return EFI_BAD_BUFFER_SIZE;
	}

	*result = New_VM(mem, id, 1, (UINT64*)mem.Start, vars, entry, entry + error);

	UINT8* target = entry;
	while (1)
	{
		if (target < (UINT8*)mem.Start || ((UINT8*)mem.Start + length) <= target)
		{
			free(&mem);
			return EFI_BAD_BUFFER_SIZE;
		}

		UINT8 value;
		size = sizeof(value);
		status = source->Read(source, &size, &value);
		if (EFI_ERROR(status) || size != sizeof(value)) break;

		*target = value;

		target++;
	}

	return EFI_SUCCESS;
}

EFI_STATUS VMIL_ToString(UINT8* data, UINT64* position, UINT64 length, CHAR16* buffer, UINTN bufferSize)
{
	if (bufferSize < 32) return EFI_BAD_BUFFER_SIZE;

	if (*position >= length) return EFI_INVALID_PARAMETER;

	UINT8 op = data[*(position++)];
	UINT64 operand = 0;

	if (op & IMMEDIATE)
	{
		if ((*position + 8) > length)
		{
			return EFI_INVALID_PARAMETER;
		}
		else
		{
			operand = *(UINT64*)&data[*position];
			*position += 8;
		}
	}

	switch (op)
	{
		case HLT:
			StrCpy(buffer, L"HLT");
			return EFI_SUCCESS;
		case BRK:
			StrCpy(buffer, L"BRK");
			return EFI_SUCCESS;
		case PUSH:
			SPrint(buffer, bufferSize, L"PUSH %d", operand);
			return EFI_SUCCESS;
		case DUP:
			StrCpy(buffer, L"DUP");
			return EFI_SUCCESS;
		case POP:
			StrCpy(buffer, L"POP");
			return EFI_SUCCESS;
		case LDSTACK:
			StrCpy(buffer, L"LDSTACK");
			return EFI_SUCCESS;
		case LDVAR:
			SPrint(buffer, bufferSize, L"LDVAR %d", operand);
			return EFI_SUCCESS;
		case LDINDVAR:
			SPrint(buffer, bufferSize, L"LDINDVAR %d", operand);
			return EFI_SUCCESS;
		case STVAR:
			SPrint(buffer, bufferSize, L"STVAR %d", operand);
			return EFI_SUCCESS;
		case ADD:
			StrCpy(buffer, L"ADD");
			return EFI_SUCCESS;
		case SUB:
			StrCpy(buffer, L"SUB");
			return EFI_SUCCESS;
		case MUL:
			StrCpy(buffer, L"MUL");
			return EFI_SUCCESS;
		case IMUL:
			StrCpy(buffer, L"IMUL");
			return EFI_SUCCESS;
		case DIV:
			StrCpy(buffer, L"DIV");
			return EFI_SUCCESS;
		case IDIV:
			StrCpy(buffer, L"IDIV");
			return EFI_SUCCESS;
		case MOD:
			StrCpy(buffer, L"MOD");
			return EFI_SUCCESS;
		case IMOD:
			StrCpy(buffer, L"IMOD");
			return EFI_SUCCESS;
		case AND:
			StrCpy(buffer, L"AND");
			return EFI_SUCCESS;
		case OR:
			StrCpy(buffer, L"OR");
			return EFI_SUCCESS;
		case XOR:
			StrCpy(buffer, L"XOR");
			return EFI_SUCCESS;
		case NOT:
			StrCpy(buffer, L"NOT");
			return EFI_SUCCESS;
		case EQU:
			StrCpy(buffer, L"EQU");
			return EFI_SUCCESS;
		case NEQ:
			StrCpy(buffer, L"NEQ");
			return EFI_SUCCESS;
		case ABV:
			StrCpy(buffer, L"ABV");
			return EFI_SUCCESS;
		case BEL:
			StrCpy(buffer, L"BEL");
			return EFI_SUCCESS;
		case GTR:
			StrCpy(buffer, L"GTR");
			return EFI_SUCCESS;
		case LES:
			StrCpy(buffer, L"LES");
			return EFI_SUCCESS;
		case JMP:
			StrCpy(buffer, L"JMP");
			return EFI_SUCCESS;
		case JIF:
			StrCpy(buffer, L"JIF");
			return EFI_SUCCESS;
	}

	return EFI_LOAD_ERROR;
}