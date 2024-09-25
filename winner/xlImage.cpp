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

#include "xlImage.h"
#include <wincodec.h>
#include "utility.h"
#include "singleton.h"
#include "xlApp.h"

//ref
//http://www.nuonsoft.com/blog/2011/10/17/introduction-to-wic-how-to-use-wic-to-load-an-image-and-draw-it-with-gdi/

namespace XWin
{
	bool WICFactory::Create()
	{
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		IWICImagingFactory* pIWICFactory = nullptr;
		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pIWICFactory)
		);
		m_pObj = pIWICFactory;
		return (hr == S_OK);
	}
	WICFactory::~WICFactory()
	{
		if (m_pObj)
		{
			((IWICImagingFactory*)m_pObj)->Release();
		}
	}
	Image::Image(std::string url)
	{
		if (url.find('\\') == url.npos || url.find('/') == url.npos)
		{
			auto m = App::I().GetModule();
			X::XModule* pModule = dynamic_cast<X::XModule*>(m.GetObj());
			if (pModule)
			{
				auto path = pModule->GetPath();
				std::string strPath(path);
				X::g_pXHost->ReleaseString(path);
				url = strPath + "\\" + url;
			}
		}
		m_url = url;
		HRESULT hr = S_OK;
		std::wstring wurl = s2ws(url);
		IWICBitmapDecoder* pIDecoder = NULL;
		IWICBitmapFrameDecode* pIDecoderFrame = NULL;
		UINT nFrameCount = 0;
		UINT uiWidth, uiHeight;

		// Create decoder for an image.
		hr = WICFactory::I().WIC<IWICImagingFactory>()->CreateDecoderFromFilename(
			wurl.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
			&pIDecoder
		);
		if (SUCCEEDED(hr))
		{
			hr = pIDecoder->GetFrameCount(&nFrameCount);
		}
		//todo: process multiple frames
		if (nFrameCount > 1)
			nFrameCount = 1;
		for (UINT i = 0; i < nFrameCount; i++)
		{
			// Retrieve the next bitmap frame.
			if (SUCCEEDED(hr))
			{
				hr = pIDecoder->GetFrame(i, &pIDecoderFrame);
			}
			// Retrieve the size of the bitmap frame.
			if (SUCCEEDED(hr))
			{
				hr = pIDecoderFrame->GetSize(&uiWidth, &uiHeight);
			}
			if (SUCCEEDED(hr))
			{
				m_pObj = (void*)pIDecoderFrame;
			}
		}
		if (pIDecoder)
		{
			pIDecoder->Release();
		}
	}
	Image::~Image()
	{
		if (m_pObj)
		{
			Obj<IWICBitmapFrameDecode>()->Release();
		}
		if (m_cleanFunc && m_pTargetObj)
		{
			m_cleanFunc(m_pTargetObj);
		}
	}
	HBITMAP Image::ToHBITMAP()
	{
		IWICFormatConverter* pConverter = nullptr;
		HRESULT hr = WICFactory::I().WIC<IWICImagingFactory>()->CreateFormatConverter(&pConverter);
		if (SUCCEEDED(hr))
		{
			hr = pConverter->Initialize(
				Obj<IWICBitmapFrameDecode>(),
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.f,
				WICBitmapPaletteTypeCustom
			);
		}
		HBITMAP bitmap = nullptr;
		if (pConverter)
		{
			UINT height=0;
			UINT width=0;
			hr = pConverter->GetSize(&width, &height);
			std::vector<BYTE> buffer(width * height * 4);
			pConverter->CopyPixels(0, width * 4, buffer.size(), buffer.data());
			bitmap = CreateBitmap(width, height, 1, 32, buffer.data());
			pConverter->Release();
		}
		return bitmap;
	}
}