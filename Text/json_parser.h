#pragma once

#include "token.h"
#include "def.h"

namespace X
{
	namespace Text
	{
		class Json
		{
			Token* mToken = nullptr;
			static std::vector<OpInfo> OPList;
			static std::vector<short> kwTree;
			static std::vector<OpAction> OpActions;
		public:
			Json();
			~Json();
			bool Init();
			bool LoadFromString(char* code, int size);
			bool Parse();
		};
	}
}
