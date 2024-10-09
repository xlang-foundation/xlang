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
#include <unordered_map>
#include <mutex>
#include "utility.h"

struct sqlite3;

namespace X
{
	namespace Database
	{
		std::string norm_db_path(X::XRuntime* rt, std::string dbPath, std::string curModulePath);
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
			Cursor()
			{
			}
			Cursor(std::string strSql)
			{
				m_sql = strSql;
			}
			~Cursor();
			void SetSql(std::string strSql)
			{
				m_sql = strSql;
			}
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
			std::mutex m_mutexOpenDbs;
			std::unordered_map<std::string, X::Value> m_openDbs;
			std::mutex m_mutexThreadDbStack;
			//threadId->dbStack( item include padIndex+db)
			std::unordered_map<unsigned long, std::vector<std::pair<int,X::Value>>> m_threadDbStack;
			X::Value m_curModule;
			std::string m_curPath;
			X::Value m_defaultDb;

			BEGIN_PACKAGE(Manager)
				APISET().AddConst("OK", SQLITE_OK);
				APISET().AddConst("ERROR", SQLITE_ERROR);
				APISET().AddConst("ABORT", SQLITE_ABORT);
				APISET().AddConst("BUSY", SQLITE_BUSY);
				APISET().AddConst("ROW", SQLITE_ROW);
				APISET().AddConst("DONE", SQLITE_DONE);
				APISET().AddFunc<0>("WritePadUseDataBinding",
					&Manager::WritePadUseDataBinding);
				APISET().AddRTFunc<1>("UseDatabase",&Manager::UseDatabase);
				APISET().AddVarFunc("WritePad", &Manager::WritePad);
				APISET().AddClass<1, SqliteDB>("Database");
				APISET().AddClass<1, Cursor>("Cursor");
			END_PACKAGE
			bool RunSQLStatement(X::XRuntime* rt, X::XObj* pContext,
				std::string& strSql,X::Value& BindingDataList,int pad_index);
			bool LiteParseStatement(std::string& strSql,std::string& varName,
				std::string& outSql);
		public:
			void SetModule(X::Value curModule)
			{
				m_curModule = curModule;
			}
			void PushThreadDbStack(int padIndex,X::Value db)
			{
				auto threadId = GetThreadID();
				std::lock_guard<std::mutex> lock(m_mutexThreadDbStack);
				m_threadDbStack[threadId].push_back(std::pair(padIndex,db));
			}
			X::Value GetThreadDB(int padIndex)
			{
				//check last of stack per this thread
				auto threadId = GetThreadID();
				std::lock_guard<std::mutex> lock(m_mutexThreadDbStack);
				auto it = m_threadDbStack.find(threadId);
				if (it != m_threadDbStack.end())
				{
					auto& dbStack = it->second;
					if (dbStack.size() > 0)
					{
						auto& last = dbStack.back();
						if (last.first == padIndex)
						{
							return last.second;
						}
					}
				}
				return m_defaultDb;
			}
			void PopThreadDbStack(int padIndex)
			{
				//from stack top to bottom if padIndex is found, pop it
				//may have multiple db with same padIndex but need to continue pop
				//if meet padIndex is not the same,break
				auto threadId = GetThreadID();
				std::lock_guard<std::mutex> lock(m_mutexThreadDbStack);
				auto it = m_threadDbStack.find(threadId);
				if (it != m_threadDbStack.end())
				{
					auto& dbStack = it->second;
					if (!dbStack.empty())
					{
						auto it2 = dbStack.rbegin();
						while (it2 != dbStack.rend())
						{
							if (it2->first == padIndex)
							{
								// Convert reverse iterator to forward iterator and erase the element
								it2 = decltype(it2)(dbStack.erase(std::next(it2).base()));
							}
							else
							{
								break;  // Stop the loop if we find an element that doesn't match padIndex
							}
						}
					}
					if (dbStack.empty())
					{
						m_threadDbStack.erase(it);
					}
				}
			}
			X::Value UseDatabase(X::XRuntime* rt, X::XObj* pContext,std::string dbPath)
			{
				dbPath = norm_db_path(rt, dbPath, m_curPath);
				m_mutexOpenDbs.lock();
				auto it = m_openDbs.find(dbPath);
				if (it != m_openDbs.end())
				{
					X::Value valDb = it->second;
					m_mutexOpenDbs.unlock();
					return valDb;
				}
				m_mutexOpenDbs.unlock();

				X::XPackageValue<SqliteDB> packDb;
				SqliteDB* pDb = packDb.GetRealObj();
				pDb->Open(dbPath);
				X::Value valDb(packDb);
				std::lock_guard<std::mutex> lock(m_mutexOpenDbs);
				m_openDbs[dbPath] = valDb;
				m_defaultDb = valDb;
				return valDb;
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
			bool WritePad(X::XRuntime* rt, XObj* pContext,
				ARGS& params, KWARGS& kwParams, X::Value& retValue);
		};
	}
}
