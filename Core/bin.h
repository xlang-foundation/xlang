#pragma once
#include "object.h"
namespace X
{
	namespace Data
	{
		class Binary
			:public Object
		{
		protected:
			char* m_data = nullptr;
			size_t m_size;
		public:
			Binary(char* data, size_t size)
			{//new copy
				m_t = Type::Binary;
				m_data = data;
				m_size = size;
			}
			char* Data() { return m_data; }
			inline virtual long long  Size()  override { return m_size; }
			~Binary()
			{
				if (m_data)
				{
					delete m_data;
				}
			}
			virtual bool Call(Runtime* rt, ARGS& params,
				KWARGS& kwParams,
				AST::Value& retValue) override
			{
				return true;
			}
		};
	}
}