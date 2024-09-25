/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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