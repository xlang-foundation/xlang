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
		class DBStatement;
		class Cursor
		{
			SqliteDB* m_db = nullptr;
			DBStatement* m_stmt = nullptr;
			X::Value m_BindingDataList;
			int m_colNum = 0;
			std::string m_sql;
			BEGIN_PACKAGE(Cursor)
				APISET().AddFunc<0>("fetch",&Cursor::fetch);
				APISET().AddProp("cols", &Cursor::GetCols);
				END_PACKAGE
		bool Open();
		public:
			Cursor(std::string strSql)
			{
				m_sql = strSql;
			}
			~Cursor();
			void SetDb(SqliteDB* db)
			{
				m_db = db;
			}
			void SetBindings(X::Value& bindings)
			{
				m_BindingDataList = bindings;
			}
			X::Value fetch();
			X::Value  GetCols();

		};
		class Manager :
			public Singleton<Manager>
		{
			std::vector<Cursor*> m_cursors;
			X::Value m_curModule;
			SqliteDB m_db;
			std::string m_curPath;
			BEGIN_PACKAGE(Manager)
				APISET().AddFunc<0>("WritePadUseDataBinding",
					&Manager::WritePadUseDataBinding);
				APISET().AddRTFunc<2>("WritePad", &Manager::WritePad);
				APISET().AddClass<1, Cursor>("Cursor");
			END_PACKAGE
			bool RunSQLStatement(X::XRuntime* rt, X::XObj* pContext,
				std::string& strSql,X::Value& BindingDataList);
			bool LiteParseStatement(std::string& strSql,std::string& varName,
				std::string& outSql);
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
			X::Value WritePad(X::XRuntime* rt, X::XObj* pContext,
				X::Value& input, X::Value& BindingDataList);
		};
	}
}
