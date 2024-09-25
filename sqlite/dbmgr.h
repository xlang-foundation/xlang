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
#include "xpackage.h"
#include "xlang.h"
#include "singleton.h"
#include "dbop.h"
#include "sqlite/sqlite3.h"

struct sqlite3;

namespace X
{
	namespace Database
	{
		class SqliteDB
		{
			BEGIN_PACKAGE(SqliteDB)
				APISET().AddFunc<1>("open", &SqliteDB::Open);
				APISET().AddFunc<0>("close", &SqliteDB::Close);
				APISET().AddFunc<1>("exec", &SqliteDB::ExecSQL);
				APISET().AddFunc<0>("beginTransaction", &SqliteDB::BeginTransaction);
				APISET().AddFunc<0>("endTransaction", &SqliteDB::EndTransaction);
				APISET().AddClass<1, Database::DBStatement, SqliteDB>("statement");
			END_PACKAGE
		public:
			SqliteDB();
			SqliteDB(std::string dbPath);
			~SqliteDB();

			bool Open(std::string dbPath);
			bool Close();
			sqlite3* db()
			{
				return mdb;
			}
			bool BeginTransaction()
			{
				return SQLITE_OK == ExecSQL("BEGIN TRANSACTION;");
			}
			bool EndTransaction()
			{
				return SQLITE_OK == ExecSQL("END TRANSACTION;");
			}
			int ExecSQL(std::string sql);
		private:
			sqlite3* mdb = nullptr;
			std::string mDbPath;
		};
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
				APISET().AddConst("OK", SQLITE_OK);
				APISET().AddConst("ERROR", SQLITE_ERROR);
				APISET().AddConst("ABORT", SQLITE_ABORT);
				APISET().AddConst("BUSY", SQLITE_BUSY);
				APISET().AddConst("ROW", SQLITE_ROW);
				APISET().AddConst("DONE", SQLITE_DONE);
				APISET().AddFunc<0>("WritePadUseDataBinding",
					&Manager::WritePadUseDataBinding);
				APISET().AddRTFunc<2>("WritePad", &Manager::WritePad);
				APISET().AddClass<1, SqliteDB>("Database");
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
					auto path = pModule->GetPath();
					std::string strPath(path);
					g_pXHost->ReleaseString(path);
					return strPath;
				}
				else
				{
					return "";
				}
			}
			FORCE_INLINE bool WritePadUseDataBinding()
			{
				return true;
			}
			X::Value WritePad(X::XRuntime* rt, X::XObj* pContext,
				X::Value& input, X::Value& BindingDataList);
		};
	}
}
