#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"
#include <Windows.h>

namespace XWin
{
	class WICFactory :
		public Singleton<WICFactory>
	{
		void* m_pObj = NULL;
		bool Create();
	public:
		WICFactory()
		{
			Create();
		}
		~WICFactory();
		template<typename T>
		T* WIC()
		{
			return (T*)m_pObj;
		}
	};

	typedef void (*ImageCleanFunc)(void* pObj);
	class Image
	{
		void* m_pObj = nullptr;
		void* m_pTargetObj = nullptr;
		ImageCleanFunc m_cleanFunc = nullptr;
		std::string m_url;
	public:
		template<typename T>
		T* Obj() { return (T*)m_pObj; }
		BEGIN_PACKAGE(Image)
		END_PACKAGE
		Image(std::string url);
		~Image();
		void SetTargetObj(void* pObj, ImageCleanFunc cleanFunc)
		{
			m_pTargetObj = pObj;
			m_cleanFunc = cleanFunc;
		}
		void* GetTargetObj() { return m_pTargetObj; }
		HBITMAP ToHBITMAP();
	};

}