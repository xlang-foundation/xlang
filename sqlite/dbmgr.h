#pragma once
#include <string>
#include "xpackage.h"
#include "xlang.h"
#include "singleton.h"

struct sqlite3;

namespace X
{
	namespace Database
	{
		class SqliteDB
		{
		public:
			SqliteDB();
			~SqliteDB();

			void Open(std::string& dbPath);
			void Close();
			sqlite3* db()
			{
				return mdb;
			}
			int ExecSQL(std::string sql);
		private:
			sqlite3* mdb = nullptr;
			std::string mDbPath;
		};
		class Manager :
			public Singleton<Manager>
		{
			X::Value m_curModule;
			SqliteDB m_db;
			std::string m_curPath;
			BEGIN_PACKAGE(Manager)
				APISET().AddFunc<0>("WritePadUseDataBinding",
					&Manager::WritePadUseDataBinding);
			APISET().AddFunc<2>("WritePad", &Manager::WritePad);
			END_PACKAGE
		public:
			void SetModule(X::Value curModule)
			{
				m_curModule = curModule;
			}
			X::Value& GetModule() { return m_curModule; }
			std::string GetCurrentPath()
			{
				X::XModule* pModule = dynamic_cast<X::XModule*>(m_curModule.GetObj());
				if (pModule)
				{
					return pModule->GetPath();
				}
				else
				{
					return "";
				}
			}
			inline bool WritePadUseDataBinding()
			{
				return true;
			}
			X::Value WritePad(X::Value& input, X::Value& BindingDataList);
		};
	}
}
