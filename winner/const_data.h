#pragma once
#include "singleton.h"
#include <unordered_map>
#include <string>

namespace XWin
{
	class Data :
		public Singleton<Data>
	{
	public:
		std::unordered_map<std::string, unsigned int>& Color();
	};
}