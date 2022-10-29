#include "xlDraw.h"
#include <d2d1.h>
#include "singleton.h"
#include "xlWindow.h"
#include "xlImage.h"
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
		bool Create()
		{
			HRESULT hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				&m_pD2DFactory
			);
			if (FAILED(hr))
			{
				return false;
			}
			return (hr == S_OK);
		}
	public:
		ID2D1Factory* GetFact()
		{
			return m_pD2DFactory;
		}

		D2DFactory()
		{
			Create();
		}
		~D2DFactory()
		{
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
		void* pTargetObj = pImg->GetTargetObj();
		ID2D1Bitmap* pBmp = nullptr;
		if (pTargetObj == nullptr)
		{
			IWICFormatConverter* pConvertedSourceBitmap;
			HRESULT hr = WICFactory::I().WIC<IWICImagingFactory>()->CreateFormatConverter(&pConvertedSourceBitmap);
			if (SUCCEEDED(hr))
			{
				hr = pConvertedSourceBitmap->Initialize(
					pImg->Obj<IWICBitmapFrameDecode>(),
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					NULL,
					0.f,
					WICBitmapPaletteTypeCustom
				);
			}
			ID2D1Bitmap* pD2DBitmap;
			hr = pDrawInfo->RT()->CreateBitmapFromWicBitmap(pConvertedSourceBitmap, NULL, &pD2DBitmap);
			if (pConvertedSourceBitmap)
			{
				pConvertedSourceBitmap->Release();
			}
			if (SUCCEEDED(hr))
			{
				pImg->SetTargetObj(pD2DBitmap,[](void* pObj) {
						((ID2D1Bitmap*)pObj)->Release();
					});
			}
			pBmp = pD2DBitmap;
		}
		else
		{
			pBmp = (ID2D1Bitmap*)pTargetObj;
		}
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
}
