#pragma once

#include "object.h"
namespace X
{
	namespace Data
	{
		class PropObject :
			virtual public XProp,
			virtual public Object
		{
		protected:
			AST::Func* m_setter = nil;
			AST::Func* m_getter = nil;
		public:
			PropObject(AST::Func* setter, AST::Func* getter) :
				Object(), XProp()
			{
				m_t = ObjType::Prop;
				m_setter = setter;
				m_getter = getter;
			}
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				std::string itemName, X::Value& val) override;
			bool SetPropValue (XRuntime* rt0, XObj* pContext, Value& v)
			{
				bool bOK = false;
				if (m_setter)
				{
					ARGS param(1);
					param.push_back(v);
					KWARGS kwParam;
					Value retVal;
					bOK = m_setter->Call(rt0, pContext, param, kwParam, retVal);
				}
				return bOK;
			}
			virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				if (params.size() == 0)
				{
					return GetPropValue(rt, pContext, retValue);
				}
				else
				{
					return SetPropValue(rt, pContext, params[0]);
				}
			}
			bool GetPropValue(XRuntime* rt0, XObj* pContext, Value& v)
			{
				bool bOK = false;
				if (m_getter)
				{
					ARGS param(0);
					KWARGS kwParam;
					bOK = m_getter->Call(rt0, pContext, param, kwParam, v);
				}
				return bOK;
			}
		};
	}
}