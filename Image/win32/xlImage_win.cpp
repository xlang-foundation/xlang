#include "xlImage.h"
#include <wincodec.h>
#include "utility.h"
#include "xlFactory.h"
#include "xlang.h"

//ref
//http://www.nuonsoft.com/blog/2011/10/17/introduction-to-wic-how-to-use-wic-to-load-an-image-and-draw-it-with-gdi/
namespace X {
	namespace Images {
		class WICFactory :
			public Singleton<WICFactory>
		{
			void* m_pObj = NULL;
			bool Create()
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
		public:
			WICFactory()
			{
				Create();
			}
			~WICFactory()
			{
				if (m_pObj)
				{
					((IWICImagingFactory*)m_pObj)->Release();
				}
			}
			template<typename T>
			T* WIC()
			{
				return (T*)m_pObj;
			}
		};

		class ImageImpl:
			public Image
		{
			IWICBitmapFrameDecode* m_pIDecoderFrame = nullptr;
		public:
			ImageImpl(std::string url)
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
						m_pIDecoderFrame = pIDecoderFrame;
					}
				}
				if (pIDecoder)
				{
					pIDecoder->Release();
				}
			}
			~ImageImpl()
			{
				if (m_pIDecoderFrame)
				{
					m_pIDecoderFrame->Release();
				}
			}
			virtual X::Value To_Tensor(int pixelFmt) override
			{
				GUID dstFmt = GUID_WICPixelFormat32bppRGBA;
				int pixSize = 4;
				ImagePixelFormat pixFmt = (ImagePixelFormat)pixelFmt;
				switch (pixFmt)
				{
				case ImagePixelFormat::RGBA:
					dstFmt = GUID_WICPixelFormat32bppRGBA;
					pixSize = 4;
					break;
				case ImagePixelFormat::RGB:
					dstFmt = GUID_WICPixelFormat24bppRGB;
					pixSize = 3;
					break;
				case ImagePixelFormat::BGRA:
					dstFmt = GUID_WICPixelFormat32bppPBGRA;
					pixSize = 4;
					break;
				case ImagePixelFormat::BGR:
					dstFmt = GUID_WICPixelFormat24bppBGR;
					pixSize = 3;
					break;
				default:
					break;
				}
				X::Tensor tensor;
				IWICFormatConverter* pConverter = nullptr;
				HRESULT hr = WICFactory::I().WIC<IWICImagingFactory>()->CreateFormatConverter(&pConverter);
				if (SUCCEEDED(hr))
				{
					hr = pConverter->Initialize(
						m_pIDecoderFrame,
						dstFmt,
						WICBitmapDitherTypeNone,
						NULL,
						0.f,
						WICBitmapPaletteTypeCustom
					);
				}
				if (pConverter)
				{
					UINT height = 0;
					UINT width = 0;
					hr = pConverter->GetSize(&width, &height);
					std::vector<int> shapes({ (int)width, (int)height, pixSize });
					tensor->SetShape(shapes);
					tensor->SetDataType(X::TensorDataType::UBYTE);
					tensor->Create(X::Value());
					char* pBuffer = tensor->GetData();
					pConverter->CopyPixels(0, width * 4, tensor->Size(),(BYTE*)pBuffer);
					pConverter->Release();
				}
				return X::Value(tensor);
			}
			bool From_Tensor(X::XRuntime* rt, X::XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue)
			{
				if (params.size() < 1)
				{
					retValue = X::Value(false);
					return false;
				}

				return true;
			}
			HBITMAP ToHBITMAP()
			{
				IWICFormatConverter* pConverter = nullptr;
				HRESULT hr = WICFactory::I().WIC<IWICImagingFactory>()->CreateFormatConverter(&pConverter);
				if (SUCCEEDED(hr))
				{
					hr = pConverter->Initialize(
						m_pIDecoderFrame,
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
					UINT height = 0;
					UINT width = 0;
					hr = pConverter->GetSize(&width, &height);
					std::vector<BYTE> buffer(width * height * 4);
					pConverter->CopyPixels(0, width * 4, buffer.size(), buffer.data());
					bitmap = CreateBitmap(width, height, 1, 32, buffer.data());
					pConverter->Release();
				}
				return bitmap;
			}
		};
		Image::Image(std::string url)
		{
			if (url.find('\\') == url.npos || url.find('/') == url.npos)
			{
				X::XModule* pModule = dynamic_cast<X::XModule*>(Factory::I().GetModule().GetObj());
				if (pModule)
				{
					auto path = pModule->GetPath();
					url = path + "\\" + url;
				}
			}
			m_url = url;
			Init();
		}
		bool Image::Init()
		{
			m_pimpl = new ImageImpl(m_url);
			return true;
		}
	}
}