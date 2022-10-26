#include "xlDraw.h"
#include <d2d1.h>
#include "singleton.h"
#include "xlWindow.h"
#include "const_data.h"
#include <wincodec.h>
#include "utility.h"

X::Value::operator XWin::Brush* ()const
{
	if (x.obj->GetType() == ObjType::Package)
	{
		XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
		return (XWin::Brush*)pPack->GetEmbedObj();
	}
	else
	{
		return nullptr;
	}
}
X::Value::operator XWin::Image* ()const
{
	if (x.obj->GetType() == ObjType::Package)
	{
		XPackage* pPack = dynamic_cast<XPackage*>(x.obj);
		return (XWin::Image*)pPack->GetEmbedObj();
	}
	else
	{
		return nullptr;
	}
}

namespace XWin
{
	class D2DFactory :
		public Singleton<D2DFactory>
	{
		ID2D1Factory* m_pD2DFactory = NULL;
		IWICImagingFactory* m_pIWICFactory = NULL;
		bool Create()
		{
			CoInitializeEx(NULL, COINIT_MULTITHREADED);
			HRESULT hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				&m_pD2DFactory
			);
			if (FAILED(hr))
			{
				return false;
			}
			// Create WIC factory
			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&m_pIWICFactory)
			);
			return (hr == S_OK);
		}
	public:
		ID2D1Factory* GetFact()
		{
			return m_pD2DFactory;
		}
		IWICImagingFactory* WIC()
		{
			return m_pIWICFactory;
		}
		D2DFactory()
		{
			Create();
		}
		~D2DFactory()
		{
			if (m_pIWICFactory)
			{
				m_pIWICFactory->Release();
			}
			if (m_pD2DFactory)
			{
				m_pD2DFactory->Release();
			}
		}
	};
	class DrawInfo
	{
		ID2D1HwndRenderTarget* pRT = NULL;
	public:
		DrawInfo(void* hWnd)
		{
			HWND hwnd = (HWND)hWnd;
			RECT rc;
			GetClientRect(hwnd, &rc);
			HRESULT hr = D2DFactory::I().GetFact()->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(
					hwnd,
					D2D1::SizeU(
						rc.right - rc.left,
						rc.bottom - rc.top)
				),
				&pRT
			);
		}
		ID2D1HwndRenderTarget* RT() { return pRT; }
	};

	Draw::Draw(Window* pWin)
	{
		m_pDrawInfo = (void*)new DrawInfo(pWin->GetWnd());
	}
	Draw::~Draw()
	{
		delete (DrawInfo*)m_pDrawInfo;
	}
	bool Draw::Begin()
	{
		((DrawInfo*)m_pDrawInfo)->RT()->BeginDraw();
		return true;
	}
	bool Draw::End()
	{
		((DrawInfo*)m_pDrawInfo)->RT()->EndDraw();
		return true;
	}
	Brush::Brush(Draw* pDraw, std::string color)
	{
		DrawInfo* pDrawInfo = (DrawInfo*)pDraw->GetDrawInfo();
		ID2D1SolidColorBrush* pBrush = NULL;
		unsigned int rgb = 0;
		auto& colorMap = Data::I().Color();
		auto it = colorMap.find(color);
		if (it != colorMap.end())
		{
			rgb = it->second;
		}

		pDrawInfo->RT()->CreateSolidColorBrush(
			D2D1::ColorF(rgb),
			&pBrush
		);
		m_pObj = (void*)pBrush;
	}
	Brush::~Brush()
	{
		Obj<ID2D1SolidColorBrush>()->Release();
	}
	bool Draw::DrawRectangle(int left, int top, int right, int bottom, Brush* pBrush)
	{
		DrawInfo* pDrawInfo = (DrawInfo*)m_pDrawInfo;
		pDrawInfo->RT()->DrawRectangle(
			D2D1::RectF(left,top,right,bottom),
			pBrush->Obj<ID2D1SolidColorBrush>());
		return true;
	}
	bool Draw::DrawImage(Image* pImg, int left, int top, int right, int bottom)
	{
		DrawInfo* pDrawInfo = (DrawInfo*)m_pDrawInfo;
		auto* pBmp = pImg->Obj<ID2D1Bitmap>();
		D2D1_RECT_F rectangle = D2D1::RectF(left, top, right, bottom);
		pDrawInfo->RT()->DrawBitmap(pBmp, rectangle);
		return true;
	}
	bool Draw::Clear(std::string color)
	{
		unsigned int rgb = 0;
		auto& colorMap = Data::I().Color();
		auto it = colorMap.find(color);
		if (it != colorMap.end())
		{
			rgb = it->second;
		}
		DrawInfo* pDrawInfo = (DrawInfo*)m_pDrawInfo;
		//pDrawInfo->RT()->SetTransform(D2D1::Matrix3x2F::Identity());
		pDrawInfo->RT()->Clear(D2D1::ColorF(rgb));
		return true;
	}
	Color::Color(unsigned int rgb, float a)
	{
		m_pObj = new D2D1::ColorF(rgb,a);
	}
	Color::Color(std::string color)
	{
		unsigned int rgb = 0;
		auto& colorMap = Data::I().Color();
		auto it = colorMap.find(color);
		if (it != colorMap.end())
		{
			rgb = it->second;
		}
		m_pObj = new D2D1::ColorF(rgb);
	}
	Color::~Color()
	{
		delete (D2D1::ColorF*)m_pObj;
	}

	Image::Image(Draw* pDraw,std::string url)
	{
		HRESULT hr = S_OK;
		std::wstring wurl = s2ws(url);
		IWICBitmapDecoder* pIDecoder = NULL;
		IWICBitmapFrameDecode* pIDecoderFrame = NULL;
		UINT nFrameCount = 0;
		UINT uiWidth, uiHeight;
		DrawInfo* pDrawInfo = (DrawInfo*)pDraw->GetDrawInfo();

		// Create decoder for an image.
		hr = D2DFactory::I().WIC()->CreateDecoderFromFilename(
			wurl.c_str(),NULL,GENERIC_READ,WICDecodeMetadataCacheOnDemand,
			&pIDecoder
		);
		if (SUCCEEDED(hr))
		{
			hr = pIDecoder->GetFrameCount(&nFrameCount);
		}
		// Process each frame in the image.
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
			IWICFormatConverter* pConvertedSourceBitmap;
			hr = D2DFactory::I().WIC()->CreateFormatConverter(&pConvertedSourceBitmap);
			if (SUCCEEDED(hr))
			{
				hr = pConvertedSourceBitmap->Initialize(
					pIDecoderFrame,                          // Input bitmap to convert
					GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
					WICBitmapDitherTypeNone,         // Specified dither pattern
					NULL,                            // Specify a particular palette 
					0.f,                             // Alpha threshold
					WICBitmapPaletteTypeCustom       // Palette translation type
				);
			}
			ID2D1Bitmap* pD2DBitmap;
			hr = pDrawInfo->RT()->CreateBitmapFromWicBitmap(pConvertedSourceBitmap, NULL, &pD2DBitmap);
			if (SUCCEEDED(hr))
			{
				m_pObj = (void*)pD2DBitmap;
			}
			if (pConvertedSourceBitmap)
			{
				pConvertedSourceBitmap->Release();
			}
			if (pIDecoderFrame)
			{
				pIDecoderFrame->Release();
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
			Obj<ID2D1Bitmap>()->Release();
		}
	}

}
