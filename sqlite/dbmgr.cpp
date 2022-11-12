#include "dbmgr.h"
#include "sqlite/sqlite3.h"
#include "utility.h"
#include "dbop.h"
#include <iostream>
#include "port.h"

namespace X
{
	namespace Database
	{
		SqliteDB::SqliteDB()
		{
		}

		SqliteDB::~SqliteDB()
		{
		}
		X::Value Manager::WritePad(X::Value& input, X::Value& BindingDataList)
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
					auto modulePath = Manager::I().GetCurrentPath();
					m_curPath = modulePath + Path_Sep_S + path;
				}
				return X::Value(true);
			}
			pos = strSql.find("USE ");
			if (pos != std::string::npos)
			{
				std::string dbPath = strSql.substr(pos+4);
				if (dbPath.find(".") == std::string::npos)
				{
					dbPath = dbPath + ".db";
				}
				if (!IsAbsPath(dbPath))
				{
					if (m_curPath.empty())
					{
						auto path = Manager::I().GetCurrentPath();
						dbPath = path + Path_Sep_S + dbPath;
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
				DBStatement dbsmt(&m_db, strSql.c_str());
				if (dbsmt.SC() != (int)DBState::Ok)
				{
					std::cout << "Error code:" << dbsmt.SC() << std::endl;
					return X::Value(false);
				}
				if (BindingDataList.IsList())
				{
					XList* list = dynamic_cast<XList*>(BindingDataList.GetObj());
					auto size = BindingDataList.Size();
					for (int i = 0; i < size; i++)
					{
						X::Value v0 = list->Get(i);
						dbsmt.bind(i+1, v0);
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
			}
			return X::Value(bOK);
		}
		void SqliteDB::Open(std::string& dbPath)
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
		}

		void SqliteDB::Close()
		{
			if (mdb)
			{
				sqlite3_close(mdb);
				mdb = nullptr;
			}
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