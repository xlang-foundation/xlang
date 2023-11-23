#pragma once

namespace X
{
	class ObjRef
	{
		int m_ref = 0;
	protected:
		FORCE_INLINE virtual int AddRef()
		{
			return ++m_ref;
		}
		FORCE_INLINE virtual int Release()
		{
			return --m_ref;
		}
	public:
		FORCE_INLINE int Ref() { return m_ref; }
		ObjRef() {}
		//line below, need to use virtual
		//for virtual inheritance from derived class
		virtual ~ObjRef() {}
	};
}