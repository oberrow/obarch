#pragma once

namespace oasm
{
	enum class opcodes
	{
		NOP,
		ADD,
		SUB,
		MOV_R16_IMM16,
		MOV_R16_R16,
		MOV_M16_R16,
		MOV_R16_M16,
		JMP_IMM16,
		JMP_R16,
		PUSH_R16,
		POP_R16,
		CALL_IMM16,
		RET,
		AND,
		OR,
		NOT,
		HLTCLK,
		RESV1,
		RESV2,
		RESV3,
		CALL_R16,
		POP,
		OUT,
		OUTC,
		JCC,
		CMP_R16_R16,
		PUSH_IMM16,
		INC_R16,
		DEC_R16,
		CMP_R16_IMM16,
	};
	
}