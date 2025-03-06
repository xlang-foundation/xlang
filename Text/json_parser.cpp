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

#include "json_parser.h"
#include "lex.h"
#include <iostream>

namespace X
{
	namespace Text
	{
		std::vector<OpInfo> Json::OPList =
		{
			{0,":"},{1,","},
			{2,"{"},{3,"}"},
			{4,"["},{5,"]"}
		};
		std::vector<short> Json::kwTree;
		std::vector<OpAction> Json::OpActions;

		Json::Json()
		{
		}

		Json::~Json()
		{
			if (mToken)
			{
				delete mToken;
			}
		}

		bool Json::Init()
		{
			Lex<OpInfo, OpAction>().MakeLexTree(
				OPList, kwTree, OpActions);
			mToken = new Token(&kwTree[0]);
			return true;
		}
		bool Json::LoadFromString(char* code, int size)
		{
			mToken->SetStream(code, size);
			return Parse();
		}
		bool Json::Parse()
		{
			while (true)
			{
				String s;
				int leadingSpaceCnt = 0;
				OneToken one;
				short idx = mToken->Get(one);
				int startLine = one.lineStart;
				s = one.id;
				if (idx == TokenEOS)
				{
					break;
				}
				std::string txt(s.s, s.size);
				std::cout << "token:" << txt << ",idx:" << idx << ",line:"
					<< one.lineStart << ",pos:" << one.charPos << std::endl;
			}
			return true;
		}
	}
}
