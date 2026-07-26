#include "PythonProject.h"
#include "PythonProcess.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#include <urlmon.h>
#endif

namespace
{
	bool IsValidVersion(const std::string& version)
	{
		if (version.empty() || version.front() == '.' || version.back() == '.')
			return false;
		for (char ch : version)
		{
			if (!std::isdigit(static_cast<unsigned char>(ch)) && ch != '.')
				return false;
		}
		return version.find('.') != std::string::npos;
	}

	std::string AbiDigits(const std::string& version)
	{
		std::istringstream input(version);
		std::string major;
		std::string minor;
		std::getline(input, major, '.');
		std::getline(input, minor, '.');
		return major + minor;
	}

	std::string FindBundledPythonVersion(
		const std::filesystem::path& toolsDirectory)
	{
		std::error_code error;
		std::string selected;
		for (const auto& entry :
			std::filesystem::directory_iterator(toolsDirectory, error))
		{
			if (error || !entry.is_regular_file())
				continue;
			const std::string name = entry.path().filename().string();
			constexpr const char* prefix = "pyeng.cp";
			constexpr const char* suffix = ".version.txt";
			if (name.rfind(prefix, 0) != 0 ||
				name.size() <= std::strlen(prefix) + std::strlen(suffix) ||
				name.compare(name.size() - std::strlen(suffix),
					std::strlen(suffix), suffix) != 0)
				continue;

			const std::string abi = name.substr(
				std::strlen(prefix),
				name.size() - std::strlen(prefix) - std::strlen(suffix));
			const auto bridge =
				toolsDirectory / ("pyeng.cp" + abi + ".dll");
			if (!std::filesystem::is_regular_file(bridge))
				continue;

			std::ifstream input(entry.path());
			std::string version;
			std::getline(input, version);
			if (IsValidVersion(version) && version > selected)
				selected = version;
		}
		return selected;
	}

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

	bool Download(const std::string& url,
		const std::filesystem::path& destination)
	{
		std::cout << "Downloading " << url << std::endl;
		const std::wstring wideUrl = Utf8ToWide(url);
		const HRESULT result = URLDownloadToFileW(
			nullptr, wideUrl.c_str(), destination.c_str(), 0, nullptr);
		if (FAILED(result))
		{
			std::cerr << "Download failed (HRESULT 0x" << std::hex
				<< static_cast<unsigned long>(result) << std::dec << ")"
				<< std::endl;
			return false;
		}
		return true;
	}

	bool EnableSitePackages(const std::filesystem::path& runtime,
		const std::string& version)
	{
		const auto pth = runtime / ("python" + AbiDigits(version) + "._pth");
		std::ifstream input(pth);
		if (!input)
		{
			std::cerr << "The Python distribution has no "
				<< pth.filename().string() << std::endl;
			return false;
		}
		std::ostringstream current;
		current << input.rdbuf();
		std::string text = current.str();
		if (text.find("Lib/site-packages") == std::string::npos)
			text += "\nLib/site-packages\n";
		const auto importSite = text.find("#import site");
		if (importSite != std::string::npos)
			text.replace(importSite, 12, "import site");
		else if (text.find("import site") == std::string::npos)
			text += "import site\n";
		std::ofstream output(pth, std::ios::trunc);
		output << text;
		return output.good();
	}

	bool ConfigureXLangBridge(const std::filesystem::path& runtime,
		const std::filesystem::path& bridgeSource,
		const std::filesystem::path& toolsDirectory,
		std::string& errorMessage)
	{
		const auto sitePackages = runtime / "Lib" / "site-packages";
		std::error_code error;
		std::filesystem::create_directories(sitePackages, error);
		if (error)
		{
			errorMessage = "Could not create private site-packages: " +
				error.message();
			return false;
		}

		const auto bridgeDestination = sitePackages / "xlang.pyd";
		std::filesystem::copy_file(bridgeSource, bridgeDestination,
			std::filesystem::copy_options::overwrite_existing, error);
		if (error)
		{
			errorMessage = "Could not install xlang.pyd: " + error.message();
			return false;
		}

		const auto engineRecord = sitePackages / "xlang_engine_path.txt";
		std::ofstream output(engineRecord, std::ios::trunc);
		output << std::filesystem::absolute(toolsDirectory).string() << "\n";
		output.close();
		if (output.fail())
		{
			errorMessage = "Could not write " + engineRecord.string();
			return false;
		}

		// Migration from the first private-runtime layout. The engine belongs
		// to the XLang installation and is resolved through the pointer file.
		std::filesystem::remove(sitePackages / "xlang_eng.dll", error);
		return true;
	}
#endif
}

std::filesystem::path X::Tools::FindProjectPython(
	const std::filesystem::path& startDirectory)
{
	std::error_code error;
	auto current = std::filesystem::absolute(startDirectory, error);
	if (error)
		return {};
	while (true)
	{
#ifdef _WIN32
		const auto candidate = current / ".xlang" / "python" / "python.exe";
		const auto bridge = current / ".xlang" / "python" /
			"Lib" / "site-packages" / "xlang.pyd";
		const auto engineRecord = current / ".xlang" / "python" /
			"Lib" / "site-packages" / "xlang_engine_path.txt";
#else
		const auto candidate = current / ".xlang" / "python" / "bin" / "python3";
		const auto bridge = current / ".xlang" / "python" /
			"lib" / "xlang.so";
		const auto engineRecord = current / ".xlang" / "python" /
			"lib" / "xlang_engine_path.txt";
#endif
		const auto marker =
			current / ".xlang" / "python" / ".xlang-runtime";
		std::ifstream enginePathInput(engineRecord);
		std::string enginePath;
		std::getline(enginePathInput, enginePath);
		if (!enginePath.empty() && enginePath.back() == '\r')
			enginePath.pop_back();
#ifdef _WIN32
		const auto engine = std::filesystem::path(enginePath) / "xlang_eng.dll";
#else
		const auto engine =
			std::filesystem::path(enginePath) / "libxlang_eng.so";
#endif
		if (std::filesystem::is_regular_file(candidate, error) &&
			std::filesystem::is_regular_file(marker, error) &&
			std::filesystem::is_regular_file(bridge, error) &&
			std::filesystem::is_regular_file(engineRecord, error) &&
			std::filesystem::is_regular_file(engine, error))
			return candidate;
		const auto parent = current.parent_path();
		if (parent == current)
			return {};
		current = parent;
	}
}

int X::Tools::InitializePythonProject(const PythonProjectOptions& options)
{
#ifndef _WIN32
	std::cerr << "xlang init python currently supports the official Windows "
		"embeddable CPython distribution." << std::endl;
	return 2;
#else
	std::string version = options.version;
	if (version.empty())
	{
		version = FindBundledPythonVersion(options.toolsDirectory);
		if (version.empty())
		{
			std::cerr << "No packaged PyEng Python version metadata was "
				"found beside xlang.exe. Rebuild/package PyEng, or pass "
				"--version X.Y.Z explicitly." << std::endl;
			return 2;
		}
		std::cout << "Using Python " << version
			<< " matched to the bundled PyEng bridge." << std::endl;
	}
	if (!IsValidVersion(version))
	{
		std::cerr << "Invalid Python version: " << version << std::endl;
		return 2;
	}

	std::error_code error;
	const auto bridgeSource = options.toolsDirectory /
		("pyeng.cp" + AbiDigits(version) + ".dll");
	const auto engineSource = options.toolsDirectory / "xlang_eng.dll";
	if (!std::filesystem::is_regular_file(bridgeSource))
	{
		std::cerr << "No ABI-matched XLang Python bridge was found at "
			<< bridgeSource.string() << ". Build or package PyEng for Python "
			<< version << " first." << std::endl;
		return 1;
	}
	if (!std::filesystem::is_regular_file(engineSource))
	{
		std::cerr << "No XLang engine was found at "
			<< engineSource.string() << "." << std::endl;
		return 1;
	}

	const auto project = std::filesystem::absolute(
		options.projectDirectory, error);
	const auto xlangDirectory = project / ".xlang";
	const auto runtime = xlangDirectory / "python";
	const auto python = runtime / "python.exe";
	const auto runtimeMarker = runtime / ".xlang-runtime";
	if (std::filesystem::is_regular_file(python))
	{
		if (!std::filesystem::is_regular_file(runtimeMarker))
		{
			std::cerr << runtime.string()
				<< " contains python.exe but has no XLang runtime marker. "
				"Move or remove it, then run the command again." << std::endl;
			return 2;
		}
		std::string bridgeError;
		if (!ConfigureXLangBridge(runtime, bridgeSource,
			options.toolsDirectory, bridgeError))
		{
			std::cerr << bridgeError << std::endl;
			return 1;
		}
		std::cout << "Private Python already exists at "
			<< python.string() << "; refreshed the XLang bridge pointer."
			<< std::endl;
		return 0;
	}
	if (std::filesystem::exists(runtime))
	{
		std::cerr << runtime.string()
			<< " exists but is not a complete Python runtime. "
			"Move or remove it, then run the command again." << std::endl;
		return 2;
	}

	std::filesystem::create_directories(xlangDirectory, error);
	if (error)
	{
		std::cerr << "Cannot create " << xlangDirectory.string()
			<< ": " << error.message() << std::endl;
		return 1;
	}

	const auto archive = xlangDirectory /
		("python-" + version + "-embed-amd64.zip.download");
	const std::string baseUrl = "https://www.python.org/ftp/python/" +
		version + "/";
	const std::string archiveName =
		"python-" + version + "-embed-amd64.zip";
	if (!Download(baseUrl + archiveName, archive))
		return 1;

	std::filesystem::create_directory(runtime, error);
	if (error)
	{
		std::cerr << "Cannot create " << runtime.string() << std::endl;
		return 1;
	}
	const int extractResult = RunProcess(
		L"C:\\Windows\\System32\\tar.exe",
		{"-xf", archive.string(), "-C", runtime.string()});
	std::filesystem::remove(archive, error);
	if (extractResult != 0 || !std::filesystem::is_regular_file(python))
	{
		std::cerr << "Could not extract the Python distribution." << std::endl;
		std::filesystem::remove_all(runtime, error);
		return 1;
	}

	std::filesystem::create_directories(runtime / "Lib" / "site-packages");
	if (!EnableSitePackages(runtime, version))
	{
		std::filesystem::remove_all(runtime, error);
		return 1;
	}

	std::string bridgeError;
	if (!ConfigureXLangBridge(runtime, bridgeSource,
		options.toolsDirectory, bridgeError))
	{
		std::cerr << bridgeError << std::endl;
		std::filesystem::remove_all(runtime, error);
		return 1;
	}

	if (options.installPip)
	{
		const auto getPip = xlangDirectory / "get-pip.py.download";
		if (!Download("https://bootstrap.pypa.io/get-pip.py", getPip))
		{
			std::filesystem::remove_all(runtime, error);
			return 1;
		}
		const int pipResult = RunProcess(
			python, {getPip.string(), "--no-warn-script-location"}, project);
		std::filesystem::remove(getPip, error);
		if (pipResult != 0)
		{
			std::cerr << "Python was installed, but pip bootstrap failed."
				<< std::endl;
			std::filesystem::remove_all(runtime, error);
			return pipResult;
		}
	}

	std::ofstream marker(runtime / ".xlang-runtime", std::ios::trunc);
	marker << "python=" << version << "\n"
		<< "source=" << baseUrl + archiveName << "\n";
	std::cout << "Installed private Python " << version
		<< " at " << python.string() << std::endl;
	return 0;
#endif
}
