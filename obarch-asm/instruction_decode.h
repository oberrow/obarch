#pragma once

#include <string>

#include <ostream>

#include "typedef.h"

namespace oasm
{
	arch_uintptr decode(std::ostream& output, mneumonic _mneumonic);
	arch_uintptr decode(std::ostream& output, mneumonic _mneumonic, const std::string& par1);
	arch_uintptr decode(std::ostream& output, mneumonic _mneumonic, const std::string& par1, const std::string& par2);

	bool isRegister(const std::string& str);
}