#pragma once
#include <string>
#include <unordered_map>
#include "exp.h"
#include <vector>
#include "Locker.h"
#include "singleton.h"

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
		public XPackage,
		public Singleton<Builtin>
	{
		std::string m_libName;
		Locker m_lock;
		std::unordered_map<std::string,int> m_mapNameToIndex;
		std::vector<BuiltinFuncInfo> m_Funcs;
	public:
		std::vector<BuiltinFuncInfo>& All()
		{
			m_lock.Lock();
			return m_Funcs;
		}
		void ReturnMap()
		{
			m_lock.Unlock();
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
		virtual void SetPackageCleanupFunc(PackageCleanup func) override {}
		virtual void SetPackageWaitFunc(PackageWaitFunc func) override {}
		virtual void SetAPISet(void* pApiSet) override {}
		virtual int AddMember(PackageMemberType type, const char* name, const char* doc, bool keepRawParams = false) override;
		virtual int QueryMethod(const char* name, bool* pKeepRawParams) override;
		virtual void* GetEmbedObj() override;
		virtual bool Init(int varNum) override;
		virtual bool SetIndexValue(int idx, Value& v) override;
		virtual bool GetIndexValue(int idx, Value& v) override;
		virtual void RemoveALl() override;
	};
}