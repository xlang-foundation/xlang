#include "fs.h"

namespace X
{
	File::File(std::string fileName, std::string mode) :
		File()
	{
		bool IsAbsPath = false;
#if (WIN32)
		//format like c:\\ or c:/
		//or \\ at the begin
		if (fileName.find(':') >0 || fileName.find("\\\\") > 0
			|| fileName.find("//") > 0)
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
			auto m = FileSystem::I().GetModule();
			X::XModule* pModule = dynamic_cast<X::XModule*>(m.GetObj());
			if (pModule)
			{
				auto path = pModule->GetPath();
#if (WIN32)
				fileName = path + "\\" + fileName;
#else
				fileName = path + "/" + fileName;
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