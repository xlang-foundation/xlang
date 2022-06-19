#pragma once
#include <string>
#include <unordered_map>
#include "exp.h"
#include <vector>
#include "singleton.h"
#include "module.h"
#include "package.h"
namespace X 
{
	typedef AST::Package* (*PackageCreator)();
	class Manager :
		public Singleton<Manager>
	{
		struct PackageInfo
		{
			PackageCreator creator = nullptr;
			AST::Package* package = nullptr;
		};
		std::unordered_map<std::string, PackageInfo> m_mapPackage;
	public:
		bool Register(const char* name, PackageCreator creator)
		{
			m_mapPackage.emplace(std::make_pair(name, PackageInfo{ creator,nullptr }));
			return true;
		}
		bool QueryAndCreatePackage(std::string& name, AST::Package** ppPackage)
		{
			bool bCreated = false;
			auto it = m_mapPackage.find(name);
			if (it != m_mapPackage.end())
			{
				PackageInfo& info = it->second;
				if (info.package == nullptr)
				{
					info.package = info.creator();
				}
				bCreated = (info.package != nullptr);
				*ppPackage = info.package;
			}
			return bCreated;
		}
	};
}
