#include "list.h"
#include "utility.h"
#include "dict.h"
#include "port.h"

namespace X
{
	namespace Data
	{
		bool List::Call(Runtime* rt, ARGS& params,
			KWARGS& kwParams,
			AST::Value& retValue)
		{
			//do twice, first to do size or other call with
			//memory allocation
			for (auto it : kwParams)
			{
				if (it.first == "size")
				{
					long long size = it.second.GetLongLong();
					m_data.resize(size);
					break;
				}
			}
			for (auto it : kwParams)
			{
				if (it.first == "init")
				{
					auto v0 = it.second;
					if (v0.IsObject() 
						&& v0.GetObj()->GetType() ==Data::Type::Str
						&& v0.ToString().find("rand")==0)
					{
						auto strV = v0.ToString();
						double d1 = 0;
						double d2 = 1;
						SCANF(strV.c_str(),"rand(%lf,%lf)",&d1,&d2);

						for (auto& v : m_data)
						{
							v = randDouble(d1,d2);
						}
					}
					else
					{
						for (auto& v : m_data)
						{
							v = v0;
						}
					}
					break;
				}
			}
			retValue = AST::Value(this);
			return true;
		}
		List* List::FlatPack(Runtime* rt,long long startIndex, long long count)
		{
			if (startIndex < 0 || startIndex >= Size())
			{
				return nullptr;
			}
			if (count == -1)
			{
				count = Size()- startIndex;
			}
			if ((startIndex + count) > Size())
			{
				return nullptr;
			}
			List* pOutList = new List();
			for (long long i = 0; i < count; i++)
			{
				long long idx = startIndex + i;
				AST::Value val;
				Get(idx, val);
				Dict* dict = new Dict();
				//Data::Str* pStrName = new Data::Str(it.first);
				//dict->Set("Name", AST::Value(pStrName));
				auto valType = val.GetValueType();
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", AST::Value(pStrType));
				if (!val.IsObject() || (val.IsObject() && val.GetObj()->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (val.IsObject())
				{
					AST::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					AST::Value valSize(val.GetObj()->Size());
					dict->Set("Size", valSize);
				}
				AST::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
	}
}