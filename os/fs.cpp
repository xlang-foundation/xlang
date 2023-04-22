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
			{
				//std::cout << "Before Set Module" << std::endl;
				auto m = FileSystem::I().GetModule();
				//std::cout << "After Set Module" << std::endl;
				X::XModule* pModule = dynamic_cast<X::XModule*>(m.GetObj());
				auto p = pModule->GetPath();
				g_pXHost->ReleaseString(p);
			}
			//std::cout << "End Set Module" << std::endl;
			auto& modulePath = FileSystem::I().GetModulePath();
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