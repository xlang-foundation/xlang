#ifndef _X_PROXY_H_
#define _X_PROXY_H_

#include "xlang.h"

namespace X
{
	typedef struct
	{
		unsigned long long pid;
		void* objId;
	} ROBJ_ID;
	typedef int ROBJ_MEMBER_ID;
	class XProxy
	{
	public:
		virtual ROBJ_ID QueryRootObject(std::string& name) = 0;
		virtual ROBJ_MEMBER_ID QueryMember(ROBJ_ID id,std::string& name,
			bool& KeepRawParams) = 0;
		virtual long long QueryMemberCount(X::ROBJ_ID id) = 0;
		virtual bool FlatPack(X::ROBJ_ID parentObjId,X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			long long startIndex,long long count, Value& retList) = 0;
		virtual X::Value UpdateItemValue(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val) = 0;
		virtual ROBJ_ID GetMemberObject(ROBJ_ID id, ROBJ_MEMBER_ID memId) = 0;
		virtual bool ReleaseObject(ROBJ_ID id) = 0;
		virtual bool Call(XRuntime* rt, XObj* pContext,
			ROBJ_ID parent_id, ROBJ_ID id, ROBJ_MEMBER_ID memId,
			ARGS& params, KWARGS& kwParams, X::Value& trailer,Value& retValue) = 0;
	};
}

#endif //_X_PROXY_H_