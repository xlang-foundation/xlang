#pragma once
#include "scope.h"
#include "singleton.h"
#include "stackframe.h"

namespace X
{
	namespace AST
	{
		class MetaScope :
			public Singleton<MetaScope>
		{
			Scope* m_pMyScope = nullptr;
			StackFrame* m_variableFrame = nullptr;
		public:
			MetaScope()
			{
				m_variableFrame = new StackFrame();
				m_pMyScope = new Scope();
				m_pMyScope->SetVarFrame(m_variableFrame);
			}
			inline Scope* GetMyScope()
			{
				return m_pMyScope;
			}
			void Init()
			{

			}
			void Cleanup()
			{
				if (m_variableFrame)
				{
					delete m_variableFrame;
					m_variableFrame = nullptr;
				}
			}
			~MetaScope()
			{
				if (m_variableFrame)
				{
					delete m_variableFrame;
				}
			}
		};
	}
}