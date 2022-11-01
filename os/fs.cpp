#include "fs.h"

namespace X
{
	File::File(std::string fileName, std::string mode) :
		File()
	{
		if (fileName.find('\\') == fileName.npos 
			|| fileName.find('/') == fileName.npos)
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