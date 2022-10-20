#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
namespace X
{
	class File
	{
		std::ifstream m_stream;
		std::string m_fileName;
		bool m_IsBinary = true;
	public:
		BEGIN_PACKAGE(File)
			APISET().AddFunc<1>("read", &File::read);
			APISET().AddFunc<0>("close", &File::close);
			APISET().AddProp("size", &File::get_size);
		END_PACKAGE
		File()
		{
		}
		File(std::string fileName, std::string mode):
			File()
		{
			m_fileName = fileName;
			m_IsBinary = (std::string::npos != mode.find_first_of('b'));
			m_stream.open(m_fileName.c_str(),
				m_IsBinary? (std::ios_base::in
				| std::ios_base::binary): std::ios_base::in);
		}
		~File()
		{
			if (m_stream.is_open())
			{
				m_stream.close();
			}
		}
		X::Value read(long long size)
		{
			if (size <= 0)
			{
				return X::Value();
			}
			if (m_IsBinary)
			{
				char* data = new char[size];
				m_stream.read(data, size);
				return X::Value(g_pXHost->CreateBin(data, size));
			}
			else
			{
				auto* pStr = g_pXHost->CreateStr(nullptr, size);
				char* data = pStr->Buffer();
				m_stream.read(data, size);
				return X::Value(pStr);
			}
		}
		bool write(void* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue)
		{
			return true;
		}
		bool close()
		{
			m_stream.close();
			return true;
		}
		X::Value get_size()
		{
			struct stat stat_buf;
			int rc = stat(m_fileName.c_str(), &stat_buf);
			size_t size = -1;
			if (rc == 0)
			{
				size = stat_buf.st_size;
			}
			return X::Value((long long)size);
		}
	};
	class FileSystem
	{
	public:
		BEGIN_PACKAGE(FileSystem)
			APISET().AddClass<2, File>("File");
		END_PACKAGE
	};
}