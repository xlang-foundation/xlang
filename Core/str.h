#pragma once

#include "object.h"

#include <string>
namespace X 
{
namespace Data 
{
	class Str
		:public Object
	{
	protected:
		std::string m_s;
	public:
		Str(size_t size)
		{
			m_t = Type::Str;
			m_s.resize(size);
		}
		Str(const char* s,int size)//from constant
		{//new copy
			m_t = Type::Str;
			m_s = std::string(s, size);
		}
		char* Buffer() { return (char*)m_s.c_str(); }
		virtual AST::Scope* GetScope();
		inline virtual std::string ToString() override
		{
			return m_s;
		}
		virtual Str& operator +=(AST::Value& r)
		{
			switch (r.GetType())
			{
			case AST::ValueType::Str:
			{
				m_s += r.ToString();
			}
				break;
			case AST::ValueType::Object:
			{
				Object* pObj = r.GetObj();
				if (pObj)
				{
					if (pObj->GetType() == Data::Type::Str)
					{
						m_s += ((Str*)pObj)->m_s;
					}
					else
					{
						m_s += pObj->ToString();
					}
				}
			}
				break;
			default:
				m_s += r.ToString();
				break;
			}
			return *this;
		}
		virtual Object& operator *=(AST::Value& r)
		{
			switch (r.GetType())
			{
			case AST::ValueType::Int64:
			{
				long long cnt = r.GetLongLong();
				std::string s = m_s;
				for (long long i = 0; i < cnt; i++)
				{
					m_s += s;
				}
			}
			break;
			default:
				break;
			}
			return *this;
		}
		inline size_t Find(std::string& x, size_t offset = 0)
		{
			return m_s.find(x, offset);
		}
		inline size_t GetSize() { return m_s.size(); }
		inline bool Slice(size_t start,size_t end,std::string& retVal)
		{
			size_t len=0;
			if (end == -1 || end>= m_s.size())
			{
				len = std::string::npos;
			}
			else
			{
				len = end - start;
			}
			retVal = m_s.substr(start, len);
			return true;
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