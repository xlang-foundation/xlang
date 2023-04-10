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