#include <iostream>
#include <fstream>
#include <sstream>

#include <iomanip>

#include <string>

#include "../obarch-asm/typedef.h"
#include "opcodes.h"

#include <emmintrin.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#undef OUT

#define MAX_REGISTER_VALUE 0xA

oasm::byte* memory;
const char* g_filename = nullptr;

#ifdef __GNUC__
#define attribute(expr) __attribute__((expr))
#else
#define attribute(expr)
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
bool shouldHalt = false;
bool resetMachine = false;
bool isErrorHalt = false;
bool debug = false;
enum { PRINTMODE_HEX, PRINTMODE_DEC };
bool printMode = PRINTMODE_HEX;
enum class flag_bit
{
	ZERO = 1,
	LESS = 2,
	GREATER = 4,
};
enum class condition_bit
{
	ZERO = 1,
	LESS = 2,
	GREATER = 4,
	NOT_ZERO = 0xFE,
	NOT_LESS = 0xFD,
	NOT_GREATER = 0xFB,
};
union obarch_registers
{
	struct
	{
		uint16_t reg16[11];
	} regId;
	struct
	{
		uint16_t R1;
		uint16_t R2;
		uint16_t R3;
		uint16_t R4;
		uint16_t R5;
		uint16_t R6;
		uint16_t R7;
		uint16_t R8;
		uint16_t SP;
		uint16_t IP;
		uint16_t flags;
	} regName;
} attribute(packed) registers;
std::ostream& operator<<(std::ostream& os, obarch_registers registers)
{
	auto oldFlags = os.flags();
	char oldFill = os.fill('0');
	auto oldWidth = os.width(2);
	os << std::hex;
	os << "R1: 0x" << registers.regName.R1 << ", R2: 0x" << registers.regName.R2 << ", R3: 0x" << registers.regName.R3 << '\n'
	   << "R4: 0x" << registers.regName.R4 << ", R5: 0x" << registers.regName.R5 << ", R6: 0x" << registers.regName.R6 << '\n'
	   << "R7: 0x" << registers.regName.R7 << ", R8: 0x" << registers.regName.R8 << ", SP: 0x" << registers.regName.SP << '\n'
	   << "IP: 0x" << registers.regName.IP << "FLAGS: 0x" << registers.regName.flags;
	os.fill(oldFill);
	os.width(oldWidth);
	os.flags(oldFlags);
	return os;
}
#ifdef _MSC_VER
#pragma pack(pop)
#endif

void debug_handler();

int main(int argc, char** argv)
{
	if (argc == 1 || argc > 3)
	{
		printf("Usage: %s input_file [-d]", argv[0]);
		return 1;
	}
	if (argc == 3)
	{
		if (_stricmp(argv[2], "-d") != 0)
		{
			printf("Usage: %s input_file [-d]", argv[0]);
			return 1;
		}
		debug = true;
	}
#ifdef _WIN32
	memory = (oasm::byte*)VirtualAlloc(nullptr, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	memory = new oasm::byte{ 0x10000 };
#endif
	g_filename = argv[1];
	std::ifstream file{ g_filename };
	if (!file)
	{
		printf("%s doesn't exist.\n", g_filename);
		return 1;
	}
	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	if (fileSize > 0x10000)
	{
		printf("Input file is too big!\n");
		return 1;
	}
	file.close();
	file.open(g_filename, std::ios::binary);
	for (size_t i = 0; i < fileSize; i++)
		memory[i] = file.get();
	file.close();

reset:
	memset(&registers, 0, sizeof(registers));
	resetMachine = false;
	shouldHalt = false;
	oasm::opcodes opcode = (oasm::opcodes)memory[registers.regName.IP];
	uint16_t operand1 = 0x0000;
	uint16_t operand2 = 0x0000;
	while(true)
	{
		opcode = (oasm::opcodes)memory[registers.regName.IP];
		if (debug)
			debug_handler();
		if (registers.regName.IP == 255)
			registers.regName.IP = 0;
		switch (opcode)
		{
		case oasm::opcodes::ADD:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2];
			if (operand1 > MAX_REGISTER_VALUE || operand2 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regId.reg16[operand1] += registers.regId.reg16[operand2];
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::SUB:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2];
			if (operand1 > MAX_REGISTER_VALUE || operand2 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regId.reg16[operand1] -= registers.regId.reg16[operand2];
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::MOV_R16_IMM16:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2] | (memory[registers.regName.IP + 3] << 8);
			if (operand1 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regId.reg16[operand1] = operand2;
			registers.regName.IP += 4;
			break;
		}
		case oasm::opcodes::MOV_R16_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2];
			if (operand1 > MAX_REGISTER_VALUE || operand2 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regId.reg16[operand1] = registers.regId.reg16[operand2];
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::MOV_M16_R16:
		{
			operand1 = memory[registers.regName.IP + 1] | (memory[registers.regName.IP + 2] << 8);
			operand2 = memory[registers.regName.IP + 3];
			if (operand2 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			memory[operand1] = registers.regId.reg16[operand2] & 0xFF;
			memory[operand1 + 1] = registers.regId.reg16[operand2] >> 8;
			registers.regName.IP += 4;
			break;
		}
		case oasm::opcodes::MOV_R16_M16:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2] | (memory[registers.regName.IP + 3] << 8);
			if (operand1 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regId.reg16[operand1] = memory[operand2] | (memory[operand2 + 1] << 8);
			registers.regName.IP += 4;
			break;
		}
		case oasm::opcodes::JMP_IMM16:
		{
			operand1 = memory[registers.regName.IP + 1] | (memory[registers.regName.IP + 2] << 8);
			registers.regName.IP = operand1;
			break;
		}
		case oasm::opcodes::JMP_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regName.IP = registers.regId.reg16[operand1];
			break;
		}
		case oasm::opcodes::PUSH_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regName.SP -= 2;
			memory[registers.regName.SP] = registers.regId.reg16[operand1];
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::POP_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			registers.regId.reg16[operand1] = memory[registers.regName.SP];
			registers.regName.SP += 2;
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::CALL_IMM16:
		{
			operand1 = memory[registers.regName.IP + 1] | (memory[registers.regName.IP + 2] << 8);
			registers.regName.SP -= 2;
			memory[registers.regName.SP] = registers.regName.IP + 3;
			registers.regName.IP = operand1;
			break;
		}
		case oasm::opcodes::RET:
		{
			registers.regName.IP = memory[registers.regName.SP];
			registers.regName.SP += 2;
			break;
		}
		case oasm::opcodes::AND:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2];
			if (operand1 > MAX_REGISTER_VALUE || operand2 > MAX_REGISTER_VALUE)
			{
				shouldHalt = true;
				isErrorHalt = true;
				break;
			}
			registers.regId.reg16[operand1] &= registers.regId.reg16[operand2];
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::OR:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2];
			if (operand1 > 3 || operand2 > 3)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			registers.regId.reg16[operand1] |= registers.regId.reg16[operand2];
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::NOT:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > 3)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			registers.regId.reg16[operand1] = ~registers.regId.reg16[operand1];
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::HLTCLK:
		{
			shouldHalt = true;
			break;
		}
		case oasm::opcodes::RESV1:
		case oasm::opcodes::RESV2:
		case oasm::opcodes::RESV3:
		{
			isErrorHalt = true;
			shouldHalt = true;
			break;
		}
		case oasm::opcodes::CALL_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			registers.regName.SP -= 2;
			memory[registers.regName.SP] = registers.regName.IP + 2;
			registers.regName.IP = registers.regId.reg16[operand1];
			break;
		}
		case oasm::opcodes::OUT:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			if (!printMode)
				printf("0x%04x", registers.regId.reg16[operand1]);
			else
				printf("%d", registers.regId.reg16[operand1]);
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::OUTC:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			printf("%c", (char)(registers.regId.reg16[operand1] & 0xFF));
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::JCC:
		{
			condition_bit _operand1 = (condition_bit)memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2] | (memory[registers.regName.IP + 3] << 8);
			bool condition = false;
			switch (_operand1)
			{
			case condition_bit::ZERO:
				condition = (registers.regName.flags & (uint16_t)flag_bit::ZERO);
				break;
			case condition_bit::NOT_ZERO:
				condition = !(registers.regName.flags & (uint16_t)flag_bit::ZERO);
				break;
			case condition_bit::LESS:
				condition = (registers.regName.flags & (uint16_t)flag_bit::LESS);
				break;
			case condition_bit::NOT_LESS:
				condition = !(registers.regName.flags & (uint16_t)flag_bit::LESS);
				break;
			case condition_bit::GREATER:
				condition = (registers.regName.flags & (uint16_t)flag_bit::GREATER);
				break;
			case condition_bit::NOT_GREATER:
				condition = !(registers.regName.flags & (uint16_t)flag_bit::GREATER);
				break;
			}

			if(condition)
				registers.regName.IP = operand2;
			else	
				registers.regName.IP += 4;
			break;
		}
		case oasm::opcodes::CMP_R16_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2];
			if (operand1 > MAX_REGISTER_VALUE || operand2 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			operand1 = registers.regId.reg16[operand1];
			operand2 = registers.regId.reg16[operand2];
			if (operand1 < operand2)
				registers.regName.flags |= (uint16_t)flag_bit::LESS;
			else
				registers.regName.flags &= ~((uint16_t)flag_bit::LESS);
			if (operand1 > operand2)
				registers.regName.flags |= (uint16_t)flag_bit::GREATER;
			else
				registers.regName.flags &= ~((uint16_t)flag_bit::GREATER);
			if ((operand1 - operand2) == 0)
				registers.regName.flags |= (uint16_t)flag_bit::ZERO;
			else
				registers.regName.flags &= ~((uint16_t)flag_bit::ZERO);
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::PUSH_IMM16:
		{
			operand1 = memory[registers.regName.IP + 1] | (memory[registers.regName.IP + 2] << 8);
			registers.regName.SP -= 2;
			memory[registers.regName.SP] = operand1;
			registers.regName.IP += 3;
			break;
		}
		case oasm::opcodes::INC_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			registers.regId.reg16[operand1]++;
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::DEC_R16:
		{
			operand1 = memory[registers.regName.IP + 1];
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			registers.regId.reg16[operand1]--;
			registers.regName.IP += 2;
			break;
		}
		case oasm::opcodes::CMP_R16_IMM16:
		{
			operand1 = memory[registers.regName.IP + 1];
			operand2 = memory[registers.regName.IP + 2] | (memory[registers.regName.IP + 3] << 8);
			if (operand1 > MAX_REGISTER_VALUE)
			{
				isErrorHalt = true;
				shouldHalt = true;
				break;
			}
			operand1 = registers.regId.reg16[operand1];
			if (operand1 < operand2)
				registers.regName.flags |= (uint16_t)flag_bit::LESS;
			else
				registers.regName.flags &= ~((uint16_t)flag_bit::LESS);
			if (operand1 > operand2)
				registers.regName.flags |= (uint16_t)flag_bit::GREATER;
			else
				registers.regName.flags &= ~((uint16_t)flag_bit::GREATER);
			if ((operand1 - operand2) == 0)
				registers.regName.flags |= (uint16_t)flag_bit::ZERO;
			else
				registers.regName.flags &= ~((uint16_t)flag_bit::ZERO);
			registers.regName.IP += 4;
			break;
		}
		case oasm::opcodes::POP:
			registers.regName.SP += 2;
		default:
			registers.regName.IP++;
			break;
		}
		if (shouldHalt || resetMachine)
			break;
	}
	if (isErrorHalt)
		printf("Exception thrown!%s\n", debug ? " Breaking into the debugger..." : "");
	if (debug && shouldHalt)
		debug_handler();
	if (resetMachine)
		goto reset;
#ifdef _WIN32
	SuspendThread(GetCurrentThread());
#else
	while (1) _mm_pause();
#endif
}

std::string regOpcodeToStr(oasm::byte regId)
{
	if (regId > MAX_REGISTER_VALUE)
		return "[bad]";
	const char* registers[] = {
		"R1", "R2", "R3", "R4",
		"R5", "R6", "R7", "R8",
		"SP", "IP", "FLAGS"
	};
	return registers[regId];
}
std::string conditionToStr(oasm::byte condition)
{
	switch (condition)
	{
	case 1:
		return "ZERO";
	case 2:
		return "LESS";
	case 4:
		return "GREATER";
	case 0xFE:
		return "NOT_ZERO";
	case 0xFD:
		return "NOT_LESS";
	case 0xFB:
		return "NOT_GREATER";
	default:
		return "[bad]";
	}
}

std::string disassemble(oasm::obarch::arch_uintptr IP, uint8_t* _instructionSize = nullptr)
{
	oasm::opcodes opcode = (oasm::opcodes)memory[IP];
	std::ostringstream str{ "" };
	str << std::hex;
	uint8_t instructionSize = 0;
	switch (opcode)
	{
	case oasm::opcodes::NOP:
		str << "nop";
		instructionSize = 1;
		break;
	case oasm::opcodes::ADD:
		str << "add " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::SUB:
		str << "sub " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::MOV_R16_IMM16:
		str << "mov " << regOpcodeToStr(memory[IP + 1]) << ", 0x" << static_cast<uint16_t>(memory[IP + 2] | (memory[IP + 3] << 8));
		instructionSize = 4;
		break;
	case oasm::opcodes::MOV_R16_R16:
		str << "mov " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::MOV_M16_R16:
		str << "mov " << '[' << (int)(memory[IP + 1] | (memory[IP + 2] << 8)) << "], " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 4;
		break;
	case oasm::opcodes::MOV_R16_M16:
		str << "mov " << regOpcodeToStr(memory[IP + 1]) << "[0x" << (int)(memory[IP + 2] | (memory[IP + 3] << 8)) << "], ";
		instructionSize = 4;
		break;
	case oasm::opcodes::JMP_IMM16:
		str << "jmp 0x" << (int)(memory[IP + 1] | (memory[IP + 2] << 8));
		instructionSize = 3;
		break;
	case oasm::opcodes::JMP_R16:
		str << "jmp " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::PUSH_R16:
		str << "push " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::POP_R16:
		str << "pop " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::CALL_IMM16:
		str << "call 0x" << (int)(memory[IP + 1] | (memory[IP + 2] << 8));
		instructionSize = 3;
		break;
	case oasm::opcodes::RET:
		str << "ret";
		instructionSize = 1;
		break;
	case oasm::opcodes::AND:
		str << "and " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::OR:
		str << "or " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::NOT:
		str << "not " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::HLTCLK:
		str << "hltclk";
		instructionSize = 1;
		break;
	case oasm::opcodes::CALL_R16:
		str << "call " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::POP:
		str << "pop";
		instructionSize = 2;
		break;
	case oasm::opcodes::OUT:
		str << "out " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::OUTC:
		str << "outc " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::JCC:
		str << "jcc " << conditionToStr(memory[IP + 1]) << " 0x" << (int)(memory[IP + 2] | (memory[IP + 2] << 3));
		instructionSize = 4;
		break;
	case oasm::opcodes::CMP_R16_R16:
		str << "cmp " << regOpcodeToStr(memory[IP + 1]) << ", " << regOpcodeToStr(memory[IP + 2]);
		instructionSize = 3;
		break;
	case oasm::opcodes::PUSH_IMM16:
		str << "push 0x" << static_cast<uint16_t>(memory[IP + 1] | (memory[IP + 2] << 8));
		instructionSize = 3;
		break;
	case oasm::opcodes::INC_R16:
		str << "inc " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::DEC_R16:
		str << "dec " << regOpcodeToStr(memory[IP + 1]);
		instructionSize = 2;
		break;
	case oasm::opcodes::CMP_R16_IMM16:
		str << "cmp " << regOpcodeToStr(memory[IP + 1]) << ", 0x" << static_cast<uint16_t>(memory[IP + 2] | (memory[IP + 3] << 8));
		instructionSize = 4;
		break;
	default:
		str << "[bad]";
		instructionSize = 1;
		break;
	}
	if (_instructionSize)
		*_instructionSize = instructionSize;
	return str.str();
}

enum class numType
{
	INVALID,
	DECIMAL = 10,
	HEX = 16,
};
bool isHexdigit(char ch);
bool strIsdigit(const std::string& str, numType& type);

void debug_handler()
{
	static bool isContinuing = false;
	static oasm::arch_uintptr continueUntil = 0;
	if (isContinuing && registers.regName.IP == continueUntil)
		isContinuing = false;
	while (!isContinuing)
	{
		std::string command = "";
		std::cin >> command;
		if (command == "dreg")
			std::cout << registers << '\n';
		else if (command == "disassemble")
		{
			size_t countInstructions = 0;
			std::cin >> countInstructions;
			uint8_t IP = registers.regName.IP;
			uint8_t instructionSize = 0;
			for (uint8_t i = 0; i < countInstructions; i++)
			{
				std::cout << "0x" << std::hex << (int)IP << std::dec << ": " << disassemble(IP, &instructionSize) << '\n';
				IP += instructionSize;
			}
		}
		else if (command == "c")
		{
			oasm::arch_uintptr address = 0;
			std::string addr = "";
			std::cin >> addr;
			numType type = numType::INVALID;
			if (strIsdigit(addr, type))
			{
				char* _unused = nullptr;
				address = static_cast<uint16_t>(std::strtol(addr.c_str(), &_unused, static_cast<int>(type)));
			}
			else
				continue;
			isContinuing = true;
			continueUntil = address;
		}
		else if (command == "s" || command.empty())
			break;
		else if (command == "q")
		{
			exit(0);
			break;
		}
		else if (command == "r")
		{
			resetMachine = true;
			break;
		}
		else if (command == "dd")
		{
			debug = false;
			break;
		}
		else if (command == "switchOutPmode")
		{
			printMode = !printMode;
		}
	}
}

bool isHexdigit(char ch)
{
	char hexDigits[] = "0123456789ABCDEFabcdef";
	for (int i = 0; i < sizeof(hexDigits); i++)
		if (ch == hexDigits[i])
			return true;
	return false;
}
bool strIsdigit(const std::string& str, numType& type)
{
	type = numType::INVALID;
	std::string _str = str;
	if (_str[0] == '0' && (_str[1] == 'x' || _str[1] == 'X'))
	{
		_str = _str.substr(2);
		for (const auto& i : _str)
			if (!isHexdigit(i))
				return false;
		type = numType::HEX;
	}
	else
	{
		for (const auto& i : _str)
			if (!isdigit(i))
				return false;
		type = numType::DECIMAL;
	}
	return true;
}