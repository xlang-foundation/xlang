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
#include "Locker.h"
#include "singleton.h"
#include "object.h"

namespace X {
	namespace Data
	{
		class Function;
	}
	struct BuiltinFuncInfo
	{
		std::string name;
		Data::Function* funcObj = nullptr;
	};
	class Builtin :
		virtual public XPackage,
		virtual public Data::Object
		//public Singleton<Builtin>
	{
		std::string m_libName;
		Locker m_lock;
		std::unordered_map<std::string,int> m_mapNameToIndex;
		std::vector<BuiltinFuncInfo> m_Funcs;
	public:
		Builtin()
		{
			m_t = X::ObjType::Package;
		}
		static Builtin& I();

		std::vector<BuiltinFuncInfo>& All()
		{
			m_lock.Lock();
			return m_Funcs;
		}
		void ReturnMap()
		{
			m_lock.Unlock();
		}
		virtual const char* GetTypeString() override
		{
			std::string typeName = "Package";
			return GetABIString(typeName);
		}
		virtual int GetPackageName(char* buffer, int bufferSize) override
		{
			std::string myName("Builtin");
			if (bufferSize < myName.size() + 1)
				return 0;
			strcpy(buffer, myName.c_str());
			return (int)myName.size();
		}
		virtual bool RunCodeWithThisScope(const char* code) override
		{
			return true;
		}
		void SetLibName(std::string& name)
		{
			m_libName = name;
		}
		virtual bool IsSamePackage(XPackage* pPack) override
		{
			return false;
		}
		void Cleanup();
		Data::Function* Find(std::string& name);
		bool Register(const char* name, X::U_FUNC func,
			std::vector<std::pair<std::string, std::string>>& params,
			const char* doc = "",
			bool regToMeta=false);
		bool RegisterWithScope(const char* name, X::U_FUNC func,
			AST::Scope* pScope,
			std::vector<std::pair<std::string, std::string>>& params,
			const char* doc = "",
			bool regToMeta = false);
		bool RegisterInternals();

		// Inherited via XPackage
		virtual void SetPackageAccessor(PackageAccessor func) override {}
		virtual void SetPackageCall(U_FUNC func) override {}
		virtual void SetPackageCleanupFunc(PackageCleanup func) override {}
		virtual void SetPackageWaitFunc(PackageWaitFunc func) override {}
		virtual void SetAPISet(void* pApiSet) override {}
		virtual int AddMember(PackageMemberType type, const char* name, const char* doc, bool keepRawParams = false) override;
		virtual int QueryMethod(const char* name, int* pFlags) override;
		virtual void* GetEmbedObj() override;
		virtual void SetEmbedObj(void* p) override {}
		virtual bool Init(int varNum) override;
		virtual bool SetIndexValue(int idx, Value& v) override;
		virtual bool GetIndexValue(int idx, Value& v) override;
		virtual void RemoveALl() override;
	};
}