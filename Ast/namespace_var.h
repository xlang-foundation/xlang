#pragma once
#include "block.h"
#include <assert.h> 
#include "scope.h"
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
			X::Value m_valObj;//Hold an object of NamespaceVarObject
		public:
			NamespaceVar() :
				Block(), UnaryOp(), Operator(), Scope()
			{
				Init();
				m_type = ObType::NamespaceVar;
			}
			NamespaceVar(short op) :
				Block(), UnaryOp(), Operator(op), Scope()
			{
				Init();
				m_type = ObType::NamespaceVar;
			}
			~NamespaceVar()
			{

			}
			inline void Init()
			{
				if (m_valObj.IsInvalid())
				{
					m_valObj = new X::Data::NamespaceVarObject(this);
				}
			}
			inline X::Data::NamespaceVarObject* Obj()
			{
				return dynamic_cast<X::Data::NamespaceVarObject*>(m_valObj.GetObj());
			}
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v)
			{
				assert(idx != -1);
				Obj()->Set(idx, v);
				return true;
			}
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext, Value& v) override
			{
				if (m_Index == -1)
				{
					ScopeLayout();
					assert(m_Index != -1 && m_scope != nullptr);
				}
				return m_scope->Set(rt, pContext, m_Index, v);
			}
			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, LValue* lValue = nullptr)
			{
				return Obj()->Get(idx, v, lValue);
			}
			inline X::Value& GetVal() { return m_valObj; }
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
				std::stack<AST::Expression*>& operands)
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
				if (m_parent && m_parent->m_type == ObType::NamespaceVar)
				{
					return dynamic_cast<Scope*>(m_parent);
				}
				else
				{
					return nullptr;
				}
			}
		};
	}
}