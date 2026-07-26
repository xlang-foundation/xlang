#include "PythonProcess.h"

#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{
#ifdef _WIN32
	std::wstring Utf8ToWide(const std::string& value)
	{
		if (value.empty())
			return {};
		const int count = MultiByteToWideChar(
			CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
		std::wstring result(static_cast<size_t>(count), L'\0');
		MultiByteToWideChar(
			CP_UTF8, 0, value.c_str(), -1, result.data(), count);
		result.pop_back();
		return result;
	}

	std::wstring QuoteArgument(const std::wstring& argument)
	{
		if (argument.find_first_of(L" \t\n\v\"") == std::wstring::npos)
			return argument;

		std::wstring quoted = L"\"";
		size_t slashes = 0;
		for (wchar_t ch : argument)
		{
			if (ch == L'\\')
			{
				++slashes;
			}
			else if (ch == L'"')
			{
				quoted.append(slashes * 2 + 1, L'\\');
				quoted.push_back(ch);
				slashes = 0;
			}
			else
			{
				quoted.append(slashes, L'\\');
				quoted.push_back(ch);
				slashes = 0;
			}
		}
		quoted.append(slashes * 2, L'\\');
		quoted.push_back(L'"');
		return quoted;
	}
#endif
}

int X::Tools::RunProcess(const std::filesystem::path& executable,
	const std::vector<std::string>& arguments,
	const std::filesystem::path& workingDirectory)
{
#ifdef _WIN32
	std::wstring commandLine = QuoteArgument(executable.wstring());
	for (const auto& argument : arguments)
	{
		commandLine.push_back(L' ');
		commandLine += QuoteArgument(Utf8ToWide(argument));
	}
	std::vector<wchar_t> mutableCommand(
		commandLine.begin(), commandLine.end());
	mutableCommand.push_back(L'\0');

	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	const std::wstring workDir = workingDirectory.wstring();
	if (!CreateProcessW(executable.c_str(), mutableCommand.data(), nullptr,
		nullptr, TRUE, 0, nullptr,
		workDir.empty() ? nullptr : workDir.c_str(), &startup, &process))
	{
		std::cerr << "Unable to start " << executable.string()
			<< " (Windows error " << GetLastError() << ")" << std::endl;
		return 1;
	}

	WaitForSingleObject(process.hProcess, INFINITE);
	DWORD exitCode = 1;
	GetExitCodeProcess(process.hProcess, &exitCode);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return static_cast<int>(exitCode);
#else
	const pid_t child = fork();
	if (child < 0)
		return 1;
	if (child == 0)
	{
		if (!workingDirectory.empty())
			chdir(workingDirectory.c_str());
		std::vector<std::string> values;
		values.reserve(arguments.size() + 1);
		values.push_back(executable.string());
		values.insert(values.end(), arguments.begin(), arguments.end());
		std::vector<char*> argv;
		argv.reserve(values.size() + 1);
		for (auto& value : values)
			argv.push_back(value.data());
		argv.push_back(nullptr);
		execv(executable.c_str(), argv.data());
		_exit(127);
	}
	int status = 0;
	if (waitpid(child, &status, 0) < 0)
		return 1;
	return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif
}
