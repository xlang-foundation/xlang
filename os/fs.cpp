#include "fs.h"

namespace fs = std::filesystem;

namespace X
{
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
		if (m_IsWrite)
		{
			m_wstream.open(m_fileName.c_str(),
				m_IsBinary ? (std::ios_base::out
					| std::ios_base::binary) : std::ios_base::out);
		}
		else
		{
			m_stream.open(m_fileName.c_str(),
				m_IsBinary ? (std::ios_base::in
					| std::ios_base::binary) : std::ios_base::in);

		}
	}

}