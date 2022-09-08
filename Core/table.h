#pragma once

#include "object.h"
#include "list.h"
#include "port.h"
#include "utility.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <string.h>

namespace X
{
namespace Data
{

class DynVariantAry
{
	char* m_data = nullptr;
	size_t m_count = 0;//current item number
	size_t m_lastTouchedCount = 0;//last row
	size_t m_maxcount = 0;//max item number,include deleted
	size_t m_itemSize = 1;
	size_t m_grownCount = 64;
	ValueType m_valueType = ValueType::None;
	std::vector<size_t> m_deleted;//row records for deleted
public:
	DynVariantAry(size_t cnt,size_t itemSize, ValueType type)
	{
		m_valueType = type;
		m_data = (char*)malloc(cnt* itemSize);
		memset(m_data, 0, cnt * itemSize);
		m_count = 0;
		m_lastTouchedCount = 0;
		m_maxcount = cnt;
		m_itemSize = itemSize;
	}
	~DynVariantAry()
	{
		if (m_valueType == ValueType::Object)
		{
			X::Value* pVals = (X::Value*)m_data;
			for (int i = 0; i < m_lastTouchedCount; i++)
			{
				X::Value& v = pVals[i];
				if (v.IsObject())
				{
					v.GetObj()->DecRef();
				}
			}
		}
		free(m_data);
		m_deleted.clear();
	}
	inline char* Item(size_t r)
	{
		return m_data + m_itemSize * r;
	}
	template<typename T>
	T Get(size_t r)
	{
		return (r < m_count)? *(T*)Item(r):T(0);
	}
	template<typename T>
	T* GetRef(size_t r)
	{
		return (T*)Item(r);
	}

	bool Get(size_t r, X::Value& v)
	{
		switch (m_valueType)
		{
		case X::ValueType::None:
			break;
		case X::ValueType::Int64:
			v = X::Value(Get<long long>(r));
			break;
		case X::ValueType::Double:
			v = X::Value(Get<double>(r));
			break;
		case X::ValueType::Object:
			v = X::Value(Get<X::Value>(r));
			break;
		case X::ValueType::Str:
			v = X::Value(Get<X::Value>(r));
			break;
		case X::ValueType::Value:
			v = X::Value(Get<X::Value>(r));
			break;
		default:
			break;
		}
		return true;
	}
	bool Set(size_t r, X::Value& v)
	{
		switch (m_valueType)
		{
		case X::ValueType::None:
			break;
		case X::ValueType::Int64:
			*GetRef<long long>(r) = v.GetLongLong();
			break;
		case X::ValueType::Double:
			*GetRef<double>(r) = v.GetDouble();
			break;
		case X::ValueType::Object:
		case X::ValueType::Str:
		case X::ValueType::Value:
			*GetRef<X::Value>(r) = v;
		default:
			break;
		}
		return true;
	}
	size_t Add(X::Value& v)
	{
		size_t r = -1;
		auto t = v.GetType();
		switch (t)
		{
		case X::ValueType::None:
			r = Add<X::Value>(v);
			break;
		case X::ValueType::Int64:
			r = Add<long long>(v.GetLongLong());
			break;
		case X::ValueType::Double:
			r = Add<double>(v.GetDouble());
			break;
		case X::ValueType::Object:
			r = Add<X::Value>(v);
			break;
		case X::ValueType::Str:
			r = Add<X::Value>(v);
			break;
		default:
			r = Add<X::Value>(v);
			break;
		}
		return r;
	}
	template<typename T>
	size_t Add(T d)
	{
		if (sizeof(T) > m_itemSize)
		{
			return -1;
		}
		if (m_deleted.size() > 0)
		{
			size_t r = m_deleted[0];
			m_deleted.erase(m_deleted.begin());
			*(T*)Item(r) = d;
			m_count++;
			return r;
		}
		//come to this line,no deleted slot to use
		if (m_count == m_maxcount)
		{
			m_maxcount += m_grownCount;
			m_data = (char*)realloc(m_data, m_maxcount * m_itemSize);
			*(T*)Item(m_count) = d;
		}
		else
		{
			*(T*)Item(m_count) = d;
		}
		m_lastTouchedCount++;
		return m_count++;
	}
	void Remove(size_t r)
	{
		//clear
		if (m_valueType == ValueType::Object)
		{//for late delete this col, no object referenced
			X::Value emptyV;
			Set(r, emptyV);
		}
		m_deleted.push_back(r);
		m_count--;
	}
};
struct ColInfo
{
	std::string name;
	DynVariantAry* ary;
};
class Table;
class TableRow :
	virtual public Object
{
	friend class Table;
	long long m_r = 0;
	long long m_rowId = 0;
	Table* m_table = nullptr;
public:
	TableRow(Table* table,long long id, long long r):
		Object(), XObj(), ObjRef()
	{
		m_table = table;
		m_r = r;
		m_rowId = id;
	}
	long long r() { return m_r; }
	virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue) override
	{
		return true;
	}
	virtual long long Size() override;
	virtual List* FlatPack(Runtime* rt, long long startIndex, long long count) override;
};
class Table
	:virtual public Object
{
	friend class TableRow;

	int m_colNum = 0;
	int m_rowNum = 0;
	long long m_lastRowId = 0;
	ColInfo m_rowIDcol;
	std::vector<ColInfo> m_cols;
	std::unordered_map<std::string, int> m_colMap;
	std::map<long long, TableRow*> m_rowMap;
public:
	Table():Object(), XObj(), ObjRef()
	{
		m_t = ObjType::Table;
		m_rowIDcol.ary = new DynVariantAry(1, sizeof(long long),ValueType::Int64);
	}
	long long GetColNum()
	{
		return m_cols.size();
	}
	~Table()
	{
		if (m_rowIDcol.ary)
		{
			delete m_rowIDcol.ary;
		}
		for (auto i : m_cols)
		{
			if (i.ary)
			{
				delete i.ary;
			}
		}
		m_cols.clear();
		m_colMap.clear();
		for (auto& it : m_rowMap)
		{
			it.second->Release();
		}
		m_rowMap.clear();
	}
	inline virtual long long Size() override
	{
		return m_rowMap.size();
	}
	virtual List* FlatPack(Runtime* rt, long long startIndex, long long count) override;
	virtual Table& operator +=(X::Value& r)
	{
		if (r.IsObject())
		{
			Data::Object* pObjVal = dynamic_cast<Data::Object*>(r.GetObj());
			if (pObjVal->GetType() == X::ObjType::List)
			{
				List* pList = dynamic_cast<List*>(pObjVal);
				FillWithList(pList->Data());
			}
		}
		return *this;
	}
	virtual std::string ToString(bool WithFormat = false) override
	{
		const int online_len = 1000;
		char convertBuf[online_len];

		std::string strOut = "rowid";
		for (auto c : m_cols)
		{
			strOut+="\t"+c.name;
		}
		strOut += "\n";
		for (auto it : m_rowMap)
		{
			SPRINTF(convertBuf,online_len,"%llu",it.first);
			strOut += convertBuf;
			auto rObj = it.second;
			for (auto c : m_cols)
			{
				X::Value v0;
				c.ary->Get(rObj->r(), v0);
				strOut += "\t" + v0.ToString();
			}
			strOut += "\n";
		}
		return strOut;
	}
	bool FillWithList(ARGS& params)
	{
		int colNum = (int)m_cols.size();
		if (colNum == 0)
		{
			return true;//empty table
		}
		int valNum = (int)params.size();
		int rowNum = (valNum + colNum - 1) / colNum;
		for (int i = 0; i < rowNum; i++)
		{
			long long rowID = m_lastRowId++;
			size_t r = m_rowIDcol.ary->Add(rowID);
			size_t base = i * colNum;
			for (int j = 0; j < colNum; j++)
			{
				size_t idx = base + j;
				if (idx < params.size())
				{
					m_cols[j].ary->Add(params[idx]);
				}
			}
			TableRow* rObj = new TableRow(this,rowID,r);
			rObj->AddRef();
			m_rowMap.emplace(std::make_pair(rowID, rObj));
		}
		return true;
	}
	virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue) override
	{
		FillWithList(params);
		retValue = X::Value(this);
		return true;
	}
	bool AddCol(std::string& strName,
		std::vector<std::string>& props,
		X::Value& valDefaultValue)
	{
		std::string type;
		if (props.size() > 0)
		{
			type = props[0];
		}
		ValueType t = ValueType::Object;
		int colSize = sizeof(X::Value);
		if (type == "int64")
		{
			t = ValueType::Int64;
			colSize = sizeof(long long);
		}
		else if (type == "double")
		{
			t = ValueType::Double;
			colSize = sizeof(double);
		}
		else if (type == "str")
		{
			t = ValueType::Object;
		}
		return AddCol(strName,colSize,t);
	}
	bool AddCol(std::string name,int colSize, ValueType type)
	{
		DynVariantAry* ary = new DynVariantAry(1, colSize, type);
		m_cols.push_back(ColInfo{ name,ary });
		m_colMap.emplace(std::make_pair(name,(int)m_cols.size()-1));
		return true;
	}
	int GetColIndex(const std::string& name)
	{
		auto it = m_colMap.find(name);
		return (it != m_colMap.end()) ? it->second : -1;
	}
	bool DeleteRow(long long rowId)
	{
		auto find = m_rowMap.find(rowId);
		if (find == m_rowMap.end())
		{
			return false;
		}
		auto* rObj = find->second;
		m_rowIDcol.ary->Remove(rObj->r());
		for (auto it : m_cols)
		{
			it.ary->Remove(rObj->r());
		}
		m_rowMap.erase(find);
		rObj->Release();
		return true;
	}
	bool GetRow(long long rowId, KWARGS& kwargs)
	{
		auto find = m_rowMap.find(rowId);
		if (find == m_rowMap.end())
		{
			return false;
		}
		auto rObj = find->second;
		for (auto it : m_cols)
		{
			X::Value v;
			it.ary->Get(rObj->r(), v);
			kwargs.emplace(std::make_pair(it.name, v));
		}
		return true;
	}
	long long  InsertRow(KWARGS kwargs)
	{
		long long rowID = m_lastRowId++;
		size_t r = m_rowIDcol.ary->Add(rowID);
		for (auto& i : kwargs)
		{
			int idx = GetColIndex(i.first);
			if (idx >= 0)
			{
				m_cols[idx].ary->Add(i.second);
			}
		}
		if (r == -1)
		{
			return -1;
		}
		TableRow* rObj = new TableRow(this, rowID, r);
		rObj->AddRef();
		m_rowMap.emplace(std::make_pair(rowID, rObj));
		return rowID;
	}
	void Test()
	{
		AddCol("Name", sizeof(X::Value),ValueType::Str);
		AddCol("Age", sizeof(long long), ValueType::Int64);
		AddCol("Weight", sizeof(double), ValueType::Double);

		for (int i = 0; i < 1000; i++)
		{
			auto rowId = InsertRow({
			{"Name","Shawn"},
			{"Age",40+i},
			{"Weight",137.3+i},
				});
			KWARGS vals;
			GetRow(rowId, vals);
		}
		DeleteRow(10);
		DeleteRow(20);
		DeleteRow(30);
		DeleteRow(40);
		for (int i = 0; i < 10; i++)
		{
			auto rowId = InsertRow({
			{"Name","Shawn"},
			{"Age",40 + i},
			{"Weight",137.3 + i},
				});
			KWARGS vals;
			GetRow(rowId, vals);
			int c = 0;
		}
		int c = 0;
	}
};
}
}