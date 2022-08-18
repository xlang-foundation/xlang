#pragma once
#include "object.h"
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
			Binary(char* data, size_t size)
			{//new copy
				m_t = ObjType::Binary;
				m_data = data;
				m_size = size;
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
			virtual bool Call(XRuntime* rt, ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				return true;
			}
		};
	}
}