#pragma once

#include "object.h"

namespace X
{
	namespace Data
	{
		class XlangStruct :
			virtual public XStruct,
			virtual public Object
		{
			bool m_bOwnData = true;
			char* m_pData = nullptr;//hold Structs data
			int m_size = 0;
		public:
			XlangStruct():
				XStruct(0),
				Object()
			{
				m_t = ObjType::Struct;
			}
			XlangStruct(char* data,int size, bool asRef):
				XlangStruct()
			{
				m_size = size;
				if (asRef)
				{
					m_pData = data;
					m_bOwnData = false;
				}
				else
				{
					m_bOwnData = true;
					m_pData = new char[size];
					if (data)
					{
						memcpy(m_pData, data, size);
					}
				}
			}
			~XlangStruct()
			{
				if (m_bOwnData && m_pData)
				{
					delete m_pData;
				}
			}
			inline virtual long long Size() { return m_size; }
			virtual char* Data() override
			{
				return m_pData;
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
			{
				AutoLock autoLock(m_lock);
				Object::ToBytes(rt, pContext, stream);
				stream << m_size;
				if (m_size)
				{
					stream.append(m_pData, m_size);
				}
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				AutoLock autoLock(m_lock);
				stream >> m_size;
				if (m_bOwnData && m_pData)
				{
					delete m_pData;
					m_pData = nullptr;
				}
				if (m_size)
				{
					m_pData = new char[m_size];
					m_bOwnData = true;
					stream.CopyTo(m_pData, m_size);
				}
				return true;
			}
		};
	}
}