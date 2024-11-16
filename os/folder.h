/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include "xpackage.h"
#include "xlang.h"
#if (WIN32)
#include <windows.h>
#endif
#include <locale>
#include <codecvt> 

// Function to convert wstring (wide string) to UTF-8 string
inline std::string WStringToUTF8(const std::wstring& wstr) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

namespace X {

	class Folder {
	public:
		BEGIN_PACKAGE(Folder)
			APISET().AddPropWithType<std::string>("Path", &Folder::folderPath);
		APISET().AddFunc<1>("BuildPath", &Folder::BuildPath);
		APISET().AddFunc<0>("Scan", &Folder::Scan);
		APISET().AddFunc<0>("List", &Folder::List);
		APISET().AddFunc<1>("CopyFolder", &Folder::CopyFolder);
		APISET().AddFunc<2>("CopyFile", &Folder::CopyFile);
		APISET().AddFunc<1>("CreateFolder", &Folder::CreateFolder);
		APISET().AddFunc<2>("RemoveFolder", &Folder::RemoveFolder);
		APISET().AddFunc<2>("Rename", &Folder::Rename);
		APISET().AddFunc<1>("DeleteFile", &Folder::DeleteFile);
		APISET().AddFunc<1>("MoveFolder", &Folder::MoveFolder);
		APISET().AddFunc<0>("ParentPath", &Folder::ParentPath);
		APISET().AddFunc<1>("RelativePath", &Folder::RelativePath);
		APISET().AddFunc<1>("IsAbsolutePath", &Folder::IsAbsolutePath);
		END_PACKAGE

	public:
		Folder(const std::string& path)
		{
			std::filesystem::path fsPath = std::filesystem::path(path);
			// Remove trailing separators
			fsPath = fsPath.lexically_normal();
			folderPath = fsPath.make_preferred().string();
		}

		std::string BuildPath(const std::string& subPath)
		{
			std::filesystem::path fullPath = folderPath;
			fullPath /= std::filesystem::path(subPath);
			return fullPath.make_preferred().string();
		}

		X::List List() {
			X::List resultList;
			try {
				for (const auto& entry : std::filesystem::directory_iterator(
					std::filesystem::u8path(folderPath))) {
#if (WIN32)
					// Convert wide string (wstring) to UTF-8 string on Windows
					resultList += WStringToUTF8(entry.path().filename().wstring());
#else
					resultList += entry.path().filename().string();
#endif
				}
			}
			catch (const std::filesystem::filesystem_error& e) {
				// Handle the exception, log it, or take other appropriate actions
				std::cerr << "Error while accessing folder: " << e.what() << std::endl;
			}
			return resultList;
		}

		X::List Scan() {
			X::List resultList;

			if (folderPath.empty()) {
#if (WIN32)
				// Enumerate all available drives on Windows
				char driveLetter = 'A';
				DWORD driveMask = GetLogicalDrives();
				while (driveMask) {
					if (driveMask & 1) {
						std::string drivePath = std::string(1, driveLetter) + ":/";
						X::Dict driveInfo;
						driveInfo->Set("Name", drivePath);
						driveInfo->Set("IsDirectory", "true");
						driveInfo->Set("Size", "N/A");
						driveInfo->Set("LastModified", "N/A");
						resultList += driveInfo;
					}
					driveLetter++;
					driveMask >>= 1;
				}
#endif
				return resultList;
			}

			// Scan the specified folderPath
			for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
				X::Dict fileInfo;
#if (WIN32)
				fileInfo->Set("Name", WStringToUTF8(entry.path().filename().wstring()));
#else
				fileInfo->Set("Name", entry.path().filename().string());
#endif
				fileInfo->Set("IsDirectory", entry.is_directory() ? "true" : "false");

				if (!entry.is_directory()) {
					fileInfo->Set("Size", std::to_string(std::filesystem::file_size(entry.path())));
				}

				auto ftime = std::filesystem::last_write_time(entry.path());
				auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ftime - std::filesystem::file_time_type::clock::now()
					+ std::chrono::system_clock::now());
				std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
				fileInfo->Set("LastModified", std::asctime(std::localtime(&cftime)));

				resultList += fileInfo;
			}
			return resultList;
		}

		bool CopyFolder(const std::string& targetPath) {
			try {
				std::filesystem::copy(folderPath, targetPath, std::filesystem::copy_options::recursive);
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}

		bool CopyFile(const std::string& filePath, const std::string& targetPath) {
			try {
				std::filesystem::copy(filePath, targetPath);
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}

		bool CreateFolder(const std::string& path) {
			try {
				std::filesystem::create_directories(path);
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}

		bool RemoveFolder(const std::string& path, bool recursive = false) {
			try {
				if (recursive) {
					std::filesystem::remove_all(path);
				}
				else {
					std::filesystem::remove(path);
				}
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}

		bool Rename(const std::string& srcPath, const std::string& targetPath) {
			try {
				std::filesystem::rename(srcPath, targetPath);
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}

		bool DeleteFile(const std::string& filePath) {
			try {
				std::filesystem::remove(filePath);
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}
		bool MoveFolder(const std::string& targetPath) {
			try {
				std::filesystem::rename(folderPath, targetPath);
				folderPath = targetPath; // Update the folderPath to the new location
				return true;
			}
			catch (std::filesystem::filesystem_error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return false;
			}
		}

		std::string ParentPath() const {
			return std::filesystem::path(folderPath).parent_path().string();
		}

		std::string RelativePath(const std::string& basePath) const {
			return std::filesystem::relative(folderPath, basePath).string();
		}
		bool IsAbsolutePath(const std::string& path) const {
#if (WIN32)
			return path.size() > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/');
#else
			return !path.empty() && path[0] == '/';
#endif
		}

	private:
		std::string folderPath;
		};

	} // namespace X
