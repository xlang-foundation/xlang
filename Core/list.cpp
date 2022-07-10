#include "list.h"
#include "utility.h"

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
						sscanf_s(strV.c_str(),"rand(%lf,%lf)",&d1,&d2);

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

	}
}