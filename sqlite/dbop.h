#pragma once
#include <string>
#include "value.h"
#include "xpackage.h"

struct sqlite3;
struct sqlite3_stmt;


namespace X
{
	namespace Database
	{
		enum class DBState
		{
			Ok = 0,//SQLITE_OK
			Row = 100,//SQLITE_ROW
			Done = 101,//SQLITE_DONE
		};
		class SqliteDB;
		class DBStatement
		{
			BEGIN_PACKAGE(DBStatement)
				APISET().AddFunc<2>("bind", &DBStatement::bind);
				APISET().AddFunc<0>("step", &DBStatement::Step);
				APISET().AddFunc<1>("get", &DBStatement::GetValue);
				APISET().AddFunc<0>("reset", &DBStatement::reset);
				APISET().AddFunc<0>("close", &DBStatement::Close);
			END_PACKAGE
		public:
			DBStatement();
			DBStatement(SqliteDB* pdb,std::string sql);
			~DBStatement();
			bool Close();
			bool bind(int idx,X::Value& val);
			bool bindtext(int idx, std::wstring str);
			bool bindblob(int idx, const char* pData, int nData);
			bool bindint(int idx, int val);
			bool binddouble(int idx, double val);
			bool bindint64(int idx, long long val);
			int getcolnum();
			inline int Step() { return (int)step(); }
			DBState step();
			bool reset();
			bool getValue(int idx, std::wstring& val);
			bool getValue(int idx, std::string& val);
			bool blob2text(const void* blob, int size, std::wstring& val);
			std::string getColName(int idx);
			int getValue(int idx);
			inline X::Value GetValue(int idx)
			{
				X::Value v;
				getValue(idx, v);
				return v;
			}
			bool getValue(int idx, X::Value& val);
			long long getInt64Value(int idx);
			double getDouble(int idx);
			bool isNull(int idx);
			const void* getBlobValue(int idx);
			int getBlobSize(int idx);
			int SC()
			{
				return statecode;
			}
		private:
			sqlite3_stmt* stmt = NULL;
			int statecode = 0;
		};
	}
}