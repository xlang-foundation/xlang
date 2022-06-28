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
	class Runtime;
	typedef AST::Package* (*PackageCreator)(Runtime* rt);
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
		void Cleanup()
		{
			for (auto& it : m_mapPackage)
			{
				it.second.package->Release();
			}
			m_mapPackage.clear();
		}
		bool Register(const char* name, PackageCreator creator)
		{
			m_mapPackage.emplace(std::make_pair(name, PackageInfo{ creator,nullptr }));
			return true;
		}
		bool QueryAndCreatePackage(Runtime* rt,std::string& name,
			AST::Package** ppPackage)
		{
			bool bCreated = false;
			auto it = m_mapPackage.find(name);
			if (it != m_mapPackage.end())
			{
				PackageInfo& info = it->second;
				if (info.package == nullptr)
				{
					info.package = info.creator(rt);
				}
				bCreated = (info.package != nullptr);
				*ppPackage = info.package;
			}
			return bCreated;
		}
	};
}
