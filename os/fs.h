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
		XPackageAPISet<File> m_Apis;
	public:
		XPackageAPISet<File>& APISET() { return m_Apis; }
		File()
		{
			m_Apis.AddFunc<1>("read", &File::read);
			m_Apis.AddFunc<0>("close", &File::close);
			m_Apis.AddProp("size", &File::get_size);
			m_Apis.Create(this);
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
				return X::Value(XBin(data, size));
			}
			else
			{
				XStr str(nullptr, (int)size);
				char* data = str.Buffer();
				m_stream.read(data, size);
				return X::Value(str);
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
		XPackageAPISet<FileSystem> m_Apis;
	public:
		XPackageAPISet<FileSystem>& APISET() { return m_Apis; }
		FileSystem()
		{
			m_Apis.AddClass<2, File>("File");
			m_Apis.Create(this);
		}
	};
}