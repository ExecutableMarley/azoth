/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <string>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>

#include "../../EPlatformError.hpp"
#include "../../../Types/EProcessArchitecture.hpp"
#include "../../../Types/EProcessPrivilegeLevel.hpp"

#include "../../../Core/ProcessImage.hpp"


namespace Azoth
{


PlatformErrorState sendSignal(pid_t pid, int signal);


EPlatformError getProcessMappedBinaries(uint32_t pid, std::unordered_map<std::string, ProcessImage>& outBinaries);


}