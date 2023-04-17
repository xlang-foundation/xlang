#pragma once
#include "object.h"
#include "port.h"

namespace X
{
	namespace Data
	{
#define CHAR_START 32 //space
#define CHAR_END   126//~

		class Binary:
			virtual public XBin,
			virtual public Object
		{
		protected:
			char* m_data = nullptr;
			bool m_OwnData = true;
			size_t m_size;
		public:
			Binary(char* data, size_t size,bool bOwnData):
				XBin(0)
			{//new copy
				m_t = ObjType::Binary;
				m_OwnData = bOwnData;
				if (data == nullptr && size>0)
				{
					m_data = new char[size];
					m_OwnData = true;
				}
				else
				{
					m_data = data;
				}
				m_size = size;
			}
			virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
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
				m_OwnData = true;
				stream.CopyTo(m_data, m_size);
				return true;
			}
			virtual bool FromString(const char* strCoded) override
			{
				if (m_data)
				{
					delete m_data;
				}
				auto size = strlen(strCoded);
				if (size > 2 && strCoded[0] == 'b' && strCoded[1] == '\'')
				{
					m_data = new char[size];//bytes converted from thi string,
					//which size should be less or eqaul to this string's size
					m_OwnData = true;
					auto i = 2;
					size_t byteIndex = 0;
					bool bCorrect = true;
					while(i<size)
					{
						unsigned char ch = strCoded[i];
						if (ch == '\\')
						{
							if ((i + 4) <= size 
								&& strCoded[i + 1] == 'x')
							{
								ch = ((strCoded[i + 2]-'0') << 8) + strCoded[i + 3]-'0';
								i += 4;
								m_data[byteIndex++] = ch;
							}
							else
							{
								bCorrect = false;
								break;
							}
						}
						else if (ch >= CHAR_START && ch <= CHAR_END)
						{
							m_data[byteIndex++] = ch;
							i++;
						}
						else
						{
							bCorrect = false;
							break;
						}
					}
					m_size = byteIndex;
					return true;
				}
				else
				{
					return false;
				}
			}
			virtual const char* ToString(bool WithFormat = false) override
			{
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
				return GetABIString(retStr);
			}
			virtual char* Data() override { return m_data; }
			inline virtual long long  Size()  override { return m_size; }
			~Binary()
			{
				if (m_OwnData && m_data!=nullptr)
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