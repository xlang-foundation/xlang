#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace X::Tools
{
	int RunProcess(const std::filesystem::path& executable,
		const std::vector<std::string>& arguments,
		const std::filesystem::path& workingDirectory = {});
}
