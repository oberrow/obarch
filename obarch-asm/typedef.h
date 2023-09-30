#pragma once

#include <vector>

namespace oasm
{
	using byte = uint8_t;
	namespace obarch
	{
		// Has nothing to do with the output.
		enum class mneumonic
		{
			INVALID,
			NOP,
			MOV,
			ADD,
			SUB,
			JMP,
			RET,
			PUSH,
			POP,
			CALL,
			AND,
			OR,
			NOT,
			HLTCLK,
			OUT,
			OUTC,
			JCC,
			CMP,
			INC,
			DEC,
		};
		using arch_intptr = int16_t;
		using arch_uintptr = uint16_t;
	}
#ifdef OASM_OBARCH
	using mneumonic = obarch::mneumonic;
	using arch_intptr = obarch::arch_intptr;
	using arch_uintptr = obarch::arch_uintptr;
#else
#error Unknown archiecture.
#endif
	struct label
	{
		arch_uintptr address = 0;
		bool is_unresolved = false;
		std::vector<arch_uintptr> references;
	};
}