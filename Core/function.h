#pragma once
#include "xlang.h"
#include "object.h"
#include <string>
namespace X
{
	namespace Data
	{
		class Function :
			virtual public XFunc,
			virtual public Object
		{
		protected:
			bool m_ownFunc = false;//m_func owned by this object or just a reference
			AST::Func* m_func = nullptr;
		public:
			Function() :XFunc(), Object()
			{

			}
			Function(AST::Func* p,bool bOwnIt = false);
			~Function();
			virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
			{
				AST::Expression exp;
				exp.SaveToStream(rt, pContext, m_func, stream);
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream)
			{
				AST::Expression exp;
				m_ownFunc = true;
				m_func = exp.BuildFromStream<AST::Func>(stream);
				return true;
			}
			virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
				std::vector<AST::Scope*>& callables) override
			{
				return m_func?m_func->CalcCallables(rt,pContext,callables):false;
			}
			virtual std::string ToString(bool WithFormat = false) override
			{
				std::string strRet= m_func->GetNameString();
				if (strRet.empty())
				{
					char v[1000];
					snprintf(v, sizeof(v), "f[%llx]",
						(unsigned long long)this);
					strRet = v;
				}
				if (WithFormat)
				{
					strRet = "\"" + strRet + "\"";
				}
				return strRet;
			}
			AST::Func* GetFunc() { return m_func; }
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override;
			virtual bool CallEx(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& trailer,
				X::Value& retValue) override;
		};

	}
}