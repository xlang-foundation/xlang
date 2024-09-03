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
				m_t = ObjType::Function;
			}
			static void cleanup();
			Function(AST::Func* p,bool bOwnIt = false);
			~Function();
			virtual X::Value GetName() override
			{
				return m_func->GetNameString();
			}
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
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			virtual int QueryMethod(const char* name, bool* pKeepRawParams = nullptr) override;
			virtual bool GetIndexValue(int idx, Value& v) override;

			std::string GetDoc()
			{
				return m_func ? m_func->GetDoc() : "";
			}
			virtual const char* ToString(bool WithFormat = false) override
			{
				std::string strRet= m_func->GetNameString();
				if (strRet.empty())
				{
					char v[1000];
					snprintf(v, sizeof(v), "lambda:line%d-%d",
						m_func->GetStartLine(), m_func->GetEndLine());
					strRet = v;
				}
				if (WithFormat)
				{
					strRet = "\"" + strRet + "\"";
				}
				return GetABIString(strRet);
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