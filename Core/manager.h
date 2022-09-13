#pragma once
#include <string>
#include <unordered_map>
#include "exp.h"
#include <vector>
#include "singleton.h"
#include "module.h"
#include "package.h"
#include "xlang.h"
#include "Locker.h"

namespace X 
{
	class Runtime;
	class XProxy;
	class Manager :
		public Singleton<Manager>
	{
		struct PackageInfo
		{
			PackageCreator creator = nullptr;
			Value package;
		};
		struct XProxyInfo
		{
			XProxyCreator creator = nullptr;
			std::unordered_map<std::string, XProxy*> Instances;
		};
		std::unordered_map<std::string, PackageInfo> m_mapPackage;
		Locker m_proxyMapLock;
		std::unordered_map<std::string, XProxyInfo> m_mapXProxy;
	public:
		void Cleanup()
		{
			m_mapPackage.clear();
			m_mapXProxy.clear();
		}
		bool RegisterProxy(const char* name, XProxyCreator creator)
		{
			m_proxyMapLock.Lock();
			m_mapXProxy.emplace(std::make_pair(name, XProxyInfo{ creator }));
			m_proxyMapLock.Unlock();
			return true;
		}
		XProxy* QueryProxy(std::string& url)
		{
			std::string proxyName;
			std::string endpoint_url;
			auto pos = url.find(':');
			if (pos != url.npos)
			{
				proxyName = url.substr(0, pos);
				endpoint_url = url.substr(pos + 1);
			}
			else
			{
				proxyName = url;
			}

			XProxy* pProxy = nullptr;
			m_proxyMapLock.Lock();
			auto it = m_mapXProxy.find(proxyName);
			if (it != m_mapXProxy.end())
			{
				auto& proxyInfo = it->second;
				auto it2 = proxyInfo.Instances.find(url);
				if (it2 == proxyInfo.Instances.end())
				{
					pProxy = proxyInfo.creator(url);
					proxyInfo.Instances.emplace(std::make_pair(url,pProxy));
				}
				else
				{
					pProxy = it2->second;
				}
			}
			m_proxyMapLock.Unlock();
			return pProxy;
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
		bool QueryPackage(std::string& name,Value& valPack)
		{
			bool bHave = false;
			auto it = m_mapPackage.find(name);
			if (it != m_mapPackage.end())
			{
				PackageInfo& info = it->second;
				bHave = info.package.IsValid();
				valPack = info.package;
			}
			return bHave;
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
					auto* pPack = info.creator();
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
