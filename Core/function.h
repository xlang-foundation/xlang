/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
			virtual void ChangeStatmentsIntoTranslateMode(
				bool changeIfStatment, bool changeLoopStatment) override;
			virtual XObj* Clone() override
			{
				Function* pNewFunc = new Function();
				pNewFunc->m_ownFunc = false;//we borrow the reference
				pNewFunc->m_func = m_func;
				pNewFunc->IncRef();
				return dynamic_cast<XObj*>(pNewFunc);
			}
			virtual X::Value GetName() override
			{
				return m_func->GetFuncName();
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
			virtual int QueryMethod(const char* name, int* pFlags) override;
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