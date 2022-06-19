#pragma once
#include "module.h"
namespace X
{
	class Http
	{
	public:
		Http()
		{

		}
		bool Create(AST::Package** ppackage)
		{
			auto* pPackage = new AST::Package();
			*ppackage = pPackage;
			return true;
		}
	};
	class HttpServer
	{

	};
}