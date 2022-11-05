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
	class Builtin :
		public Singleton<Builtin>
	{
		Locker m_lock;
		std::unordered_map<std::string, Data::Function*> m_mapFuncs;
	public:
		std::unordered_map<std::string, Data::Function*>& All()
		{
			m_lock.Lock();
			return m_mapFuncs;
		}
		void ReturnMap()
		{
			m_lock.Unlock();
		}
		void Cleanup();
		Data::Function* Find(std::string& name);
		bool Register(const char* name, X::U_FUNC func,
			std::vector<std::pair<std::string, std::string>>& params,
			const char* doc = "",
			bool regToMeta=false);
		bool RegisterInternals();
	};
}