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

#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include "file.h"
#include "folder.h"

namespace fs = std::filesystem;
namespace X
{
	// File class has been moved to file.h

	class FileSystem:
		public Singleton<FileSystem>
	{
		X::Value m_curModule;
		std::string m_curModulePath;
	public:
		void Run()
		{
#if 0
			std::string code = R"(
				xyz =10
				abc ="this is a string"
				def init(x,y):
					f = File("1.txt","w")
					f.close()
					return x+y
			)";
			APISET().GetPack()->RunCodeWithThisScope(code.c_str());
#endif
		}
		void SetModule(X::Value curModule)
		{
			X::XModule* pModule = dynamic_cast<X::XModule*>(curModule.GetObj());
			if (pModule)
			{
				auto path = pModule->GetPath();
				m_curModulePath = path;
				g_pXHost->ReleaseString(path);
			}
			m_curModule = curModule;
			//std::cout << "*****SetModule****" << std::endl;
		}
		std::string& GetModulePath()
		{
			return m_curModulePath;
		}
		void SetModulePath(std::string& path)
		{
			m_curModulePath = path;
		}
		X::Value& GetModule() 
		{ 
			return m_curModule; 
		}
		std::string GetXModulePath()
		{
			std::string modulePath;
			X::XRuntime* pRt = X::g_pXHost->GetCurrentRuntime();
			if (pRt)
			{
				X::Value varModulePath = pRt->GetXModuleFileName();
				if (varModulePath.IsValid())
				{
					std::string strModulePath = varModulePath.ToString();
					if (!strModulePath.empty())
					{
						fs::path filePath(strModulePath);
						modulePath = filePath.parent_path().string();
					}
				}
				else
				{
					//todo:
					modulePath = GetModulePath();
				}
			}
			else
			{
				modulePath = GetModulePath();
			}
			return modulePath;
		}

		std::string ConvertReletivePathToFullPath(std::string strPath)
		{
			std::string modulePath = GetModulePath();
			std::filesystem::path rootPath = modulePath;
			std::filesystem::path fullPath;

			// Check if the path is already absolute
			if (std::filesystem::path(strPath).is_absolute())
			{
				fullPath = std::filesystem::absolute(strPath);
			}
			else {
				// Combine the paths and make absolute
				fullPath = std::filesystem::absolute(rootPath / strPath);
			}

			// Normalize the path by resolving '..' and '.'
			fullPath = fullPath.lexically_normal();

			return fullPath.string();
		}
		bool CheckAndMakeFolder(std::string folderPath)
		{
			// Check if the folder already exists
			if (!std::filesystem::exists(folderPath))
			{
				// Create the folder
				return std::filesystem::create_directories(folderPath);
			}
			return true;
		}
		std::string ReadAllTexts(std::string fileName)
		{
			auto fullPath = ConvertReletivePathToFullPath(fileName);
			if (fullPath.empty())
			{
				return "";
			}
			// Check if the file exists
			if (std::filesystem::exists(fullPath))
			{
				// Open the file for reading
				std::ifstream file(fullPath);

				if (file.is_open()) 
				{
					// Read the entire file into a string
					std::string file_content((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());
					file.close();
					return file_content;
				}
				else 
				{
					return "";
				}
			}
			else 
			{
				return "";
			}
			return "";
		}
		BEGIN_PACKAGE(FileSystem)
			APISET().AddClass<2, File>("File");
			APISET().AddClass<1, Folder>("Folder");
			APISET().AddFunc<1>("ReadAllTexts", &FileSystem::ReadAllTexts);
			APISET().AddFunc<1>("CheckAndMakeFolder", &FileSystem::CheckAndMakeFolder);
		END_PACKAGE
	};
}