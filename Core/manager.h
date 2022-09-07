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
			Value package;
		};
		std::unordered_map<std::string, PackageInfo> m_mapPackage;
	public:
		void Cleanup()
		{
			m_mapPackage.clear();
		}
		bool Register(const char* name, PackageCreator creator)
		{
			m_mapPackage.emplace(std::make_pair(name, PackageInfo{ creator,Value()}));
			return true;
		}
		bool Register(const char* name, Value& objPackage)
		{
			m_mapPackage.emplace(std::make_pair(name, PackageInfo{ nullptr,objPackage }));
			return true;
		}
		bool QueryAndCreatePackage(Runtime* rt,std::string& name,
			Value& valPack)
		{
			bool bCreated = false;
			auto it = m_mapPackage.find(name);
			if (it != m_mapPackage.end())
			{
				PackageInfo& info = it->second;
				if (info.package.IsInvalid())
				{
					auto* pPack = info.creator(rt);
					info.package = pPack;
				}
				bCreated = info.package.IsValid();
				valPack = info.package;
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
