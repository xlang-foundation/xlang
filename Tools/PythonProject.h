#pragma once

#include <filesystem>
#include <string>

namespace X::Tools
{
	struct PythonProjectOptions
	{
		std::filesystem::path projectDirectory;
		std::filesystem::path toolsDirectory;
		// Empty means: use the exact Python version recorded beside the
		// packaged ABI-matched PyEng binary.
		std::string version;
		bool installPip = true;
	};

	int InitializePythonProject(const PythonProjectOptions& options);
	std::filesystem::path FindProjectPython(
		const std::filesystem::path& startDirectory);
}
