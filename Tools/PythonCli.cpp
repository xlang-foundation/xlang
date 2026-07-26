#include "PythonCli.h"
#include "PythonProcess.h"
#include "PythonProject.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
	void PrintPythonCliUsage()
	{
		std::cout <<
			"xlang init python [PROJECT_PATH] [--version X.Y.Z] [--without-pip]\n"
			"xlang python [--project PATH] [python arguments]\n"
			"xlang pip [--project PATH] <pip arguments>\n";
	}
}

bool X::Tools::TryRunPythonCli(int argc, char* argv[], int& exitCode)
{
	exitCode = 0;
	if (argc < 2)
		return false;

	const std::string command = argv[1];
	if (command == "init")
	{
		if (argc < 3 || std::string(argv[2]) != "python")
			return false;

		PythonProjectOptions options;
		options.projectDirectory = std::filesystem::current_path();
		options.toolsDirectory =
			std::filesystem::absolute(argv[0]).parent_path();
		bool projectWasSet = false;
		for (int index = 3; index < argc; ++index)
		{
			const std::string argument = argv[index];
			if (argument == "--version" && index + 1 < argc)
				options.version = argv[++index];
			else if (argument == "--project" && index + 1 < argc)
			{
				options.projectDirectory = argv[++index];
				projectWasSet = true;
			}
			else if (argument == "--without-pip")
				options.installPip = false;
			else if (!argument.empty() && argument.front() != '-' &&
				!projectWasSet)
			{
				options.projectDirectory = argument;
				projectWasSet = true;
			}
			else
			{
				std::cerr << "Unknown init python option: "
					<< argument << std::endl;
				PrintPythonCliUsage();
				exitCode = 2;
				return true;
			}
		}
		exitCode = InitializePythonProject(options);
		return true;
	}

	if (command == "python")
	{
		std::filesystem::path project = std::filesystem::current_path();
		int firstPythonArgument = 2;
		if (argc >= 4 && std::string(argv[2]) == "--project")
		{
			project = argv[3];
			firstPythonArgument = 4;
		}

		const auto python = FindProjectPython(project);
		if (python.empty())
		{
			std::cerr << "This folder is not an initialized XLang Python "
				"project. Run 'xlang init python' here first." << std::endl;
			exitCode = 2;
			return true;
		}

		std::vector<std::string> pythonArguments;
		for (int index = firstPythonArgument; index < argc; ++index)
			pythonArguments.emplace_back(argv[index]);
		exitCode = RunProcess(python, pythonArguments, project);
		return true;
	}

	if (command != "pip" && command != "-pip")
		return false;

	std::filesystem::path project = std::filesystem::current_path();
	std::vector<std::string> pipArguments;
	for (int index = 2; index < argc; ++index)
	{
		const std::string argument = argv[index];
		if (argument == "--project" && index + 1 < argc)
			project = argv[++index];
		else
			pipArguments.push_back(argument);
	}
	if (pipArguments.empty())
	{
		PrintPythonCliUsage();
		exitCode = 2;
		return true;
	}

	const auto python = FindProjectPython(project);
	if (python.empty())
	{
		std::cerr << "This folder is not an initialized XLang Python "
			"project. Run 'xlang init python' here first." << std::endl;
		exitCode = 2;
		return true;
	}
	std::vector<std::string> arguments{"-m", "pip"};
	arguments.insert(
		arguments.end(), pipArguments.begin(), pipArguments.end());
	exitCode = RunProcess(python, arguments, project);
	return true;
}
