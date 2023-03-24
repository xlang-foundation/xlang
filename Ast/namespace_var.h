#pragma once
#include "block.h"
#include <assert.h> 
#include "namespacevar_object.h"
#include "dotop.h"

namespace X
{
	namespace AST
	{
		class NamespaceVar :
			virtual public Block,
			virtual public Scope
		{
			int m_Index = -1;
			X::Value m_obj;
		public:
			NamespaceVar() :
				Block(), UnaryOp(), Operator()
			{
				m_type = ObType::NamespaceVar;
			}
			NamespaceVar(short op) :
				Block(), UnaryOp(), Operator(op)
			{
				m_type = ObType::NamespaceVar;
			}
			~NamespaceVar()
			{

			}
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v)
			{
				if (m_obj.IsObject())
				{
					auto* pNameObj = dynamic_cast<Data::NamespaceVarObject*>(m_obj.GetObj());
					pNameObj->Set(idx, v);
					return true;
				}
				else
				{
					return false;
				}
			}
			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, LValue* lValue = nullptr)
			{
				if (m_obj.IsObject())
				{
					auto* pNameObj = dynamic_cast<Data::NamespaceVarObject*>(m_obj.GetObj());
					return pNameObj->Get(idx,v,lValue);
				}
				else
				{
					return false;
				}

			}
			virtual int AddOrGet(std::string& name, bool bGetOnly)
			{
				if (m_obj.IsObject())
				{
					auto* pNameObj = dynamic_cast<Data::NamespaceVarObject*>(m_obj.GetObj());
					int idx = pNameObj->AddOrGet(name, bGetOnly);
					if (idx >= 0)
					{
						pNameObj->AddSlotTo(idx);
					}
					return idx;
				}
				else
				{
					return -1;
				}
			}
			inline std::string GetName()
			{
				if (R)
				{
					if (R->m_type == ObType::Var)
					{
						return dynamic_cast<Var*>(R)->GetNameString();
					}
					else if (R->m_type == ObType::Dot)
					{
						auto* pDotOp = dynamic_cast<DotOp*>(R);
						auto* l0 = pDotOp->GetL();
						if (l0->m_type == ObType::Var)
						{
							return dynamic_cast<Var*>(l0)->GetNameString();
						}
						else
						{
							//error
						}
					}
				}
				return "";
			}
			void AddHhierarchy(Expression* item);
			virtual void Add(Expression* item) override;
			virtual void ScopeLayout() override;
			virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands, int LeftTokenIndex)
			{
				if (!operands.empty()
					&& operands.top()->GetTokenIndex() > m_tokenIndex)
				{
					R = operands.top();
				}
				operands.push(this);
				return true;
			}
			virtual bool Exec(XlangRuntime* rt, 
				ExecAction& action, XObj* pContext, 
				Value& v, LValue* lValue = nullptr) override;

			// Inherited via Scope
			virtual Scope* GetParentScope() override 
			{ 
				return FindScope(); 
			}
		};
	}
}