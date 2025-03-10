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
#include "object.h"
#include <functional>
#include "limits.h"
#include "tensorop.h"
#include "list.h"
/*
	Shawn Xiong @2/15/2023
	Tensor is a multiple dimensional array whith same data type
	compare with list which hold each item at least take 16-bytes
	(X::Value), but Tensor will reduce memory usage
	for example Tensor of float, each item just take 4-bytes
	and Tensor also can hold X::Value as its item, then item can be
	vairant data type
	in this Implementation, use templeate to define this Tensor
	class, and will register for diffrent type of Tensor
	include: 
		int, float, double,complex and X::Value
	maybe also include char as item to reduce memeory 
	in some case use as flags
*/

#define Tensor_DType "dtype"
#define Tensor_Shape "shape"
#define Tensor_Max "max"
#define Tensor_Min "min"
#define Tensor_Name "name"

namespace X
{
	namespace Data
	{
		struct TensorDim
		{
			long long offset;//start pos
			long long size;
			long long stride;//offset to next row data
			long long dimProd;
		};
		struct TensorIndex
		{
			long long i,j;
		};
		using TensorIterateProc = std::function<void(std::vector<long long>& indices)>;
		class Tensor:
			virtual public XTensor,
			virtual public Object
		{
			std::string m_name;

			long long m_dataSize = 0;//calculated from m_dims,keep for fast calculation
			//reference tensor to hold m_data
			//when create Tensor, need to Increase refcount for m_pTensorToOwneData
			Tensor* m_pTensorToOwneData = nullptr;
			char* m_data=nullptr;
			int m_startItemOffet = 0;// unit is sizeof data type
			std::vector<TensorDim> m_dims;
			TensorDataType m_dataType = TensorDataType::UNKNOWN;
			X::Value m_desc;//used to hold extra info

			FORCE_INLINE virtual X::Value GetDesc() override
			{
				return m_desc;
			}
			FORCE_INLINE virtual void SetDesc(X::Value& v) override
			{
				m_desc = v;
			}
		public:
			virtual long long GetItemSize() override
			{
				long long size = 1;
				switch (m_dataType)
				{
				case X::TensorDataType::BOOL:
				case X::TensorDataType::BYTE:
				case X::TensorDataType::QINT8:
				case X::TensorDataType::QUINT8:
				case X::TensorDataType::UBYTE:
					size = 1;
					break;
				case X::TensorDataType::SHORT:
				case X::TensorDataType::USHORT:
				case X::TensorDataType::BFLOAT16:
				case X::TensorDataType::HALFFLOAT:
					size = 2;
					break;
				case X::TensorDataType::INT:
				case X::TensorDataType::QINT32:
				case X::TensorDataType::UINT:
					size = 4;
					break;
				case X::TensorDataType::LONGLONG:
				case X::TensorDataType::ULONGLONG:
					size = 8;
					break;
				case X::TensorDataType::FLOAT:
					size = 4;
					break;
				case X::TensorDataType::DOUBLE:
					size = 8;
					break;
				case X::TensorDataType::CFLOAT:
					size = 8;
					break;
				case X::TensorDataType::CDOUBLE:
					size = 16;
					break;
				case X::TensorDataType::FLOAT8_E4M3FN:
				case X::TensorDataType::FLOAT8_E4M3FNUZ:
				case X::TensorDataType::FLOAT8_E5M2:
				case X::TensorDataType::FLOAT8_E5M2FNUZ:
					size = 1;
					break;
				default:
					break;
				}
				return size;
			}
			virtual X::Value Shapes() override
			{
				X::List shapes;
				for (auto& d : m_dims)
				{
					shapes += d.size;
				}
				return shapes;
			}
			long long GetCount()
			{
				long long itemCnt = 1;
				for (auto& d : m_dims)
				{
					itemCnt *= d.size;
				}
				return itemCnt;
			}
			long long GetCountWithStride()
			{
				long long itemCnt = 1;
				for (auto& d : m_dims)
				{
					itemCnt *= d.stride;
				}
				return itemCnt;
			}
			long long CalcItemOffset(std::vector<long long>& indices)
			{
				long long off = m_startItemOffet;
				int idxCnt = (int)indices.size();
				for (int i = 0; i < idxCnt; i++)
				{
					auto& dim = m_dims[i];
					if (indices[i] < 0 || indices[i] >= dim.size)
						return -1;
					off += (indices[i] + dim.offset) * dim.dimProd;
				}
				off *= GetItemSize();
				return off;
			}
			void DeepCopyDataFromList(List* pList,std::vector<long long>& indices,int level);
			void CalcDimProd()
			{
				int nd = (int)m_dims.size();
				long long a = 1;
				m_dims[nd - 1].dimProd = a;
				for (int i = nd - 1; i >= 1; i--)
				{
					//use stride instead of size because view of Tensor will change size 
					//but stride keeps same as original
					a *= m_dims[i].stride;
					m_dims[i - 1].dimProd = a;
				}
			}
		public:
			static void Init();
			static void cleanup();
			Tensor();
			~Tensor();

			FORCE_INLINE void SetName(std::string& n)
			{
				m_name = n;
			}
			FORCE_INLINE std::string& GetName() { return m_name; }
			//this function only return first dim's size
			//because debug will use it as first level
			virtual long long Size() override
			{
				if (m_dims.size() > 0)
				{
					return m_dims[0].size;
				}
				else
				{
					return 0;
				}
			}
			virtual long long GetDataSize()
			{
				return GetCount() * GetItemSize();
			}
			virtual char* GetData() override
			{
				return m_data;
			}
			virtual void SetData(char* data, long long size) override
			{
				if (m_data == nullptr)
				{
					m_data = new char[size];
					m_dataSize = size;
				}
				else
				{
					if (m_dataSize != size)
					{
						delete[] m_data;
						m_data = new char[size];
						m_dataSize = size;
					}
				}
				memcpy(m_data, data, size);
			}
			std::vector<TensorDim> GetDims() 
			{
				return m_dims;
			}
			FORCE_INLINE virtual int GetDimCount() override
			{
				return (int)m_dims.size();
			}
			FORCE_INLINE virtual long long GetDimSize(int dimIdx) override
			{
				return m_dims[dimIdx].size;
			}
			
			virtual void SetShape(Port::vector<int>& shapes) override
			{
				m_dims.clear();
				for (auto i : shapes)
				{
					m_dims.push_back(TensorDim{0,i,i});
				}
			}
			//used by tensorOP to allocate data bases on pBaseTensor
			bool CreateBaseOnTensor(Tensor* pBaseTensor)
			{
				//if m_data allocated,means already created
				if (m_data)
				{
					return true;
				}
				//this tensor will be orginal tensor, not view of tensor
				//so do init below
				m_dataType = pBaseTensor->m_dataType;
				for (auto& dim : pBaseTensor->m_dims)
				{
					TensorDim newDim = dim;
					//remove view prop from base tensor
					//this tensor starts from new status
					//just keep size
					newDim.offset = 0;
					newDim.stride = newDim.size;
					m_dims.push_back(newDim);
				}
				CalcDimProd();
				long long totalSize = GetCount() * GetItemSize();
				if (totalSize > 0)
				{
					m_data = new char[totalSize];
					m_dataSize = totalSize;
				}
				return true;
			}
			//used by tensorOP to allocate data bases on pBaseTensor
			bool CreateBaseOnShape(std::vector<int> shapes)
			{
				//if m_data allocated,means already created
				if (m_data)
				{
					return true;
				}
				Port::vector<int> port_shapes(0);
				int shape_size = shapes.size();
				port_shapes.resize(shape_size);
				for (int i = 0; i < shape_size; i++)
					port_shapes.push_back(shapes[i]);

				SetShape (port_shapes);
				CalcDimProd();
				long long totalSize = GetCount() * GetItemSize();
				if (totalSize > 0)
				{
					m_data = new char[totalSize];
					m_dataSize = totalSize;
				}
				return true;
			}
			bool CreateBaseOnTensorWithPermute(Tensor* pBaseTensor, std::vector<int>& axes)
			{
				//if m_data allocated,means already created
				if (m_data)
				{
					return true;
				}
				//this tensor will be orginal tensor, not view of tensor
				//so do init below
				m_dataType = pBaseTensor->m_dataType;
				std::vector<TensorDim> newDims;
				for (auto& dim : pBaseTensor->m_dims)
				{
					TensorDim newDim = dim;
					//remove view prop from base tensor
					//this tensor starts from new status
					//just keep size
					newDim.offset = 0;
					newDim.stride = newDim.size;
					newDims.push_back(newDim);
				}
				int axCnt = (int)axes.size();
				int dimCnt = (int)newDims.size();
				m_dims.resize(dimCnt);
				for (int i=0;i< dimCnt;i++)
				{
					m_dims[i] = (i < axCnt) ? newDims[axes[i]] : newDims[i];
				}
				CalcDimProd();
				long long totalSize = GetCount() * GetItemSize();
				if (totalSize > 0)
				{
					m_data = new char[totalSize];
					m_dataSize = totalSize;
				}
				return true;
			}
			virtual bool Set(Value valIdx, X::Value& val) override
			{
				std::vector<long long> indices;
				if (valIdx.IsList())
				{
					List* pList = dynamic_cast<List*>(valIdx.GetObj());
					auto size = pList->Size();
					for (long long i = 0; i< size; i++)
					{
						indices.push_back((long long)pList->Get(i));
					}
				}
				else
				{
					indices.push_back((long long)valIdx);
				}
				SetDataWithIndices(indices, val);
				return true;
			}
			bool Get(std::vector<Data::TensorIndex>& IdxAry, X::Value& retVal);
			virtual XObj* Clone() override
			{
				auto* newTensor = new Tensor();
				newTensor->IncRef();
				newTensor->m_data = new char[m_dataSize];
				memcpy(newTensor->m_data, m_data, m_dataSize);
				newTensor->m_dataSize = m_dataSize;
				newTensor->m_dims = m_dims;
				newTensor->m_dataType = m_dataType;
				return newTensor;
			}
			void SetDataWithIndices(std::vector<long long>& indices, X::Value& val);
			void SetDataWithOffset(long long addr, X::Value& val);
			X::Value GetDataWithIndices(std::vector<long long>& indices);
			X::Value GetDataWithOffset(long long addr);

			//keep use same memory
			FORCE_INLINE X::Value reshape(X::Value& listOfShape)
			{
				Port::vector<int> shapes(0);
				int shapeCount = 1;
				if (listOfShape.IsList())
				{
					X::Data::List* pList = dynamic_cast<X::Data::List*>(listOfShape.GetObj());
					int axesCnt = (int)pList->Size();
					shapes.resize(axesCnt);
					for (int i = 0; i < axesCnt; i++)
					{
						int s = (int)pList->Get(i);
						shapeCount *= s;
						shapes.push_back(s);
					}
				}
				else
				{
					return X::Value();
				}
				//if this tensor is a view of origial tensor
				//need to check with the stride to keep memory as same size
				if (shapeCount != GetCountWithStride())
				{
					return X::Value();
				}
				auto* pNewTensor = new Tensor();
				//keep this tensor as New Tensor's ref
				IncRef();
				pNewTensor->m_dataType = m_dataType;
				pNewTensor->m_pTensorToOwneData = this;
				pNewTensor->m_data = m_data;
				pNewTensor->SetShape(shapes);
				pNewTensor->CalcDimProd();
				pNewTensor->m_dataSize = m_dataSize;
				return X::Value(pNewTensor);
			}
			FORCE_INLINE X::Value asType(int type)
			{
				TensorDataType dt = (TensorDataType)type;
				Tensor* pNewTensor = new Tensor();
				pNewTensor->SetDataType(dt);
				int dimCnt = (int)m_dims.size();
				Port::vector<int> dims(dimCnt);
				for (int i = 0; i < dimCnt; i++)
				{
					dims.push_back((int)m_dims[i].size);
				}
				pNewTensor->SetShape(dims);
				X::Value initData;
				pNewTensor->Create(initData);
				auto it_proc = [pNewTensor, this, dimCnt](std::vector<long long>& indices)
				{
					X::Value val = GetDataWithIndices(indices);
					pNewTensor->SetDataWithIndices(indices, val);
				};
				IterateAll(it_proc);
				return X::Value(pNewTensor);

			}
			//indices array needs to have all left side indices + (rightDimCount)
			FORCE_INLINE void IterateRight(TensorIterateProc proc, std::vector<long long>& indices,int rightDimCount)
			{
				int dimSize = m_dims.size();
				int startDim = dimSize - rightDimCount;
				std::vector<int> dimList;
				for (int i = startDim; i < dimSize; i++)
				{
					dimList.push_back(i);
				}
				IterateLoop(indices, startDim, proc, dimList, 0);
			}
			FORCE_INLINE void IterateLeft(TensorIterateProc proc,int leftDimCount)
			{
				std::vector<long long> indices;
				indices.resize(leftDimCount);
				std::vector<int> dimList;
				for (int i = 0; i < leftDimCount; i++)
				{
					dimList.push_back(i);
				}
				IterateLoop(indices, 0,proc, dimList, 0);
			}
			FORCE_INLINE void IterateAll(TensorIterateProc proc)
			{
				std::vector<long long> indices;
				int dimSize = m_dims.size();
				if (dimSize == 0)
					return;
				indices.resize(dimSize);
				std::vector<int> dimList;
				for (int i = 0; i < dimSize; i++)
				{
					dimList.push_back(i);
				}
				IterateLoop(indices,0,proc,dimList,0);
			}
			FORCE_INLINE void IterateLoop(std::vector<long long>& indices, int Offset,
				TensorIterateProc proc, std::vector<int>& dimList, std::vector<long long>& strides)
			{
				long long itemCount = 1;
				int dimListCnt = (int)dimList.size();
				//init indices to 0 with starting from offset
				for (int i = Offset; i < indices.size(); i++)
				{
					auto dimIdex = dimList[i- Offset];
					indices[dimIdex] = 0;
					auto& d = m_dims[dimIdex];
					itemCount *= d.size;
				}
				long long idx = 0;
				while (idx++ < itemCount)
				{
					proc(indices);

					auto lastDimListIdx = dimList[dimListCnt-1];
					auto& d = m_dims[lastDimListIdx];
					indices[lastDimListIdx]+= strides[lastDimListIdx];
					while (indices[lastDimListIdx] == d.size)
					{
						indices[lastDimListIdx] = 0;
						if (lastDimListIdx == 0)
						{
							break;
						}
						lastDimListIdx--;
						indices[lastDimListIdx]+= strides[lastDimListIdx];
					}
				}
			}
			//before call,indices need has same size of m_dims
			void IterateLoop(std::vector<long long>&indices,int Offset,
				TensorIterateProc proc, std::vector<int>& dimList ,int level=0)
			{
				auto lastLevel = dimList.size()-1;
				auto& dim = m_dims[dimList[level]];
				for (long long i = 0; i < dim.size; i++)
				{
					indices[Offset+level] = i;
					if (lastLevel == level)
					{
						proc(indices);
					}
					else
					{
						IterateLoop(indices, Offset,proc, dimList,level + 1);
					}
				}
			}
			virtual void SetDataType(TensorDataType t) override
			{
				m_dataType = t;
			}
			virtual TensorDataType GetDataType() override
			{
				return m_dataType;
			}
			virtual bool Multiply(const X::Value& r, X::Value& retVal) override;
			virtual bool Divide(const X::Value& r, X::Value& retVal) override;
			virtual bool Divided(const X::Value& leftValue, X::Value& retVal) override;
			virtual bool Add(const X::Value& r, X::Value& retVal) override;
			virtual bool Minus(const X::Value& r, X::Value& retVal) override;
			virtual bool Minuend(const X::Value& leftValue, X::Value& retVal) override;
			virtual Tensor& operator *=(X::Value& r) override;
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual X::Value UpdateItemValue(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				std::string itemName, X::Value& val) override;
			virtual bool Create(X::Value& initData) override;
			static AST::Scope* GetBaseScope();
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			virtual const char* ToString(bool WithFormat = false) override
			{
				AutoLock autoLock(m_lock);
				std::string strShapes;
				std::string strElements;
				char v[1000];
				int dcnt = (int)m_dims.size();
				for (int i=0;i<dcnt;i++)
				{
					auto& d = m_dims[i];
					if (i > 0)
					{
						strShapes += ",";
					}
					snprintf(v, sizeof(v), "%lld",d.size);
					strShapes += v;
				}
				std::string strOut = "Tensor(size=(" + strShapes + "),";
				strOut +="[";

				auto it_proc = [this, &strElements, v](std::vector<long long>& indices)
				{
					X::Value val = GetDataWithIndices(indices);
					//char v[1000];
					memset ((void *)v, 0, sizeof(v));
					switch (m_dataType)
					{
					case X::TensorDataType::BOOL:
						if (val)
							snprintf((char *)v, sizeof(v), "%s","True");
						else
							snprintf((char *)v, sizeof(v), "%s","False");
						break;
					case X::TensorDataType::BYTE:
						break;
					case X::TensorDataType::UBYTE:
						break;
					case X::TensorDataType::SHORT:
						snprintf((char *)v, sizeof(v), "%hd",(short)val.GetDouble());
						break;
					case X::TensorDataType::USHORT:
						snprintf((char *)v, sizeof(v), "%hu",(unsigned short)val.GetDouble());
						break;
					case X::TensorDataType::HALFFLOAT:
						snprintf((char *)v, sizeof(v), "%f",val.GetDouble());
						break;
					case X::TensorDataType::INT:
						snprintf((char *)v, sizeof(v), "%d",(int)val.GetLongLong());
						break;
					case X::TensorDataType::UINT:
						snprintf((char *)v, sizeof(v), "%u",(unsigned int)val.GetLongLong());
						break;
					case X::TensorDataType::LONGLONG:
						snprintf((char *)v, sizeof(v), "%lld",(long long)val.GetLongLong());
						break;
					case X::TensorDataType::ULONGLONG:
						snprintf((char *)v, sizeof(v), "%lld",(unsigned long long)val.GetLongLong());
						break;
					case X::TensorDataType::BFLOAT16:
					case X::TensorDataType::FLOAT:
						snprintf((char *)v, sizeof(v), "%f",(float)val.GetDouble());
						break;
					case X::TensorDataType::DOUBLE:
						snprintf((char *)v, sizeof(v), "%lf",(double)val.GetDouble());
						break;
					case X::TensorDataType::CFLOAT:
						//todo:
						break;
					case X::TensorDataType::CDOUBLE:
						//todo:
						break;
					default:
						break;
					}				
					strElements.append(v);
					strElements.append(",");
				};

				IterateAll(it_proc);			
				if (!strElements.empty())
				{
					if (strElements[strElements.size() - 1] == ',')
					{
						strElements[strElements.size() - 1] = ']';
					}
				}
				strOut += strElements; 
				strOut += ")";

				return GetABIString(strOut);
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
			{
				AutoLock autoLock(m_lock);
				Object::ToBytes(rt, pContext, stream);

				// Serialize tensor metadata
				stream << m_name;
				stream << static_cast<int>(m_dataType);
				size_t dimCount = m_dims.size();
				stream << dimCount;
				for (const auto& dim : m_dims)
				{
					stream << dim.offset << dim.size << dim.stride << dim.dimProd;
				}

				// Serialize tensor data
				size_t dataSize = GetDataSize();
				stream << dataSize;
				if (m_data && dataSize > 0)
				{
					stream.append(m_data, dataSize);
				}

				return true;
			}

			virtual bool FromBytes(X::XLangStream& stream) override
			{
				AutoLock autoLock(m_lock);
				Object::FromBytes(stream);

				// Deserialize tensor metadata
				stream >> m_name;
				int dtype;
				stream >> dtype;
				m_dataType = static_cast<TensorDataType>(dtype);

				size_t dimCount;
				stream >> dimCount;
				m_dims.resize(dimCount);
				for (size_t i = 0; i < dimCount; i++)
				{
					stream >> m_dims[i].offset >> m_dims[i].size >> m_dims[i].stride >> m_dims[i].dimProd;
				}

				// Deserialize tensor data
				size_t dataSize;
				stream >> dataSize;
				if (dataSize > 0)
				{
					if (m_data)
					{
						delete[] m_data;
					}
					m_data = new char[dataSize];
					m_dataSize = dataSize;
					stream.CopyTo(m_data, dataSize);
				}

				return true;
			}

		};
	}
}
