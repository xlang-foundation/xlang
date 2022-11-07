#include "prop.h"
#include "list.h"
#include "dict.h"

namespace X
{
	namespace Data
	{
		List* PropObject::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			List* pOutList = nullptr;
			X::Value v0;
			if (GetPropValue(rt, pContext, v0))
			{
				if (v0.IsObject())
				{
					Object* pObj = dynamic_cast<Object*>(v0.GetObj());
					pOutList = pObj->FlatPack(rt, nullptr, IdList, id_offset,startIndex, count);
				}
			}
			return pOutList;
		}
	}
}