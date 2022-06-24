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
			size_t Size() { return m_size; }
			~Binary()
			{
				if (m_data)
				{
					delete m_data;
				}
			}
			virtual bool Call(Runtime* rt, std::vector<AST::Value>& params,
				std::unordered_map<std::string, AST::Value>& kwParams,
				AST::Value& retValue) override
			{
				return true;
			}
		};
	}
}