#include "table.h"
#include "dict.h"
namespace X
{
	namespace Data
	{
		long long TableRow::Size()
		{
			return m_table->GetColNum();
		}
		List* TableRow::FlatPack(Runtime* rt,
			long long startIndex, long long count)
		{
			if (startIndex < 0 || startIndex >= Size())
			{
				return nullptr;
			}
			if (count == -1)
			{
				count = Size() - startIndex;
			}
			if ((startIndex + count) > Size())
			{
				return nullptr;
			}
			List* pOutList = new List();
			for (long long i = 0; i < count; i++)
			{
				long long idx = startIndex + i;
				auto& col = m_table->m_cols[idx];
				X::Value val;
				col.ary->Get(m_r, val);

				Dict* dict = new Dict();
				X::Value colName((char*)col.name.c_str(),(int)col.name.size());
				dict->Set("Name", colName);

				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject() && 
					dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					X::Value valSize(val.GetObj()->Size());
					dict->Set("Size", valSize);
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
		List* Table::FlatPack(Runtime* rt, 
			long long startIndex, long long count)
		{
			if (startIndex < 0 || startIndex >= Size())
			{
				return nullptr;
			}
			if (count == -1)
			{
				count = Size() - startIndex;
			}
			if ((startIndex + count) > Size())
			{
				return nullptr;
			}
			auto row_it = m_rowMap.begin();
			std::advance(row_it, startIndex);

			List* pOutList = new List();
			for (long long i = 0; i < count; i++)
			{
				TableRow* row = row_it->second;
				X::Value val(row);
				Dict* dict = new Dict();
				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value("TableRow"));
				X::Value objId((unsigned long long)(X::XObj*)row);
				dict->Set("Value", objId);
				X::Value valSize(row->Size());
				dict->Set("Size", valSize);

				X::Value valDict(dict);
				pOutList->Add(rt, valDict);

				std::advance(row_it,1);
			}
			return pOutList;
		}
	}
}