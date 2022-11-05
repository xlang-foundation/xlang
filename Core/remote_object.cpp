#include "remote_object.h"
#include "list.h"

namespace X
{
	X::Data::List* RemoteObject::FlatPack(XlangRuntime* rt, XObj* pContext,
		long long startIndex, long long count)
	{
		X::Data::List* pPackList = nullptr;
		X::Value valPack;
		bool bOK = m_proxy->FlatPack(m_remote_Parent_Obj_id,
			m_remote_Obj_id, startIndex, count, valPack);
		if (bOK && valPack.IsObject())
		{
			pPackList = dynamic_cast<X::Data::List*>(valPack.GetObj());
			if (pPackList)
			{
				pPackList->IncRef();
			}
		}
		return pPackList;
	}
}