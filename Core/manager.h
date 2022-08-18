#pragma once
#include <string>
#include <unordered_map>
#include "exp.h"
#include <vector>
#include "singleton.h"
#include "module.h"
#include "package.h"
#include "xlang.h"

namespace X 
{
	class Runtime;
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
				if (it.second.package)
				{
					it.second.package->Release();
				}
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
					auto* pPack = info.creator(rt);
					pPack->IncRef();
					info.package = dynamic_cast<AST::Package*>(pPack);
				}
				bCreated = (info.package != nullptr);
				*ppPackage = info.package;
			}
			return bCreated;
		}
		bool HasPackage(std::string& name)
		{
			auto it = m_mapPackage.find(name);
			return (it != m_mapPackage.end());
		}
	};
}
