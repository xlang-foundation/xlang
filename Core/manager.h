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
#include "port.h"
#include "xproxy.h"
#include <iostream>

namespace X 
{
	class XlangRuntime;
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
			XProxyFilter filter = nullptr;
			std::unordered_map<std::string, XProxy*> Instances;
		};
		std::unordered_map<std::string, PackageInfo> m_mapPackage;
		Locker m_proxyMapLock;
		std::unordered_map<std::string, XProxyInfo> m_mapXProxy;
		std::vector<CLEANUP> m_cleanups;
		//if in xlang code, for example use line below
		//import some_module thru 'lrpc:1000'
		//but this lrpc port is hosted by same process runs xlang
		//we need to treat it as import local module, not remote module
		//so m_lrpcPorts will keep these ports called with Lrpc_Listen
		Locker m_lockLrpcPorts;
		std::vector<int> m_lrpcPorts;
	public:
		void AddLrpcPort(int port)
		{
			m_lockLrpcPorts.Lock();
			m_lrpcPorts.push_back(port);
			m_lockLrpcPorts.Unlock();
		}
		void RemoveLrpcPort(int port)
		{
			m_lockLrpcPorts.Lock();
			auto it = m_lrpcPorts.begin();
			while (it != m_lrpcPorts.end())
			{
				if (*it == port)
				{
					m_lrpcPorts.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
			m_lockLrpcPorts.Unlock();
		}
		FORCE_INLINE bool IsLrpcHostedByThisProcess(const char* url)
		{
			bool bFound = false;
			int nPort = 0;
			SCANF(url, "%d", &nPort);
			m_lockLrpcPorts.Lock();
			for (auto it : m_lrpcPorts)
			{
				if (it == nPort)
				{
					bFound = true;
				}
			}			
			m_lockLrpcPorts.Unlock();
			return bFound;
		}
		void Cleanup()
		{
			m_mapPackage.clear();
			m_mapXProxy.clear();
			for (auto f : m_cleanups)
			{
				f();
			}
			m_cleanups.clear();
		}
		void AddCleanupFunc(CLEANUP f)
		{
			m_cleanups.push_back(f);
		}
		bool RegisterProxy(const char* name, XProxyCreator creator,XProxyFilter filter = nullptr)
		{
			m_proxyMapLock.Lock();
			m_mapXProxy.emplace(std::make_pair(name, XProxyInfo{ creator,filter}));
			m_proxyMapLock.Unlock();
			return true;
		}
		void RemoveProxy(const char* name,std::string& rootObjectName,XProxy* pProxy)
		{
			UnloadPackage(rootObjectName);
			std::string strName(name);
			m_proxyMapLock.Lock();
			auto it = m_mapXProxy.find(strName);
			if (it != m_mapXProxy.end())
			{
				auto& proxyInfo = it->second;
				for(auto it2 = proxyInfo.Instances.begin();it2 != proxyInfo.Instances.end();)
				{
					if(it2->second == pProxy)
					{
						proxyInfo.Instances.erase(it2);
						break;
					}
					++it2;
				}
			}
			m_proxyMapLock.Unlock();
		}
		XProxy* QueryProxy(std::string& url,bool& bFilterOut)
		{
			bFilterOut = false;
			std::string proxyName;
			std::string endpoint_url;
			int timeout = -1;

			auto items = splitWithChars(url, ":~");
			if (items.size() >= 1)
			{
				proxyName = items[0];
			}
			if (items.size() >= 2)
			{
				endpoint_url = items[1];
			}
			if (items.size() >= 3)
			{
				timeout = atoi(items[2].c_str());
			}

			XProxy* pProxy = nullptr;
			m_proxyMapLock.Lock();
			auto it = m_mapXProxy.find(proxyName);
			if (it != m_mapXProxy.end())
			{
				auto& proxyInfo = it->second;
				if (proxyInfo.filter && proxyInfo.filter(endpoint_url.c_str()))
				{
					bFilterOut = true;
				}
				else
				{
					auto it2 = proxyInfo.Instances.find(url);
					if (it2 == proxyInfo.Instances.end())
					{
						pProxy = proxyInfo.creator(endpoint_url.c_str());
						proxyInfo.Instances.emplace(std::make_pair(endpoint_url, pProxy));
					}
					else
					{
						pProxy = it2->second;
					}
				}
			}
			m_proxyMapLock.Unlock();
			if (pProxy)
			{
				pProxy->SetTimeout(timeout);
			}
			return pProxy;
		}
		bool Register(const char* name,PackageCreator creator,void* pContextForCreator = nullptr)
		{
			m_mapPackage.emplace(std::make_pair(name, PackageInfo{ creator,Value(pContextForCreator)}));
			return true;
		}
		bool Register(const char* name,Value& objPackage)
		{
			m_mapPackage.emplace(std::make_pair(name, PackageInfo{ nullptr,objPackage }));
			return true;
		}
		bool UnloadPackage(std::string packName)
		{
			auto it = m_mapPackage.find(packName);
			if (it != m_mapPackage.end())
			{
				m_mapPackage.erase(it);
				return true;
			}
			return false;
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
		bool QueryAndCreatePackage(XlangRuntime* rt,std::string& name,
			Value& valPack)
		{
			bool bCreated = false;
			auto it = m_mapPackage.find(name);
			if (it != m_mapPackage.end())
			{
				PackageInfo& info = it->second;
				if (info.creator)
				{
					auto* pPack = info.creator(info.package);
					info.package = pPack;
				}
				bCreated = info.package.IsValid();
				valPack = info.package;

				//X::ObjRef* pRef = dynamic_cast<X::ObjRef*>(valPack.GetObj());
				//std::cout << "QueryAndCreatePackage,query:"<<name<<",RefCount:" <<pRef->Ref()<< std::endl;
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
