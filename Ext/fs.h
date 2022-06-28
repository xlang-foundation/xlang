#pragma once
#include "xlang.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include "bin.h"
#include "str.h"
namespace X
{
	class File
	{
		std::ifstream m_stream;
		std::string m_fileName;
		bool m_IsBinary = true;
	public:
		BEGIN_PACKAGE(File)
			ADD_FUNC("read", read)
			ADD_FUNC("write", write)
			ADD_FUNC("close", close)
			ADD_FUNC("size", get_size)
			END_PACKAGE

		File(ARGS& params,KWARGS& kwParams)
		{
			m_fileName = params[0].ToString();
			std::string mode;
			if (params.size() > 1)
			{
				mode = params[1].ToString();
			}
			m_IsBinary = (std::string::npos != mode.find_first_of('b'));
			m_stream.open(m_fileName.c_str(),
				std::ios_base::in|
				m_IsBinary?std::ios_base::binary:0);
		}
		~File()
		{
			if (m_stream.is_open())
			{
				m_stream.close();
			}
		}
		bool read(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			auto size = params[0].GetLongLong();
			if (m_IsBinary)
			{
				char* data = new char[size];
				Data::Binary* pBinObj = new Data::Binary(data, size);
				m_stream.read(data, size);
				retValue = AST::Value(pBinObj);
			}
			else
			{
				Data::Str* pStrObj = new Data::Str((size_t)size);
				char* data = pStrObj->Buffer();
				m_stream.read(data, size);
				retValue = AST::Value(pStrObj);
			}
			return true;
		}
		bool write(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			return true;
		}
		bool close(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			m_stream.close();
			retValue = AST::Value(true);
			return true;
		}
		bool get_size(void* rt, void* pContext,
			ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			struct stat stat_buf;
			int rc = stat(m_fileName.c_str(), &stat_buf);
			size_t size = (rc == 0 ? stat_buf.st_size : -1);
			retValue = AST::Value((long long)size);
			return true;
		}
	};
	class FileSystem
	{
	public:
		BEGIN_PACKAGE(FileSystem)
			ADD_CLASS("File", File)
		END_PACKAGE
		FileSystem()
		{

		}
	};
}