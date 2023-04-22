#pragma once

namespace X
{
	class ObjRef
	{
		int m_ref = 0;
	protected:
		inline virtual int AddRef()
		{
			return ++m_ref;
		}
		inline virtual int Release()
		{
			return --m_ref;
		}
	public:
		inline int Ref() { return m_ref; }
		ObjRef() {}
		//line below, need to use virtual
		//for virtual inheritance from derived class
		virtual ~ObjRef() {}
	};
}