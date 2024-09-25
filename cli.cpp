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

#include "cli.h"
#include <string>
#include "xhost.h"


namespace X
{
	void CLI::MainLoop()
	{
		auto host = X::g_pXHost;
		std::string prompt = ">>> ";
		std::cout << prompt;
		while (m_run)
		{
			std::string input;
			std::getline(std::cin, input);
			if (input == "!code")
			{
				const char* pStrCode =  host->GetInteractiveCode();
				if (pStrCode)
				{
					std::string code = pStrCode;
					host->ReleaseString(pStrCode);
					std::cout << code <<std::endl;
				}
				std::cout << prompt;
				continue;
			}
			if (input == "!quit")
			{
				break;
			}
			X::Value retValue;
			bool bOK = host->RunCodeLine(input.c_str(),(int)input.size(),retValue);
			if (!bOK)
			{

			}
			if (retValue.IsValid())
			{
				std::cout << retValue.ToString() << std::endl;
			}
			std::cout << prompt;
		}
	}
}