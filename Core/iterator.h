#pragma once
#include "object.h"
#include "var.h"
#include "runtime.h"

namespace X
{
	namespace Data
	{
		class Iterator :
			virtual public XIterator,
			virtual public Object
		{
			AST::Var* m_varToImpack = nullptr;
			X::Value m_container;
			Iterator_Pos m_pos = nullptr;
		public:
			Iterator():
				XIterator(0)
			{
				m_t = ObjType::Iterator;
			}
			inline void SetImpactVar(AST::Var* pVar)
			{
				m_varToImpack = pVar;
			}
			inline void SetContainer(X::Value& v)
			{
				m_container = v;
			}
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				if (m_container.IsObject())
				{
					auto* pDataObj = dynamic_cast<Data::Object*>(m_container.GetObj());
					X::Value newVal;
					if (pDataObj->GetAndUpdatePos(m_pos, newVal))
					{
						m_varToImpack->Set(dynamic_cast<XlangRuntime*>(rt), pContext, newVal);
						retValue = X::Value(true);
					}
					else
					{
						retValue = X::Value(false);
					}
				}
				else
				{
					retValue = X::Value(false);
				}
				return true;
			}
		};
	}
}
