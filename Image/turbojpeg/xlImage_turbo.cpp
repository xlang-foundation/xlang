#include "xlImage.h"
#include "xlFactory.h"
#include "xlang.h"
#include "include/turbojpeg.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

namespace X {
	namespace Images {
		class ImageImpl :
			public Image
		{
			std::string m_url;
			unsigned char* m_data = nullptr;
			int m_dataSize = 0;
		public:
			ImageImpl(std::string url)
			{
				m_url = url;
			}
			~ImageImpl()
			{
				if (m_data)
				{
					tjFree(m_data);
				}
			}
			virtual bool Save() override
			{
				std::ofstream wstream;
				wstream.open(m_url.c_str(), std::ios_base::out | std::ios_base::binary);
				wstream.write((char*)m_data, m_dataSize);
				wstream.close();
				return true;
			}
			virtual X::Value To_Tensor(int pixelFmt) override
			{
				struct stat stat_buf;
				int rc = stat(m_url.c_str(), &stat_buf);
				size_t jepgSize = -1;
				if (rc == 0)
				{
					jepgSize = stat_buf.st_size;
				}
				else
				{
					return X::Value(false);
				}
				char* pCompressedBuf = new char[jepgSize];
				std::ifstream stream;
				stream.open(m_url.c_str(),std::ios_base::in|std::ios_base::binary);
				stream.read(pCompressedBuf, jepgSize);
				stream.close();
				tjhandle _jpegDecompressor = tjInitDecompress();
				int width = 0;
				int height = 0;
				int jpegSubsamp = 0;
				tjDecompressHeader2(_jpegDecompressor, (unsigned char*)pCompressedBuf, jepgSize,
					&width, &height, &jpegSubsamp);

				int pixSize = 4;
				TJPF tjPixFmt = TJPF_RGBA;
				ImagePixelFormat pixFmt = (ImagePixelFormat)pixelFmt;
				switch (pixFmt)
				{
				case ImagePixelFormat::RGBA:
					tjPixFmt = TJPF_RGBA;
					pixSize = 4;
					break;
				case ImagePixelFormat::RGB:
					tjPixFmt = TJPF_RGB;
					pixSize = 3;
					break;
				case ImagePixelFormat::BGRA:
					tjPixFmt = TJPF_BGRA;
					pixSize = 4;
					break;
				case ImagePixelFormat::BGR:
					tjPixFmt = TJPF_BGR;
					pixSize = 3;
					break;
				default:
					break;
				}
				X::Tensor tensor;
				Port::vector<int> shapes(3);
				shapes.push_back((int)height);
				shapes.push_back((int)width);
				shapes.push_back(pixSize);
				tensor->SetShape(shapes);
				tensor->SetDataType(X::TensorDataType::UBYTE);
				X::Value initData;
				tensor->Create(initData);
				char* pBuffer = tensor->GetData();
				tjDecompress2(_jpegDecompressor, (unsigned char*)pCompressedBuf, jepgSize,
					(unsigned char*)pBuffer,width, width * pixSize, height, tjPixFmt, /*TJFLAG_FASTDCT*/0);
				tjDestroy(_jpegDecompressor);
				return X::Value(tensor);
			}
			virtual bool From_Tensor(X::XRuntime* rt, X::XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				if (params.size() == 0 || !params[0].IsObject())
				{
					retValue = X::Value(false);
					return false;
				}
				XObj* pXObj = params[0].GetObj();
				if (pXObj->GetType() != ObjType::Tensor)
				{
					retValue = X::Value(false);
					return false;
				}
				XTensor* pTensor = dynamic_cast<XTensor*>(pXObj);
				int dimCount = pTensor->GetDimCount();
				if (dimCount == 1 || dimCount > 3)
				{
					retValue = X::Value(false);
					return false;
				}
				unsigned char* pImgRawData = (unsigned char*)pTensor->GetData();
				auto itemType = pTensor->GetDataType();
				//for gray ( 1 byte) or 32 bits pixel format with itemType is INT or UINT
				int COLOR_COMPONENTS = 4;
				TJPF pixF = TJPF_RGB;
				if (dimCount == 2)
				{
					switch (itemType)
					{
					case X::TensorDataType::BYTE:
					case X::TensorDataType::UBYTE:
						COLOR_COMPONENTS = 1;
						pixF = TJPF_GRAY;
						break;
					case X::TensorDataType::INT:
					case X::TensorDataType::UINT:
						COLOR_COMPONENTS = 4;
						pixF = TJPF_RGBA;
						break;
					default:
						break;
					}
				}
				else if (dimCount == 3)
				{
					auto dimSize = pTensor->GetDimSize(2);
					if ((dimSize >= 3 && dimSize <= 4) &&
						(itemType == X::TensorDataType::BYTE
							|| itemType == X::TensorDataType::UBYTE))
					{
						pixF = (dimSize ==3) ? TJPF_RGB:TJPF_RGBA;
						COLOR_COMPONENTS = dimSize;
					}
				}

				const int JPEG_QUALITY = 75;
				int _height = pTensor->GetDimSize(0);
				int _width = pTensor->GetDimSize(1);

				long unsigned int _jpegSize = 0;
				unsigned char* _compressedImage = nullptr; //!< Memory is allocated by tjCompress2 if _jpegSize == 0

				tjhandle _jpegCompressor = tjInitCompress();

				tjCompress2(_jpegCompressor, pImgRawData, _width, 0, _height, pixF,
					&_compressedImage, &_jpegSize, TJSAMP_444, JPEG_QUALITY,
					TJFLAG_FASTDCT);

				tjDestroy(_jpegCompressor);

				//to free the memory allocated by TurboJPEG (either by tjAlloc(), 
				//or by the Compress/Decompress) after you are done working on it:
				//tjFree(_compressedImage);
				m_data = _compressedImage;
				m_dataSize = _jpegSize;

				return true;
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