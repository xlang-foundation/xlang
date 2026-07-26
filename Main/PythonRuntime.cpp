/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
*/

#include "PythonRuntime.h"

#include <array>
#include <filesystem>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{
	std::vector<std::string> SplitLines(const std::string& text)
	{
		std::vector<std::string> lines;
		std::istringstream stream(text);
		std::string line;
		while (std::getline(stream, line))
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			lines.push_back(line);
		}
		return lines;
	}

#if defined(_WIN32)
	std::string QuoteWindowsArgument(const std::string& argument)
	{
		if (argument.empty())
		{
			return "\"\"";
		}
		if (argument.find_first_of(" \t\n\v\"") == std::string::npos)
		{
			return argument;
		}

		std::string quoted;
		quoted.push_back('"');
		size_t backslashes = 0;
		for (char ch : argument)
		{
			if (ch == '\\')
			{
				++backslashes;
				continue;
			}
			if (ch == '"')
			{
				quoted.append(backslashes * 2 + 1, '\\');
				quoted.push_back('"');
				backslashes = 0;
				continue;
			}
			quoted.append(backslashes, '\\');
			backslashes = 0;
			quoted.push_back(ch);
		}
		quoted.append(backslashes * 2, '\\');
		quoted.push_back('"');
		return quoted;
	}
#endif

	bool RunPythonProbe(const std::string& pythonExecutable,
		const std::string& code,
		std::string& output,
		std::string& error)
	{
#if defined(_WIN32)
		SECURITY_ATTRIBUTES security{};
		security.nLength = sizeof(security);
		security.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &security, 0))
		{
			error = "CreatePipe failed";
			return false;
		}
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOA startup{};
		PROCESS_INFORMATION process{};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.hStdOutput = writePipe;
		startup.hStdError = writePipe;
		startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

		std::string commandLine =
			QuoteWindowsArgument(pythonExecutable) + " -c " +
			QuoteWindowsArgument(code);
		std::vector<char> writableCommand(commandLine.begin(), commandLine.end());
		writableCommand.push_back('\0');

		const char* applicationName =
			std::filesystem::path(pythonExecutable).is_absolute()
			? pythonExecutable.c_str()
			: nullptr;

		BOOL started = CreateProcessA(
			applicationName,
			writableCommand.data(),
			nullptr,
			nullptr,
			TRUE,
			CREATE_NO_WINDOW,
			nullptr,
			nullptr,
			&startup,
			&process);
		CloseHandle(writePipe);
		if (!started)
		{
			CloseHandle(readPipe);
			error = "Unable to start Python executable (error " +
				std::to_string(GetLastError()) + ")";
			return false;
		}

		std::array<char, 512> buffer{};
		DWORD bytesRead = 0;
		while (ReadFile(readPipe, buffer.data(),
			static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) &&
			bytesRead > 0)
		{
			output.append(buffer.data(), bytesRead);
		}
		CloseHandle(readPipe);

		WaitForSingleObject(process.hProcess, INFINITE);
		DWORD exitCode = 1;
		GetExitCodeProcess(process.hProcess, &exitCode);
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		if (exitCode != 0)
		{
			error = output.empty() ? "Python runtime probe failed" : output;
			return false;
		}
		return true;
#else
		int pipeFds[2];
		if (pipe(pipeFds) != 0)
		{
			error = "pipe failed";
			return false;
		}

		pid_t pid = fork();
		if (pid == -1)
		{
			close(pipeFds[0]);
			close(pipeFds[1]);
			error = "fork failed";
			return false;
		}
		if (pid == 0)
		{
			dup2(pipeFds[1], STDOUT_FILENO);
			dup2(pipeFds[1], STDERR_FILENO);
			close(pipeFds[0]);
			close(pipeFds[1]);
			if (pythonExecutable.find('/') != std::string::npos)
			{
				execl(pythonExecutable.c_str(), pythonExecutable.c_str(),
					"-c", code.c_str(), static_cast<char*>(nullptr));
			}
			else
			{
				execlp(pythonExecutable.c_str(), pythonExecutable.c_str(),
					"-c", code.c_str(), static_cast<char*>(nullptr));
			}
			_exit(127);
		}

		close(pipeFds[1]);
		std::array<char, 512> buffer{};
		ssize_t count = 0;
		while ((count = read(pipeFds[0], buffer.data(), buffer.size())) > 0)
		{
			output.append(buffer.data(), static_cast<size_t>(count));
		}
		close(pipeFds[0]);

		int status = 0;
		waitpid(pid, &status, 0);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
			error = output.empty() ? "Python runtime probe failed" : output;
			return false;
		}
		return true;
#endif
	}
}

bool query_python_runtime(const std::string& pythonExecutable,
	PythonRuntimeInfo& runtime,
	std::string& error)
{
	const std::string executable =
		pythonExecutable.empty()
#if defined(_WIN32)
		? "python"
#else
		? "python3"
#endif
		: pythonExecutable;

	const char* probe =
		"import os,sys,sysconfig\n"
		"print(os.path.abspath(sys.executable))\n"
		"print(os.path.abspath(sys.base_prefix))\n"
		"print(f'cp{sys.version_info.major}{sys.version_info.minor}')\n"
		"if os.name == 'nt':\n"
		" print(os.path.join(sys.base_prefix, "
		"f'python{sys.version_info.major}{sys.version_info.minor}.dll'))\n"
		"else:\n"
		" print(os.path.join(sysconfig.get_config_var('LIBDIR') or '', "
		"sysconfig.get_config_var('LDLIBRARY') or ''))\n";

	std::string output;
	if (!RunPythonProbe(executable, probe, output, error))
	{
		return false;
	}

	auto lines = SplitLines(output);
	if (lines.size() < 4 || lines[0].empty() || lines[1].empty() ||
		lines[2].empty())
	{
		error = "Python runtime probe returned incomplete information";
		return false;
	}

	runtime.executable = lines[0];
	runtime.home = lines[1];
	runtime.abiTag = lines[2];
	runtime.runtimeLibrary = lines[3];

	if (!pythonExecutable.empty() &&
		!std::filesystem::exists(runtime.executable))
	{
		error = "Configured Python executable does not exist: " +
			runtime.executable;
		return false;
	}
	if (runtime.runtimeLibrary.empty() ||
		!std::filesystem::exists(runtime.runtimeLibrary))
	{
		error = "Python runtime library does not exist: " +
			runtime.runtimeLibrary;
		return false;
	}
	return true;
}
