#pragma once
#include <efi.h>

//Returns the smaller of two values.
UINTN min(UINTN a, UINTN b)
{
	if (a < b) return a;
	else return b;
}

//Returns the larger of two values.
UINTN max(UINTN a, UINTN b)
{
	if (a > b) return a;
	else return b;
}