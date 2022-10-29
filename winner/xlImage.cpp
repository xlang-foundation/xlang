#include "xlImage.h"
#include <wincodec.h>
#include "utility.h"
#include "singleton.h"

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
}