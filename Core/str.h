#pragma once

#include "object.h"
#include "utility.h"
#include <string>
namespace X 
{
namespace Data 
{
	class Str:
		virtual public XStr,
		virtual public Object
	{
	protected:
		std::string m_s;
	public:
		Str() :XStr(0)
		{
			m_t = ObjType::Str;
		}
		Str(size_t size):XStr(0)
		{
			m_t = ObjType::Str;
			m_s.resize(size);
		}
		Str(std::string& str) :XStr(0)
		{
			m_t = ObjType::Str;
			m_s = str;
		}
		Str(const std::string& str) :XStr(0)
		{
			m_t = ObjType::Str;
			m_s = str;
		}
		Str(const char* s,int size) :XStr(0)//from constant
		{//new copy
			m_t = ObjType::Str;
			m_s = std::string(s, size);
		}
		virtual char* Buffer() override { return (char*)m_s.c_str(); }
		virtual AST::Scope* GetScope();
		virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream) override
		{
			Object::ToBytes(rt,pContext,stream);
			stream << m_s;
			return true;
		}
		virtual bool FromBytes(X::XLangStream& stream) override
		{
			stream >> m_s;
			return true;
		}
		inline virtual std::string ToString(bool WithFormat = false) override
		{
			return m_s;
		}
		virtual int cmp(X::Value* r)
		{
			return m_s.compare(r->ToString());;
		}
		virtual size_t Hash() override
		{
			return std::hash<std::string>{}(m_s);
		}
		virtual Str& operator +=(X::Value& r)
		{
			switch (r.GetType())
			{
			case ValueType::Str:
			{
				m_s += r.ToString();
			}
				break;
			case ValueType::Object:
			{
				Object* pObj = dynamic_cast<Object*>(r.GetObj());
				if (pObj)
				{
					if (pObj->GetType() == ObjType::Str)
					{
						m_s += dynamic_cast<Str*>(pObj)->m_s;
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
		virtual Object& operator *=(X::Value& r)
		{
			switch (r.GetType())
			{
			case ValueType::Int64:
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
		inline size_t RFind(std::string& x, size_t offset = 0)
		{
			return m_s.rfind(x, offset);
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
		inline bool Split(std::string& delims, std::vector<std::string>& retList)
		{
			if (delims.size() == 1)
			{
				retList = split(m_s, delims[0]);
			}
			else
			{
				retList = split(m_s, delims.c_str());
			}
			return true;
		}
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams,
			X::Value& retValue) override
		{
			return true;
		}
	};
}
}