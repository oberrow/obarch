#include <stdio.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <string>

#include <filesystem>

#include <unordered_map>

#include "instruction_decode.h"
#include "typedef.h"

#include <cstring>

std::string& remove_trailing(std::string& str, char ch = ' ')
{
	str.erase(0, str.find_first_not_of(ch));
	return str;
}

#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#endif

namespace oasm
{
	int main(int argc, char** argv);
}

int main(int argc, char** argv)
{
	return oasm::main(argc, argv);
}

namespace oasm
{
	const char* g_instruction = nullptr;
	const char* g_filename = nullptr;
	size_t g_line = 0;
	bool g_error = false;
	std::unordered_map<std::string, label> g_labels;
	arch_uintptr g_address = 0;
	int main(int argc, char** argv)
	{
		if (argc == 1)
		{
			printf("Usage: %s [input_file] output_file", argv[0]);
			return 1;
		}

		std::string workingDir = std::filesystem::current_path().string();

		// Load the file into memory.
		std::stringstream memory{};
		g_filename = (argc == 2) ? "stdin" : argv[1];
		std::istream input{ std::cin.rdbuf() };
		std::ifstream file;
		if (strcmp(g_filename, "stdin") != 0)
		{
			file.open(g_filename);
			if (!file)
			{
				printf("%s doesn't exist.\n", g_filename);
				return 1;
			}
			input.rdbuf(file.rdbuf());
		}

		size_t nLines = 0;
		std::string ln = "";

		for (; std::getline(input, ln); nLines++)
			memory << ln << '\n';

		std::stringstream output{};

		for (; g_line < nLines; g_line++)
		{
			std::string _mneumonic = "";
			std::string par1 = "";
			std::string par2 = "";
			std::getline(memory, ln);
			remove_trailing(ln);
			remove_trailing(ln, '\t');
			if (ln.empty())
				continue;
			if (ln.front() == ';')
				continue;
			if (ln.find(':') != std::string::npos)
			{
				std::string _label = ln.substr(0, ln.length() - 1);
				if (isRegister(_label))
				{
					fprintf(stderr, "File %s, Line %lld: [Error] Label name is a register.\n", g_filename, g_line);
					g_error = true;
					continue;
				}
				label& l = g_labels[_label];
				l.address = g_address;
				l.is_unresolved = false;
				continue;
			}
			ln.push_back('\n');
			std::istringstream mem{ ln };
			mem >> _mneumonic;
			if (mem.peek() != '\n')
			{
				mem >> par1;
				par1 = par1.substr(0, par1.find_first_of(','));
			}
			if (mem.peek() != '\n')
				mem >> par2;
			if (_mneumonic == "unresolved")
			{
				if (par1.empty())
				{
					fprintf(stderr, "File %s, Line %lld: [Warning] No argument passed to \"unresolved\".\n", g_filename, g_line);
					continue;
				}
				label l;
				l.address = 0;
				l.is_unresolved = true;
				g_labels.try_emplace(par1, l);
				continue;
			}
			mneumonic mneumonic = oasm::mneumonic::INVALID;
		
			g_instruction = _mneumonic.c_str();
			if (!_stricmp(g_instruction, "NOP"))
				mneumonic = mneumonic::NOP;
			else if (!_stricmp(g_instruction, "MOV"))
				mneumonic = mneumonic::MOV;
			else if (!_stricmp(g_instruction, "ADD"))
				mneumonic = mneumonic::ADD;
			else if (!_stricmp(g_instruction, "SUB"))
				mneumonic = mneumonic::SUB;
			else if (!_stricmp(g_instruction, "JMP"))
				mneumonic = mneumonic::JMP;
			else if (!_stricmp(g_instruction, "RET"))
				mneumonic = mneumonic::RET;
			else if (!_stricmp(g_instruction, "PUSH"))
				mneumonic = mneumonic::PUSH;
			else if (!_stricmp(g_instruction, "POP"))
				mneumonic = mneumonic::POP;
			else if (!_stricmp(g_instruction, "CALL"))
				mneumonic = mneumonic::CALL;
			else if (!_stricmp(g_instruction, "AND"))
				mneumonic = mneumonic::AND;
			else if (!_stricmp(g_instruction, "OR"))
				mneumonic = mneumonic::OR;
			else if (!_stricmp(g_instruction, "NOT"))
				mneumonic = mneumonic::NOT;
			else if (!_stricmp(g_instruction, "HLTCLK"))
				mneumonic = mneumonic::HLTCLK;
			else if (!_stricmp(g_instruction, "OUT"))
				mneumonic = mneumonic::OUT;
			else if (!_stricmp(g_instruction, "OUTC"))
				mneumonic = mneumonic::OUTC;
			else if (!_stricmp(g_instruction, "JCC"))
				mneumonic = mneumonic::JCC;
			else if (!_stricmp(g_instruction, "CMP"))
				mneumonic = mneumonic::CMP;
			else if (!_stricmp(g_instruction, "INC"))
				mneumonic = mneumonic::INC;
			else if (!_stricmp(g_instruction, "DEC"))
				mneumonic = mneumonic::DEC;
			else
			{
				fprintf(stderr, "File %s, Line %lld: [Warning] Unrecoginized mneumonic %s.\n", g_filename, g_line, _mneumonic.c_str());
				continue;
			}
			if (par1.empty())
				g_address += oasm::decode(output, mneumonic);
			else if (par2.empty())
				g_address += oasm::decode(output, mneumonic, par1);
			else
				g_address += oasm::decode(output, mneumonic, par1, par2);
		}

		const char* outputFilename = argc == 2 ? argv[1] : argv[2];
		std::ofstream outputFile{ outputFilename, std::ios::binary | std::ios::trunc };

		if (!g_error)
		{
			for (auto& [key, data] : g_labels)
			{
				if (data.is_unresolved && data.references.size())
				{
					fprintf(stderr, "File %s, Line %lld: [Error] Unresolved symbol \"%s\" referenced %lld time%s in %s.\n", g_filename, g_line,
						key.c_str(),
						data.references.size(),
						data.references.size() == 1 ? "" : "s",
						g_filename);
					g_error = true;
				}
			}
			if (g_error)
				return 1;
			// Flush the changes to disk.
			std::string changes = output.str();
			byte address = 0;
			bool skipNextByte = false;
			for (const auto ch : changes)
			{
				if (skipNextByte)
				{
					address++;
					skipNextByte = false;
					continue;
				}
				label* l = nullptr;
				for (auto& [key, data] : g_labels)
				{
					for (auto& reference : data.references)
					{
						if (address == reference)
						{
							l = &data;
							break;
						}
					}
					if (l)
						break;
				}
				if(!l)
					outputFile.put(ch);
				else
				{
					outputFile.put(l->address & 0xFF);
					outputFile.put(l->address >> 8);
					skipNextByte = true;
				}
				address++;
			}
			outputFile.flush();
			outputFile.close();
		}
		else
			return 1;

		return 0;
	}
}