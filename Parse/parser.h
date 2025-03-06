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

#include "exp.h"
#include "token.h"
#include <stack>
#include <vector>
#include "def.h"
#include "block.h"
#include "blockstate.h"
#include "number.h"
#include "module.h"
#include "op_registry.h"
#include "decor.h"

namespace X 
{
	namespace AST
	{
		class Module;
		class JitBlock;
	}
class OpRegistry;
class Parser
{
	OpRegistry* m_reg = nullptr;
	int m_tokenIndex = 0;
	Token* mToken = nil;
	AST::Block* m_lastComingBlock = nullptr;
	std::stack<BlockState*> m_stackBlocks;
	BlockState* m_curBlkState = nil;
	std::vector<short> m_preceding_token_indexstack;
	std::string m_strModuleName;//for debug
	//status for JitBlock
	//if this flag is true, means the next line will be a JitBlock
	//then inside LineOpFeedIntoBlock, will check the input line is JitBlock or not
	//use this flag to avoid this check always
	bool m_bMeetJitBlock = false;
	bool m_bInsideJitBlock = false;
	AST::JitBlock* m_curJitBlock = nullptr;
	//Use this to meet some line pos >= this then this block finished
	int m_blockStartCharPos = 0;

	FORCE_INLINE bool LastIsLambda();
	//use this stack to keep 3 preceding tokens' index
	//and if meet slash, will pop one, because slash means line continuing
	FORCE_INLINE void reset_preceding_token()
	{
		m_preceding_token_indexstack.clear();
	}
	FORCE_INLINE void push_preceding_token(short idx)
	{
		if (m_preceding_token_indexstack.size() > 3)
		{
			m_preceding_token_indexstack.erase(
				m_preceding_token_indexstack.begin());
		}
		m_preceding_token_indexstack.push_back(idx);
	}
	FORCE_INLINE short get_last_token()
	{
		return m_preceding_token_indexstack.size() > 0?
			m_preceding_token_indexstack[
				m_preceding_token_indexstack.size() - 1] :
			(short)TokenIndex::TokenInvalid;
	}
	FORCE_INLINE void pop_preceding_token()
	{
		if (m_preceding_token_indexstack.size() > 0)
		{
			m_preceding_token_indexstack.pop_back();
		}
	}

private:
	void ResetForNewLine();
	bool LineOpFeedIntoBlock(AST::Expression* line,
		AST::Indent& lineIndent);
	FORCE_INLINE AST::Expression* IsJitBlock(AST::Expression* lineStatment)
	{
		AST::Expression* pExpJitBlock = nullptr;
		auto opType = lineStatment->m_type;
		if (opType == AST::ObType::JitBlock)
		{
			pExpJitBlock = lineStatment;
		}
		else if (opType == AST::ObType::BinaryOp)
		{
			auto* pBinOp = (AST::BinaryOp*)lineStatment;
			auto* pL = pBinOp->GetL();
			if (pL)
			{
				if (pL->m_type == AST::ObType::BinaryOp)
				{
					auto* pLBinOp = (AST::BinaryOp*)pL;
					auto* pLL = pLBinOp->GetL();
					if (pLL && pLL->m_type == AST::ObType::JitBlock)
					{
						pExpJitBlock = pLL;
					}
				}
				else if (pL->m_type == AST::ObType::JitBlock)
				{
					pExpJitBlock = pL;
				}
			}
		}
		return pExpJitBlock;
	}
public:
	FORCE_INLINE void SetModuleName(std::string strName)
	{
		m_strModuleName = strName;
	}
	void SetSkipLineFeedFlags(bool b)
	{
		if (m_curBlkState)
		{
			m_curBlkState->m_SkipLineFeedN = b;
		}
	}
	FORCE_INLINE void SetMeetJitBlock(bool b)
	{
		m_bMeetJitBlock = b;
	}
	FORCE_INLINE bool LastLineIsJitDecorator()
	{
		if (m_curBlkState == nullptr)
		{
			return false;
		}
		auto* pBlock = m_curBlkState->Block();
		if (pBlock)
		{
			auto& lasBlockBody = pBlock->GetBody();
			if (lasBlockBody.size())
			{
				auto* pLastLine = lasBlockBody[lasBlockBody.size() - 1];
				if (pLastLine->m_type == AST::ObType::Decor)
				{
					auto* pDecor = (AST::Decorator*)pLastLine;
					if (pDecor->IsJitDecorator())
					{
						return true;
					}
				}
			}
		}
		return false;
	}
	BlockState* GetCurBlockState() {return m_curBlkState;}
	bool NewLine(bool meetLineFeed_n,bool checkIfIsLambdaOrPair = true);
	AST::Operator* PairLeft(short opIndex);//For "(","[","{"
	void PairRight(OP_ID leftOpToMeetAsEnd); //For ')',']', and '}'
	FORCE_INLINE bool PreTokenIsOp()
	{ 
		if (m_preceding_token_indexstack.size() == 0)
		{
			return false;
		}
		else
		{
			return m_preceding_token_indexstack[m_preceding_token_indexstack.size() - 1] >= 0;
		}
	}

	FORCE_INLINE OpAction OpAct(short idx)
	{
		return G::I().R().OpAct(idx);
	}
public:
	Parser();
	~Parser();
	bool Init(OpRegistry* reg = nullptr);
	bool Compile(AST::Module* pModule,char* code, int size);
	AST::Module* GetModule();
};
}