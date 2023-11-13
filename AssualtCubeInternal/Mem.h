#pragma once
#include <Windows.h>

namespace mem
{
	void write(BYTE* dst, const BYTE* src, unsigned int size);
	void nop(BYTE* dst, unsigned int size);
}
