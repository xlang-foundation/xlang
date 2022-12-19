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
			AST::Expression* m_varToImpack = nullptr;
			X::Value m_container;
			Iterator_Pos m_pos = nullptr;
		public:
			Iterator():
				XIterator(0)
			{
				m_t = ObjType::Iterator;
			}
			inline void SetImpactVar(AST::Expression* pVar)
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
					X::ARGS vals;
					if (pDataObj->GetAndUpdatePos(m_pos, vals))
					{
						m_varToImpack->SetArry(dynamic_cast<XlangRuntime*>(rt), 
							pContext, vals);
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
