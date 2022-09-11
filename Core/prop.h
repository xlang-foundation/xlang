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
			bool SetProp(XRuntime* rt0,XObj* pContext,Value& v)
			{
				bool bOK = false;
				if (m_setter)
				{
					ARGS param{ v };
					KWARGS kwParam;
					Value retVal;
					bOK = m_setter->Call(rt0, pContext, param, kwParam, retVal);
				}
				return bOK;
			}
			bool GetProp(XRuntime* rt0, XObj* pContext, Value& v)
			{
				bool bOK = false;
				if (m_getter)
				{
					ARGS param;
					KWARGS kwParam;
					bOK = m_getter->Call(rt0, pContext, param, kwParam, v);
				}
				return bOK;
			}
		};
	}
}