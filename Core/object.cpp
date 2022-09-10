#include "object.h"
#include "list.h"
#include "dict.h"
#include "xclass.h"
#include "attribute.h"

namespace X
{
	namespace Data
	{
		AttributeBag* Object::GetAttrBag()
		{
			AutoLock(m_lock);
			if (m_aBag == nullptr)
			{
				m_aBag = new AttributeBag();
			}
			return m_aBag;
		}
		void Object::DeleteAttrBag()
		{
			AutoLock(m_lock);
			if (m_aBag)
			{
				delete m_aBag;
				m_aBag = nullptr;
			}
		}
	}
}