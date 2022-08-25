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
