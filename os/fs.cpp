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

#include "fs.h"
#if (WIN32)
#include <Windows.h>
#endif
namespace fs = std::filesystem;

namespace X
{
	// Helper function: UTF-8 to UTF-16
	extern std::wstring UTF8ToWString(const std::string& utf8);

	File::File(std::string fileName, std::string mode) :
		File()
	{
		bool IsAbsPath = false;
#if (WIN32)
		//format like c:\\ or c:/
		//or \\ at the begin
		if (fileName.find(':') !=std::string::npos
			|| fileName.find("\\\\") == 0
			|| fileName.find("/") == 0)
		{
			IsAbsPath = true;
		}
#else
		if (fileName.find('/') == 0 || fileName.find("~") ==0)
		{
			IsAbsPath = true;
		}

#endif
		if (!IsAbsPath)
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
					modulePath = FileSystem::I().GetModulePath();
				}
			}
			else
			{
				modulePath = FileSystem::I().GetModulePath();
			}
			if (!modulePath.empty())
			{
#if (WIN32)
				fileName = modulePath + "\\" + fileName;
#else
				fileName = modulePath + "/" + fileName;
#endif
			}
		}
		m_fileName = fileName;
		m_IsBinary = (std::string::npos != mode.find_first_of('b'));
		m_IsWrite = (std::string::npos != mode.find_first_of('w'));
		// Convert UTF-8 to UTF-16
#if (WIN32)
		std::wstring wideFileName = UTF8ToWString(m_fileName);

		if (m_IsWrite)
		{
			m_wstream.open(wideFileName.c_str(),
				m_IsBinary ? (std::ios_base::out | std::ios_base::binary)
				: std::ios_base::out);
		}
		else
		{
			m_stream.open(wideFileName.c_str(),
				m_IsBinary ? (std::ios_base::in | std::ios_base::binary)
				: std::ios_base::in);
		}
#else
		if (m_IsWrite)
		{
			m_wstream.open(m_fileName.c_str(),
				m_IsBinary ? (std::ios_base::out | std::ios_base::binary)
				: std::ios_base::out);
		}
		else
		{
			m_stream.open(m_fileName.c_str(),
				m_IsBinary ? (std::ios_base::in | std::ios_base::binary)
				: std::ios_base::in);
		}
#endif
	}
	X::Value File::get_size()
	{
		size_t size = -1;

#if (WIN32)
		// Convert UTF-8 to wide string
		std::wstring wideFileName = UTF8ToWString(m_fileName);

		struct _stat64 stat_buf;
		int rc = _wstat64(wideFileName.c_str(), &stat_buf);
#else
		struct stat stat_buf;
		int rc = stat(m_fileName.c_str(), &stat_buf);
#endif

		if (rc == 0)
		{
			size = stat_buf.st_size;
		}

		return X::Value((long long)size);
	}

}