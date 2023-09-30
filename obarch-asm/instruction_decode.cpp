#include "instruction_decode.h"

#include <vector>
#include <unordered_map>

#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#endif

namespace oasm
{
	extern const char* g_instruction;
	extern const char* g_filename;
	extern size_t g_line;
	extern bool g_error;
	extern std::unordered_map<std::string, label> g_labels;
	extern arch_uintptr g_address;

	byte regstrToOpcode(const std::string& _regName)
	{
		const char* regName = _regName.c_str();
		byte ret = 0xff;
		if (!_stricmp(regName, "R1"))
			ret = 0;
		else if (!_stricmp(regName, "R2"))
			ret = 1;
		else if (!_stricmp(regName, "R3"))
			ret = 2;
		else if (!_stricmp(regName, "R4"))
			ret = 3;
		else if (!_stricmp(regName, "R5"))
			ret = 4;
		else if (!_stricmp(regName, "R6"))
			ret = 5;
		else if (!_stricmp(regName, "R7"))
			ret = 6;
		else if (!_stricmp(regName, "R8"))
			ret = 7;
		else if (!_stricmp(regName, "SP"))
			ret = 8;
		else if (!_stricmp(regName, "IP"))
			ret = 9;
		else if (!_stricmp(regName, "FLAGS"))
			ret = 10;
		
		return ret;
	}
	bool strIsalpha(const std::string& str)
	{
		for (const auto& i : str)
			if (!isalpha(i))
				return false;
		return true;
	}
	bool strIsalpunc(const std::string& str)
	{
		for (const auto& i : str)
			if (!isalpha(i) && !ispunct(i))
				return false;
		return true;
	}
	enum class numType
	{
		INVALID,
		DECIMAL = 10,
		HEX = 16,
	};
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

	arch_uintptr decode(std::ostream& output, mneumonic _mnemonic)
	{
		switch (_mnemonic)
		{
		case mneumonic::NOP:
			if (!g_error)
				output.put('\x00');
			break;
		case mneumonic::HLTCLK:
			if (!g_error)
				output.put('\x10');
			break;
		case mneumonic::RET:
			if (!g_error)
				output.put('\x0C');
			break;
		case mneumonic::POP:
			if (!g_error)
				output.put('\x15');
			break;
		default:
			g_error = true;
			fprintf(stderr, "File %s, Line %lld: [Error] Invalid amount of parameters for mnemonic %s\n", g_filename, g_line, g_instruction);
			break;
		}
		return 1;
	}
	arch_uintptr decode(std::ostream& output, mneumonic _mnemonic, const std::string& par1)
	{
		arch_uintptr ret = 2;
		switch (_mnemonic)
		{
		case mneumonic::JMP:
		{
			byte opcode = 0;
			byte operand = 0;
			numType type = numType::DECIMAL;
			ret = 3;
			if (strIsdigit(par1, type))
			{
				char* _unused = nullptr;
				operand = static_cast<char>(std::strtol(par1.c_str(), &_unused, static_cast<int>(type)));
				opcode = 0x07;
			}
			else
			{
				opcode = 0x08;
				operand = regstrToOpcode(par1);
				if (operand == 0xFF)
				{
					opcode = 0x07;
					if(!g_labels.contains(par1) && par1 != "$")
					{
						fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
						g_error = true;
						return 0;
					}
					if (par1 != "$")
					{
						label& label = g_labels.at(par1);
						operand = label.address;
						if(label.is_unresolved)
							label.references.push_back(g_address + 1);
					}
					else
						operand = g_address;
				}
				else
					ret = 2;
			}
			if (!g_error)
			{
				output.put(opcode);
				output.put(operand & 0xFF);
				if(opcode != 0x08)
					output.put(operand >> 8);
			}
			break;
		}
		case mneumonic::CALL:
		{
			byte opcode = 0x0B;
			uint16_t operand = 0;
			numType type = numType::DECIMAL;
			if (isRegister(par1))
			{
				opcode = 0x14;
				operand = regstrToOpcode(par1);
				ret = 2;
			}
			else
			{
				ret = 3;
				numType type = numType::INVALID;
				if (strIsdigit(par1, type))
				{
					char* _unused = nullptr;
					operand = static_cast<char>(std::strtol(par1.c_str(), &_unused, static_cast<int>(type)));
				}
				if(type == numType::INVALID)
				{
					if (!g_labels.contains(par1) && par1 != "$")
					{
						fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
						g_error = true;
						return 0;
					}
					if (par1 != "$")
					{
						label& label = g_labels.at(par1);
						operand = label.address;
						if (label.is_unresolved)
							label.references.push_back(g_address + 1);
					}
					else
						operand = static_cast<arch_uintptr>(g_address) + 1 + sizeof(arch_uintptr);
				}
			}
			if (!g_error)
			{
				output.put(opcode);
				output.put(operand & 0xFF);
				if (opcode != 0x14)
					output.put(operand >> 8);
			}
			break;
		}
		case mneumonic::NOT:
		{
			byte operand1 = regstrToOpcode(par1);
			if (operand1 == 0xFF)
			{
				fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
				g_error = true;
			}
			if (!g_error)
			{
				output.put('\x0F');
				output.put(operand1);
			}
			break;
		}
		case mneumonic::OUT:
		{
			byte operand1 = regstrToOpcode(par1);
			if (operand1 == 0xFF)
			{
				fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
				g_error = true;
			}
			if (!g_error)
			{
				output.put('\x16');
				output.put(operand1);
			}
			break;
		}
		case mneumonic::OUTC:
		{
			byte operand1 = regstrToOpcode(par1);
			if (operand1 == 0xFF)
			{
				fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
				g_error = true;
			}
			if (!g_error)
			{
				output.put('\x17');
				output.put(operand1);
			}
			break;
		}
		case mneumonic::PUSH:
		{
			byte opcode = 0x09;
			uint16_t operand = 0;
			if (isRegister(par1))
				operand = regstrToOpcode(par1);
			else
			{
				opcode = 0x1A;
				numType type = numType::INVALID;
				if (strIsdigit(par1, type))
				{
					char* _unused = nullptr;
					operand = static_cast<uint16_t>(std::strtol(par1.c_str(), &_unused, static_cast<int>(type)));
				}
				if (type == numType::INVALID)
				{
					if (!g_labels.contains(par1) && par1 != "$")
					{
						fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
						g_error = true;
						return 0;
					}
					if (par1 != "$")
					{
						label& label = g_labels.at(par1);
						operand = label.address;
						if (label.is_unresolved)
							label.references.push_back(g_address + 1);
					}
					else
						operand = g_address + 1 + sizeof(arch_uintptr);
				}
				ret = 3;
			}
			output.put(opcode);
			output.put(operand & 0xFF);
			if(opcode == 0x1A)
				output.put(operand >> 8);				
			break;
		}
		case mneumonic::POP:
		{
			byte operand1 = regstrToOpcode(par1);
			if (operand1 == 0xFF)
			{
				fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
				g_error = true;
			}
			if (!g_error)
			{
				output.put('\x0A');
				output.put(operand1);
			}
			break;
		}
		case mneumonic::INC:
		{
			byte operand1 = regstrToOpcode(par1);
			if (operand1 == 0xFF)
			{
				fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
				g_error = true;
			}
			if (!g_error)
			{
				output.put('\x1B');
				output.put(operand1);
			}
			break;
		}
		case mneumonic::DEC:
		{
			byte operand1 = regstrToOpcode(par1);
			if (operand1 == 0xFF)
			{
				fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
				g_error = true;
			}
			if (!g_error)
			{
				output.put('\x1C');
				output.put(operand1);
			}
			break;
		}
		default:
			g_error = true;
			fprintf(stderr, "File %s, Line %lld: [Error] Invalid amount of operand for mneumonic %s\n", g_filename, g_line, g_instruction);
			break;
		}
		return ret;
	}
	static void process_double_r16(std::ostream& output, byte opcode, const std::string& par1, const std::string& par2)
	{
		byte operand1 = regstrToOpcode(par1);
		byte operand2 = regstrToOpcode(par2);
		if (operand1 == 0xFF)
		{
			fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par1.c_str());
			g_error = true;
		}
		if (operand2 == 0xFF)
		{
			fprintf(stderr, "File %s, Line %lld: [Error] %s is not a register.\n", g_filename, g_line, par2.c_str());
			g_error = true;
		}
		if(!g_error)
		{
			output << opcode;
			output << operand1;
			output << operand2;
		}
	}
	arch_uintptr decode(std::ostream& output, mneumonic _mnemonic, const std::string& par1, const std::string& par2)
	{
		arch_uintptr ret = 3;
		switch (_mnemonic)
		{
		case mneumonic::ADD:
			process_double_r16(output, 0x01, par1, par2);
			break;
		case mneumonic::SUB:
			process_double_r16(output, 0x02, par1, par2);
			break;
		case mneumonic::AND:
			process_double_r16(output, 0x0D, par1, par2);
			break;
		case mneumonic::OR:
			process_double_r16(output, 0x0E, par1, par2);
			break;
		case mneumonic::CMP:
		{
			if (isRegister(par1) && isRegister(par2))
			{
				process_double_r16(output, 0x19, par1, par2);
				break;
			}
			if (!isRegister(par1))
			{
				fprintf(stderr, "File %s, Line %lld: [Error] Operand 1 must be a register.\n", g_filename, g_line);
				g_error = true;
				return 0;
			}
			uint8_t operand1 = regstrToOpcode(par1);
			uint16_t operand2 = 0;
			numType type = numType::INVALID;
			if (strIsdigit(par2, type))
			{
				char* _unused = nullptr;
				operand2 = static_cast<uint16_t>(std::strtol(par2.c_str(), &_unused, static_cast<int>(type)));
			}
			if (type == numType::INVALID)
			{
				if (!g_labels.contains(par2) && par2 != "$")
				{
					fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
					g_error = true;
					return 0;
				}
				if (par2 != "$")
				{
					label& label = g_labels.at(par2);
					operand2 = label.address;
					if (label.is_unresolved)
						label.references.push_back(g_address + 1);
				}
				else
					operand2 = g_address + 1 + sizeof(arch_uintptr);
			}
			output.put(0x1D);
			output.put(operand1);
			output.put(operand2 & 0xFF);
			output.put(operand2 >> 8);
			break;
		}
		case mneumonic::MOV:
		{
			ret = 4;
			if (isRegister(par1) && isRegister(par2))
			{
				byte operand1 = regstrToOpcode(par1);
				byte operand2 = regstrToOpcode(par2);
				process_double_r16(output, 0x04, par1, par2);
				ret = 3;
				break;
			}
			else if (isRegister(par1) && !isRegister(par2))
			{
				if (par2.front() == '[')
				{
					if (par2.back() != ']')
					{
						fprintf(stderr, "File %s, Line %lld: [Error] Missing closing ']'.\n", g_filename, g_line);
						g_error = true;
						return 0;
					}
					std::string _address = par2.substr(1, par2.length() - 1);
					arch_uintptr operand = 0;
					numType type = numType::DECIMAL;
					if (strIsalpunc(_address))
					{
						if (!g_labels.contains(par2) && par2 != "$")
						{
							fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
							g_error = true;
							return 0;
						}
						if (par1 != "$")
						{
							label& label = g_labels.at(par2);
							operand = label.address;
							if (label.is_unresolved)
								label.references.push_back(g_address + 2);
						}
						else
							operand = g_address;
					}
					else if (strIsdigit(_address, type))
					{
						char* _unused = nullptr;
						operand = static_cast<byte>(std::strtol(_address.c_str(), &_unused, (int)type));
					}
					byte regOpcode = regstrToOpcode(par1);
					output.put(0x06);
					output.put(regOpcode);
					output.put(operand & 0xFF);
					output.put(operand >> 8);
				}
				else
				{
					numType type = numType::INVALID;
					uint16_t operand = 0;
					
					if (strIsalpunc(par2))
					{
						if (!g_labels.contains(par2) && par2 != "$")
						{
							fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
							g_error = true;
							return 0;
						}
						if (par2 != "$")
						{
							label& label = g_labels.at(par2);
							operand = label.address;
							if (label.is_unresolved)
								label.references.push_back(g_address + 2);
						}
						else
							operand = g_address;
					}
					else if (strIsdigit(par2, type))
					{
						char* _unused = nullptr;
						operand = static_cast<uint16_t>(std::strtol(par2.c_str(), &_unused, (int)type));
					}
					char* _unused = nullptr;
					byte regOpcode = regstrToOpcode(par1);
					output.put(0x03);
					output.put(regstrToOpcode(par1));
					output.put(operand & 0xFF);
					output.put(static_cast<char>(operand >> 8));
				}
			}
			else if (isRegister(par2) && !isRegister(par1))
			{
				std::string _address = par2;
				if (_address.front() == '[')
					_address = _address.substr(1);
				if (_address.back() == ']')
					_address = _address.substr(0, _address.length() - 1);
				arch_uintptr operand = 0;
				numType type = numType::DECIMAL;
				if (strIsalpunc(_address))
				{
					if (!g_labels.contains(par2) && par2 != "$")
					{
						fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
						g_error = true;
						return 0;
					}
					if (par1 != "$")
					{
						label& label = g_labels.at(par2);
						operand = label.address;
						if (label.is_unresolved)
							label.references.push_back(g_address + 1);
					}
					else
						operand = g_address;
				}
				else if (strIsdigit(_address, type))
				{
					char* _unused = nullptr;
					operand = static_cast<byte>(std::strtol(_address.c_str(), &_unused, (int)type));
				}
				output.put(0x05);
				output.put(operand & 0xff);
				output.put(operand >> 8);
				output.put(regstrToOpcode(par2));
			}
			else
			{
				fprintf(stderr, "File %s, Line %lld: [Error] Invalid operands for mnemonic %s\n", g_filename, g_line, g_instruction);
				g_error = true;
				break;
			}
			break;
		}
		case mneumonic::JCC:
		{
			arch_uintptr operand1 = 0;
			arch_uintptr operand2 = 0;
			numType type = numType::INVALID;
			const char* _operand1 = par1.c_str();
			if (_stricmp(_operand1, "ZERO") == 0 || _stricmp(_operand1, "EQUAL") == 0)
				operand1 = 0x01;
			else if (_stricmp(_operand1, "NOT_ZERO") == 0 || _stricmp(_operand1, "NOT_EQUAL") == 0)
				operand1 = 0xFE;
			else if (_stricmp(_operand1, "LESS") == 0)
				operand1 = 0x02;
			else if (_stricmp(_operand1, "NOT_LESS") == 0)
				operand1 = 0xFD;
			else if (_stricmp(_operand1, "GREATER") == 0)
				operand1 = 0x04;
			else if (_stricmp(_operand1, "NOT_GREATER") == 0)
				operand1 = 0xFB;
			else
			{
				fprintf(stderr, "File %s, Line %lld: [Error] Unknown condition %s.\n", g_filename, g_line, _operand1);
				g_error = true;
				return 0;
			}
			type = numType::INVALID;
			if (strIsdigit(par2, type))
			{
				char* _unused = nullptr;
				operand2 = static_cast<char>(std::strtol(par2.c_str(), &_unused, static_cast<int>(type)));
			}
			else
			{
				if (!g_labels.contains(par2) && par2 != "$")
				{
					fprintf(stderr, "File %s, Line %lld: [Error] Label %s doesn't exist.\n", g_filename, g_line, par1.c_str());
					g_error = true;
					return 0;
				}
				if (par2 != "$")
				{
					label& label = g_labels.at(par2);
					operand2 = label.address;
					if (label.is_unresolved)
						label.references.push_back(g_address + 1);
				}
				else
					operand2 = g_address;
			}
			output.put('\x18');
			output.put(operand1);
			output.put(operand2 & 0xff);
			output.put(operand2 >> 8);
			ret = 4;
			break;
		}
		default:
			fprintf(stderr, "File %s, Line %lld: [Error] Invalid amount of operands for mnemonic %s\n", g_filename, g_line, g_instruction);
			g_error = true;
			break;
		}
		return ret;
	}


	bool isRegister(const std::string& str)
	{
		const char* regNames[] =
		{
			"R1",
			"R2",
			"R3",
			"R4",
			"R5",
			"R6",
			"R7",
			"R8",
			"SP",
			"IP",
			"FLAGS",
		};
		for (int i = 0; i < sizeof(regNames) / sizeof(regNames[0]); i++)
			if (str == regNames[i])
				return true;
		return false;
	}
}
