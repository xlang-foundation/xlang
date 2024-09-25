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

#include "dbmgr.h"
#include "sqlite/sqlite3.h"
#include "utility.h"
#include "dbop.h"
#include <iostream>
#include "port.h"
#include <regex>

namespace X
{
	namespace Database
	{
		std::string getDirectoryFromPath(const std::string& path)
		{
			size_t pos = path.find_last_of("/\\");
			if (pos != std::string::npos) {
				return path.substr(0, pos);
			}
			return path; // Return the original path if no separator is found
		}
		SqliteDB::SqliteDB()
		{
		}
		SqliteDB::SqliteDB(std::string dbPath)
		{
			if (dbPath.rfind(".db") == std::string::npos)
			{
				dbPath = dbPath + ".db";
			}
			if (!IsAbsPath(dbPath))
			{
				auto* rt = g_pXHost->GetCurrentRuntime();
				std::string curPath;
				//check this .x module path first
				X::Value valXModulePath = rt->GetXModuleFileName();
				if (valXModulePath.IsValid())
				{
					curPath = valXModulePath.ToString();
					curPath = getDirectoryFromPath(curPath);
				}
				else
				{
					curPath = Manager::I().GetCurrentPath();
				}

				dbPath = curPath + Path_Sep_S + dbPath;
			}
			Open(dbPath);
		}
		SqliteDB::~SqliteDB()
		{
		}
		Cursor::~Cursor()
		{
			if (m_stmt)
			{
				delete m_stmt;
				m_stmt = nullptr;
			}
		}
		bool Cursor::Open()
		{
			if (m_stmt == nullptr)
			{
				m_stmt = new DBStatement(m_db, m_sql.c_str());
				if (m_stmt->SC() != (int)DBState::Ok)
				{
					return X::Value(false);
				}
				if (m_BindingDataList.IsList())
				{
					XList* list = dynamic_cast<XList*>(m_BindingDataList.GetObj());
					auto size = m_BindingDataList.Size();
					for (int i = 0; i < size; i++)
					{
						X::Value v0 = list->Get(i);
						m_stmt->bind(i + 1, v0);
					}
				}
				m_colNum = m_stmt->getcolnum();
			}
			return true;
		}
		X::Value  Cursor::GetCols()
		{
			if (!Open())
			{
				return X::Value(false);
			}
			X::List list;
			for (int i = 0; i < m_colNum; i++)
			{
				auto name = m_stmt->getColName(i);
				list += name;
			}
			return list;
		}
		X::Value Cursor::fetch()
		{
			if (!Open())
			{
				return X::Value(false);
			}
			if(m_stmt->step() == DBState::Row)
			{
				X::List list;
				for (int i = 0; i < m_colNum; i++)
				{
					X::Value colVal;
					m_stmt->getValue(i, colVal);
					list += colVal;
				}
				return list;
			}
			else
			{
				return X::Value();//None
			}
		}
		bool Manager::LiteParseStatement(std::string& strSql,
			std::string& varName,std::string& outSql)
		{
			bool bHaveVar = false;
			std::regex var_regex("^[\t ]*(\\w+)[\t ]*=");
			std::smatch matches;
			if (std::regex_search(strSql, matches, var_regex))
			{
				if (matches.size() > 1)
				{
					int matchedSize = (int)matches[0].str().size();
					outSql = strSql.substr(matchedSize);
					varName = matches[1].str();
					bHaveVar = true;
				}
			}
			return bHaveVar;
		}
		bool Manager::RunSQLStatement(X::XRuntime* rt, X::XObj* pContext,
			std::string& strSql,X::Value& BindingDataList)
		{
			DBStatement dbsmt(&m_db, strSql.c_str());
			if (dbsmt.SC() != (int)DBState::Ok)
			{
				std::cout << "Error code:" << dbsmt.SC() << std::endl;
				return false;
			}
			if (BindingDataList.IsList())
			{
				XList* list = dynamic_cast<XList*>(BindingDataList.GetObj());
				auto size = BindingDataList.Size();
				for (int i = 0; i < size; i++)
				{
					X::Value v0 = list->Get(i);
					dbsmt.bind(i + 1, v0);
				}
			}
			int colNum = dbsmt.getcolnum();
			std::cout << "Col:" << colNum << std::endl;
			for (int i = 0; i < colNum; i++)
			{
				auto name = dbsmt.getColName(i);
				std::cout << name << '\t';
			}
			std::cout << std::endl;
			while (dbsmt.step() == DBState::Row)
			{
				for (int i = 0; i < colNum; i++)
				{
					std::string v0;
					dbsmt.getValue(i, v0);
					std::cout << v0 << '\t';
				}
				std::cout << std::endl;
			}
			return true;
		}
		X::Value Manager::WritePad(X::XRuntime* rt, X::XObj* pContext,
			X::Value& input, X::Value& BindingDataList)
		{
			if (input.IsInvalid())
			{
				//this WritePad poped
				m_db.Close();
				return X::Value(true);
			}
			bool bOK = false;
			std::string strSql = input.ToString();
			std::cout << "Sql:" << input.ToString() << std::endl;
			trim(strSql);
			if (strSql.find_last_of(";") != std::string::npos)
			{
				strSql = strSql.substr(0, strSql.size() - 1);
			}
			size_t pos = strSql.find("cd ");
			if (pos != std::string::npos)
			{
				std::string path = strSql.substr(pos + 3);
				if (IsAbsPath(path))
				{
					m_curPath = path;
				}
				else if(!m_curPath.empty())
				{
					m_curPath += Path_Sep_S + path;
				}
				else
				{
					std::string modulePath;
					//check this .x module path first
					X::Value valXModulePath = rt->GetXModuleFileName();
					if (valXModulePath.IsValid())
					{
						modulePath = valXModulePath.ToString();
						modulePath = getDirectoryFromPath(modulePath);
					}
					else
					{
						modulePath = Manager::I().GetCurrentPath();
					}

					m_curPath = modulePath + Path_Sep_S + path;
				}
				return X::Value(true);
			}
			pos = strSql.find("USE ");
			if (pos != std::string::npos)
			{
				std::string dbPath = strSql.substr(pos+4);
				if (dbPath.rfind(".db") == std::string::npos)
				{
					dbPath = dbPath + ".db";
				}
				if (!IsAbsPath(dbPath))
				{
					if (m_curPath.empty())
					{
						std::string curPath;
						//check this .x module path first
						X::Value valXModulePath = rt->GetXModuleFileName();
						if (valXModulePath.IsValid())
						{
							curPath = valXModulePath.ToString();
							curPath = getDirectoryFromPath(curPath);
						}
						else
						{
							curPath = Manager::I().GetCurrentPath();
						}
						dbPath = curPath + Path_Sep_S + dbPath;
					}
					else
					{
						dbPath = m_curPath + Path_Sep_S + dbPath;
					}
				}
				m_db.Open(dbPath);
				if (m_db.db())
				{
					bOK = true;
				}
			}
			else
			{
				//regular sql statement
				//but we suppport this syntax
				//% var1 = SEELCT ....
				std::string varName;
				std::string outSql;
				bool bHaveAssign = LiteParseStatement(strSql, varName, outSql);
				if (bHaveAssign)
				{
					Cursor* pCursor = new Cursor(outSql);
					pCursor->SetDb(&m_db);
					pCursor->SetBindings(BindingDataList);
					X::Value val = X::Value(pCursor->APISET().GetProxy(pCursor),false);
					rt->AddVar(varName.c_str(), val);
					m_cursors.push_back(pCursor);
				}
				else
				{
					bOK = RunSQLStatement(rt, pContext, strSql, BindingDataList);
				}
			}
			return X::Value(bOK);
		}
		bool SqliteDB::Open(std::string dbPath)
		{
			Close();
			mDbPath = dbPath;

			sqlite3* db = nullptr;
			int rc;

			rc = sqlite3_open(dbPath.c_str(), &db);
			if (rc)
			{
				sqlite3_close(db);
			}
			else
			{
				mdb = db;
			}
			return true;
		}

		bool SqliteDB::Close()
		{
			if (mdb)
			{
				sqlite3_close(mdb);
				mdb = nullptr;
			}
			return true;
		}
		int exec_callback(void* NotUsed, int argc, char** argv, char** azColName)
		{
			return 0;
		}
		int SqliteDB::ExecSQL(std::string sql)
		{
			char* zErrMsg = 0;
			int rc = sqlite3_exec(mdb, sql.c_str(), exec_callback, 0, &zErrMsg);
			return rc;
		}
	}
}