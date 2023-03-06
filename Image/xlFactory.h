#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlImage.h"

namespace X {
	namespace Images {
		enum class ImagePixelFormat
		{
			RGBA,
			RGB,
			BGRA,
			BGR,
			
		};
		class Factory:
			public Singleton<Factory>
		{
			X::Value m_curModule;
		public:
			BEGIN_PACKAGE(Factory)
				APISET().AddConst("rgba", (int)ImagePixelFormat::RGBA);
				APISET().AddConst("rgb", (int)ImagePixelFormat::RGB);
				APISET().AddConst("bgra", (int)ImagePixelFormat::BGRA);
				APISET().AddConst("bgr", (int)ImagePixelFormat::BGR);
				APISET().AddClass<1, Image>("Image");
			END_PACKAGE
			void SetModule(X::Value curModule)
			{
				m_curModule = curModule;
			}
			X::Value& GetModule() { return m_curModule; }
		};
	}
}