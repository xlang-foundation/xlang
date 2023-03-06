#pragma once
#include "singleton.h"
#include "xpackage.h"
#include "xlang.h"

namespace X {
	namespace Images {
		class Image
		{
			std::string m_url;
			Image* m_pimpl = nullptr;
		public:
			BEGIN_PACKAGE(Image)
				APISET().AddFunc<1>("to_tensor", &Image::To_Tensor, "image.to_tensor()");
			END_PACKAGE
			Image()
			{

			}
			Image(std::string url);
			bool Init();
			~Image()
			{
			}
			virtual X::Value To_Tensor(int pixelFmt)
			{
				if (m_pimpl)
				{
					return m_pimpl->To_Tensor(pixelFmt);
				}
				else
				{
					return X::Value();
				}
			}
		};

	}
}