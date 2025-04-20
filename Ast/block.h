﻿/*
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
#include "exp.h"
#include "op.h"
#include "xlog.h"


namespace X
{
	namespace AST
	{
		struct Indent
		{
			int charPos = 0;
			int tab_cnt = 0;
			int space_cnt = 0;
			Indent(int cp, int tab, int space)
			{
				charPos = cp;
				tab_cnt = tab;
				space_cnt = space;
			}
			bool operator>=(const Indent& other)
			{
				return (charPos >= other.charPos)
					&& (tab_cnt >= other.tab_cnt)
					&& (space_cnt >= other.space_cnt);
			}
			bool Equal(const Indent& other)
			{
				return (charPos == other.charPos)
					&& (tab_cnt == other.tab_cnt)
					&& (space_cnt == other.space_cnt);
			}
			bool operator==(const Indent& other)
			{
				return (charPos == other.charPos)
					&& (tab_cnt == other.tab_cnt)
					&& (space_cnt == other.space_cnt);
			}
			bool operator<(const Indent& other)
			{
				return (charPos <= other.charPos) &&
					((tab_cnt <= other.tab_cnt && space_cnt < other.space_cnt)
						|| (tab_cnt < other.tab_cnt && space_cnt <= other.space_cnt));
			}
		};

		class ActionOperator :
			public Operator
		{
		public:
			ActionOperator() :Operator()
			{
				m_type = ObType::ActionOp;
			}
			ActionOperator(short op) :
				Operator(op)
			{
				m_type = ObType::ActionOp;
			}
			virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands, int LeftTokenIndex)
			{
				operands.push(this);
				return true;
			}
			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
				Value& v, LValue* lValue = nullptr)
			{
				return true;
			}
		};
		class Block :
			public UnaryOp
		{
		protected:
			bool NoIndentCheck = false;//just for lambda block
			Indent IndentCount = { 0,-1,-1 };
			Indent ChildIndentCount = { 0,-1,-1 };
			bool m_bRunning = false;
			std::vector<Expression*> Body;
		public:
			Block() :UnaryOp()
			{
				m_type = ObType::Block;
			}
			Block(short op) :
				UnaryOp(op)
			{
				m_type = ObType::Block;
			}
			~Block()
			{
				for (auto it : Body)
				{
					delete it;
				}
				Body.clear();
			}
			FORCE_INLINE bool Expanding(X::Exp::ExpresionStack& stack)
			{
				for (auto rit = Body.rbegin(); rit != Body.rend(); ++rit)
				{
					stack.push({ *rit,false });
				}
				return true;
			}
			FORCE_INLINE std::vector<Expression*>& GetBody()
			{
				return Body;
			}
			long long GetBodySize()
			{
				return Body.size();
			}
			//using Body's last expression's line number to get the end line number
			FORCE_INLINE void ReCalcHintWithBody()
			{
				if (Body.size() > 0)
				{
					auto& last = Body[Body.size() - 1];
					ReCalcHint(last);
				}
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
			{
				UnaryOp::ToBytes(rt, pContext, stream);
				stream << NoIndentCheck << IndentCount
					<< ChildIndentCount << m_bRunning;
				stream << (int)Body.size();
				for (auto* exp : Body)
				{
					SaveToStream(rt, pContext, exp, stream);
				}
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				UnaryOp::FromBytes(stream);
				stream >> NoIndentCheck >> IndentCount
					>> ChildIndentCount >> m_bRunning;
				int size = 0;
				stream >> size;
				for (int i = 0; i < size; i++)
				{
					auto* exp = BuildFromStream<Expression>(stream);
					Body.push_back(exp);
				}
				return true;
			}
			FORCE_INLINE int GetStartLine()
			{
				if (Body.size() > 0)
				{
					return Body[0]->GetStartLine();
				}
				else
				{
					return -1;
				}
			}
			FORCE_INLINE bool IsNoIndentCheck()
			{
				return NoIndentCheck;
			}
			FORCE_INLINE void SetNoIndentCheck(bool b)
			{
				NoIndentCheck = b;
			}
			virtual void Add(Expression* item);
			FORCE_INLINE Indent GetIndentCount() { return IndentCount; }
			FORCE_INLINE Indent GetChildIndentCount() { return ChildIndentCount; }
			FORCE_INLINE void SetIndentCount(Indent cnt) { IndentCount = cnt; }
			FORCE_INLINE void SetChildIndentCount(Indent cnt) { ChildIndentCount = cnt; }
			bool RunLast(XRuntime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr);
			bool RunFromLine(XRuntime* rt, XObj* pContext, long long lineNo, Value& v, LValue* lValue = nullptr);
			bool ExecForTrace(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue);
			FORCE_INLINE virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override
			{
				return Exec_i(rt, action, pContext, v, lValue);
			}

			FORCE_INLINE bool Exec_i(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr)
			{
				auto bodySize = Body.size();
				if (bodySize == 0)
				{
					return true;
				}
				if (!rt->m_bNoDbg && G::I().GetTrace())
				{
					return ExecForTrace(rt, action, pContext, v, lValue);
				}

				bool bOk = true;
				m_bRunning = true;
				//just for debug easy,write same code here
				// because dbg also run xlang code
				//then easy to set breakpoint for xlang code in debug mode
				//not for dbg xlang code

				//for break, continue and pass
				//just do a check if i it is one of them
				//then do process with them

				auto lastIdx = bodySize - 1;
				for (size_t idx = 0; idx < bodySize; idx++)
				{
					auto& i = Body[idx];
					if (i->m_type == ObType::ActionOp)
					{
						auto* pActionOperator = dynamic_cast<ActionOperator*>(i);
						OP_ID opId = pActionOperator->GetId();
						switch (opId)
						{
							case OP_ID::Break:
								action.type = ExecActionType::Break;
								break;
							case OP_ID::Continue:
								action.type = ExecActionType::Continue;
								break;
							case OP_ID::Pass:
								continue;//just run next line					
							default:
								break;
						}
					}
					Value v0;
					int line = i->GetStartLine();
					int pos = i->GetCharPos();
					//Update Stack Frame
					rt->GetCurrentStack()->SetLine(line);
					rt->GetCurrentStack()->SetCharPos(pos);
					rt->GetCurrentStack()->SetCurExp(i);
					ExecAction action0;
					bOk = ExpExec(i, rt, action0, pContext, v0, lValue);
					//if break or cotinue action passed back
					//break this loop,and pass back to caller
					if (action0.type == ExecActionType::Break ||
						action0.type == ExecActionType::Continue)
					{
						action = action0;
						break;
					}
					if (!bOk)
					{
						LOG <<LOG_RED<< "Error Occurs in Module: "<<rt->GetName()<< ":" << line <<LOG_RESET<< LINE_END;
						auto code = i->GetCode();
						LOG << LOG_RED << "*** " << code <<LOG_RESET<<LINE_END;
					}
					if (v0.IsValid() && (idx == lastIdx))
					{
						v = v0;
					}
					if (action0.type == ExecActionType::Return)
					{
						action = action0;
						break;
					}
				}
				m_bRunning = false;
				return bOk;
			}
		};
		class For :
			public Block
		{
		public:
			For() :
				Block()
			{
				m_type = ObType::For;
			}
			For(short op) :
				Block(op)
			{
				m_type = ObType::For;
			}
			virtual void ScopeLayout() override;
			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
		class While :
			public Block
		{
		public:
			While() :
				Block()
			{
				m_type = ObType::While;
			}
			While(short op) :
				Block(op)
			{
				m_type = ObType::While;
			}

			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};

		class If :
			public Block
		{
			//Translate Mode means other program will translate
			//If with its blocks into another code like cuda or c++
			//so if m_translateMode is true, need to run Translate 
			//instead to run Exec

			bool m_translateMode = false;
			bool m_isIf = false;//if it is 'if', this flag is true, if it is 'elif', 'else' will be false
			If* m_next = nil;//elif  or else
			If* m_prev = nil;//just record it, don't need to delete
		public:
			If() :
				Block()
			{
				m_type = ObType::If;
			}
			If(short op, bool needParam = true) :
				Block(op)
			{
				m_type = ObType::If;
				NeedParam = needParam;
			}
			inline If* GetNext() { return m_next; }
			inline If* GetPrev() { return m_prev; }
			inline void SetTranslateMode(bool bMode)
			{
				m_translateMode = bMode;

				auto bodySize = Body.size();
				for (size_t idx = 0; idx < bodySize; idx++)
				{
					auto& i = Body[idx];
					if (i->m_type == ObType::If)
					{
						If* pIf = dynamic_cast<If*>(i);
						if (pIf)
						{
							pIf->SetTranslateMode(bMode);
						}
					}
				}
				if (m_next)
				{
					m_next->SetTranslateMode(bMode);
				}
			}
			bool Translate(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue);
			inline void SetFlag(bool b)
			{
				m_isIf = b;
			}
			inline bool IsIf() { return m_isIf; }
			~If()
			{
				if (m_next) delete m_next;
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
			{
				Block::ToBytes(rt, pContext, stream);
				SaveToStream(rt, pContext, m_next, stream);
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				Block::FromBytes(stream);
				m_next = BuildFromStream<If>(stream);
				if (m_next)
				{
					m_next->m_prev = this;
				}
				return true;
			}
			virtual bool EatMe(Expression* other) override;
			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}