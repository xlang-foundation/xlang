#pragma once
#include "object.h"
#include "port.h"

namespace X
{
	namespace Data
	{
		class Binary:
			virtual public XBin,
			virtual public Object
		{
		protected:
			char* m_data = nullptr;
			size_t m_size;
		public:
			Binary(char* data, size_t size):
				XBin(0)
			{//new copy
				m_t = ObjType::Binary;
				m_data = data;
				m_size = size;
			}
			virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
			{
				Object::ToBytes(rt, pContext,stream);
				stream << m_size;
				stream.append(m_data, m_size);
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				stream >> m_size;
				if (m_data)
				{
					delete m_data;
				}
				m_data = new char[m_size];
				stream.CopyTo(m_data, m_size);
				return true;
			}
			virtual std::string ToString(bool WithFormat = false) override
			{
				#define CHAR_START 32 //space
				#define CHAR_END   126//~
				std::string retStr="b'";
				for (size_t i = 0; i < m_size; i++)
				{
					unsigned char ch = m_data[i];
					if (ch >= CHAR_START && ch <= CHAR_END)
					{
						retStr += ch;
					}
					else
					{
						const int online_len = 5;
						char strBuf[online_len];
						SPRINTF(strBuf, online_len, "\\x%02X",ch);
						retStr += strBuf;
					}
				}
				retStr += "'";
				return retStr;
			}
			virtual char* Data() override { return m_data; }
			inline virtual long long  Size()  override { return m_size; }
			~Binary()
			{
				if (m_data)
				{
					delete m_data;
				}
			}
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				return true;
			}
		};
	}
}