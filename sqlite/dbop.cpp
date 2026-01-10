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
			statecode = sqlite3_prepare_v2(
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
		bool DBStatement::bindNull(int idx)
		{
			return sqlite3_bind_null(stmt, idx) == SQLITE_OK;
		}
		bool DBStatement::bind(int idx, X::Value& val)
		{
			bool bOK = true;
			ValueType t = val.GetType();
			switch (t)
			{
			case X::ValueType::Invalid:
				bOK = bindNull(idx);
				break;
			case X::ValueType::None:
				bOK = bindNull(idx);
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
			statecode = sqlite3_reset(stmt);
			return SQLITE_OK == statecode;
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
		bool DBStatement::fetchall(X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
		{
			int limit = -1;
			if (params.size() >= 1)
			{
				X::Value vLimit = params[0];
				if (vLimit.IsLong())
				{
					limit = (int)(long long)vLimit;
				}
			}
			else
			{
				auto it = kwParams.find("limit");
				if (it)
				{
					X::Value vLimit = it->val;
					if (vLimit.IsLong())
					{
						limit = (int)(long long)vLimit;
					}
				}
			}
			X::List resultList;
			if (stmt == nullptr)
			{
				return false;
			}

			int colCount = getcolnum();
			int rowCount = 0;

			while (true)
			{
				// Check limit
				if (limit > 0 && rowCount >= limit)
				{
					break;
				}

				DBState state = step();
				if (state != DBState::Row)
				{
					break;
				}

				// Create a list for this row
				X::List rowList;
				for (int i = 0; i < colCount; i++)
				{
					X::Value val;
					getValue(i, val);
					rowList += val;
				}

				resultList->append(rowList);
				rowCount++;
			}
			retValue = resultList;
			return true;
		}

		bool DBStatement::fetchallDict(X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
		{
			int limit = -1;
			if (params.size() >= 1)
			{
				X::Value vLimit = params[0];
				if (vLimit.IsLong())
				{
					limit = (int)(long long)vLimit;
				}
			}
			else
			{
				auto it = kwParams.find("limit");
				if (it)
				{
					X::Value vLimit = it->val;
					if (vLimit.IsLong())
					{
						limit = (int)(long long)vLimit;
					}
				}
			}

			X::List resultList;
			if (stmt == nullptr)
			{
				return false;
			}

			int colCount = getcolnum();
			if (colCount == 0)
			{
				return true;
			}

			// Build column names with duplicate handling
			// Step 1: Get raw column names and extract base names (remove table prefix like "o.")
			std::vector<std::string> rawNames(colCount);
			std::vector<std::string> baseNames(colCount);

			for (int i = 0; i < colCount; i++)
			{
				rawNames[i] = getColName(i);
				std::string baseName = rawNames[i];

				// Remove table prefix (e.g., "o.colname" -> "colname")
				size_t dotPos = baseName.find('.');
				if (dotPos != std::string::npos)
				{
					baseName = baseName.substr(dotPos + 1);
				}
				baseNames[i] = baseName;
			}

			// Step 2: Count occurrences of each base name to detect duplicates
			std::unordered_map<std::string, int> nameCount;
			for (int i = 0; i < colCount; i++)
			{
				nameCount[baseNames[i]]++;
			}

			// Step 3: Build final column names
			// If base name is unique, use it directly
			// If base name has duplicates, use prefix_colname format (replace . with _)
			std::vector<std::string> finalNames(colCount);
			std::unordered_map<std::string, int> usedNames; // Track used names for additional disambiguation

			for (int i = 0; i < colCount; i++)
			{
				std::string finalName;

				if (nameCount[baseNames[i]] > 1)
				{
					// Duplicate base name - use full name with . replaced by _
					finalName = rawNames[i];
					// Replace all dots with underscores
					for (size_t j = 0; j < finalName.length(); j++)
					{
						if (finalName[j] == '.')
						{
							finalName[j] = '_';
						}
					}
				}
				else
				{
					// Unique base name
					finalName = baseNames[i];
				}

				// Handle edge case: if the transformed name still conflicts
				if (usedNames.find(finalName) != usedNames.end())
				{
					// Append index to make it unique
					usedNames[finalName]++;
					finalName = finalName + "_" + std::to_string(usedNames[finalName]);
				}
				else
				{
					usedNames[finalName] = 0;
				}

				finalNames[i] = finalName;
			}

			// Step 4: Fetch rows and build dictionaries
			int rowCount = 0;

			while (true)
			{
				// Check limit
				if (limit > 0 && rowCount >= limit)
				{
					break;
				}

				DBState state = step();
				if (state != DBState::Row)
				{
					break;
				}

				// Create a dict for this row
				X::Dict rowDict;
				for (int i = 0; i < colCount; i++)
				{
					X::Value val;
					getValue(i, val);
					rowDict->Set(finalNames[i], val);
				}

				resultList->append(rowDict);
				rowCount++;
			}

			retValue = resultList;
			return true;
		}

	}
}