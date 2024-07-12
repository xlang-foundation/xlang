#pragma once
#include "exp.h"
#include "op.h"

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
				if (rt->GetTrace())
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
						auto pid = GetPID();
						std::cout << "Error Occurs in line:" << line << ",pid:" << pid << std::endl;
						auto code = i->GetCode();
						std::cout << "*** " << code << std::endl;
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
			bool m_isIf = false;//if it is 'if', this flag is true, if it is 'elif', 'else' will be false
			If* m_next = nil;//elif  or else
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
			void SetFlag(bool b)
			{
				m_isIf = b;
			}
			bool IsIf() { return m_isIf; }
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
				return true;
			}
			virtual bool EatMe(Expression* other) override;
			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}