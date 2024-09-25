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

#include "dbop.h"
#include "dbmgr.h"
#include "utility.h"
#include "sqlite/sqlite3.h"

namespace X
{
	namespace Database
	{
		DBStatement::DBStatement()
		{
		}
		DBStatement::DBStatement(SqliteDB* pdb,std::string sql)
		{
			statecode = sqlite3_prepare(
				pdb->db(),
				sql.c_str(),
				-1,
				&stmt,
				0  // Pointer to unused portion of stmt
			);
		}


		DBStatement::~DBStatement()
		{
			if (stmt)
			{
				sqlite3_finalize(stmt);
				stmt = nullptr;
			}
		}
		bool DBStatement::Close()
		{
			if (stmt)
			{
				sqlite3_finalize(stmt);
				stmt = nullptr;
			}
			return true;
		}
		bool DBStatement::bind(int idx, X::Value& val)
		{
			bool bOK = true;
			ValueType t = val.GetType();
			switch (t)
			{
			case X::ValueType::Invalid:
				break;
			case X::ValueType::None:
				break;
			case X::ValueType::Int64:
				bOK = bindint64(idx, (long long)val);
				break;
			case X::ValueType::Double:
				bOK = binddouble(idx, (double)val);
				break;
			case X::ValueType::Object:
			{
				auto obType = val.GetObj()->GetType();
				switch (obType)
				{
				case X::ObjType::Base:
					break;
				case X::ObjType::Str:
				{
					std::wstring ws = s2ws(val.ToString());
					bOK = bindtext(idx, ws);
				}
					break;
				case X::ObjType::Binary:
					break;
				case X::ObjType::Expr:
					break;
				case X::ObjType::Function:
					break;
				case X::ObjType::MetaFunction:
					break;
				case X::ObjType::XClassObject:
					break;
				case X::ObjType::Prop:
					break;
				case X::ObjType::ObjectEvent:
					break;
				case X::ObjType::FuncCalls:
					break;
				case X::ObjType::Package:
					break;
				case X::ObjType::ModuleObject:
					break;
				case X::ObjType::Future:
					break;
				case X::ObjType::List:
					break;
				case X::ObjType::Dict:
					break;
				case X::ObjType::TableRow:
					break;
				case X::ObjType::Table:
					break;
				case X::ObjType::RemoteObject:
					break;
				case X::ObjType::PyProxyObject:
					break;
				default:
					break;
				}
			}
				break;
			case X::ValueType::Str:
			{
				std::wstring ws = s2ws(val.ToString());
				bOK = bindtext(idx, ws);
			}
				break;
			case X::ValueType::Value:
				break;
			default:
				break;
			}
			return bOK;
		}
		bool DBStatement::bindtext(int idx, std::wstring str)
		{
			if (statecode != SQLITE_OK)
			{
				return false;
			}
			std::string utf8str = ws2s(str);
			statecode = sqlite3_bind_text(
				stmt,
				idx,  // Index of wildcard
				utf8str.c_str(),
				(int)utf8str.length(),  // length of text
				SQLITE_TRANSIENT
			);
			return (statecode == SQLITE_OK);
		}

		bool DBStatement::bindblob(int idx, const char* pData, int nData)
		{
			if (statecode != SQLITE_OK)
			{
				return false;
			}
			statecode = sqlite3_bind_blob(
				stmt,
				idx,
				(const void*)pData,
				nData, SQLITE_TRANSIENT
			);
			return (statecode == SQLITE_OK);
		}

		bool DBStatement::bindint(int idx, int val)
		{
			if (statecode != SQLITE_OK)
			{
				return false;
			}
			statecode = sqlite3_bind_int(
				stmt,
				idx,
				val
			);
			return (statecode == SQLITE_OK);
		}
		bool DBStatement::binddouble(int idx, double val)
		{
			if (statecode != SQLITE_OK)
			{
				return false;
			}
			statecode = sqlite3_bind_double(
				stmt,
				idx,
				val
			);
			return (statecode == SQLITE_OK);
		}
		bool DBStatement::bindint64(int idx, long long val)
		{
			if (statecode != SQLITE_OK)
			{
				return false;
			}
			statecode = sqlite3_bind_int64(
				stmt,
				idx,
				val
			);
			return (statecode == SQLITE_OK);
		}
		int DBStatement::getcolnum()
		{
			return sqlite3_column_count(stmt);
		}

		DBState DBStatement::step()
		{
			statecode = sqlite3_step(stmt);
			return (DBState)statecode;
		}

		bool DBStatement::reset()
		{
			return SQLITE_OK == sqlite3_reset(stmt);
		}
		bool DBStatement::getValue(int idx, X::Value& val)
		{//SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, or SQLITE_NULL
			int iType = sqlite3_column_type(stmt, idx);
			switch (iType)
			{
			case SQLITE_INTEGER:
				val = sqlite3_column_int64(stmt, idx);
				break;
			case SQLITE_FLOAT:
				val = sqlite3_column_double(stmt, idx);
				break;
			case SQLITE_TEXT:
			{
				const char* retstr = NULL;
				retstr = (const char*)sqlite3_column_text(stmt, idx);
				if (retstr == NULL)
				{
					return false;
				}
				val = retstr;
			}
				break;
			case SQLITE_BLOB:
				break;			
			case SQLITE_NULL:
				break;	
			default:
				break;
			}
			return true;
		}
		bool DBStatement::getValue(int idx, std::wstring& val)
		{
			const char* retstr = NULL;
			retstr = (const char*)sqlite3_column_text(stmt, idx);
			if (retstr == NULL)
			{
				return false;
			}
			std::string txt((const char*)retstr);
			val = s2ws(txt);
			return true;
		}

		bool DBStatement::getValue(int idx, std::string& val)
		{
			const char* retstr = NULL;
			retstr = (const char*)sqlite3_column_text(stmt, idx);
			if (retstr == NULL)
			{
				return false;
			}
			val = retstr;
			return true;
		}

		bool DBStatement::blob2text(const void* blob,
			int size, std::wstring& val)
		{
			std::string txt((const char*)blob, size);
			val = s2ws(txt);
			return true;
		}

		const void* DBStatement::getBlobValue(int idx)
		{
			return sqlite3_column_blob(stmt, idx);
		}

		int DBStatement::getBlobSize(int idx)
		{
			return sqlite3_column_bytes(stmt, idx);
		}

		std::string DBStatement::getColName(int idx)
		{
			const char* name = NULL;
			name = (const char*)sqlite3_column_name(
				stmt, idx);
			if (name == NULL)
			{
				return "";
			}
			std::string txt((const char*)name);
			return txt;
		}

		int DBStatement::getValue(int idx)
		{
			return sqlite3_column_int(stmt, idx);
		}

		long long DBStatement::getInt64Value(int idx)
		{
			return sqlite3_column_int64(stmt, idx);
		}

		double DBStatement::getDouble(int idx)
		{
			return sqlite3_column_double(stmt, idx);
		}

		bool DBStatement::isNull(int idx)
		{
			return sqlite3_column_type(stmt, idx) == SQLITE_NULL;
		}

	}
}