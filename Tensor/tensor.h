#pragma once
#include "object.h"
#include <functional>
#include "limits.h"
#include "tensorop.h"
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
			TensorDataType m_dataType;

			long long CalcItemOffset(std::vector<long long>& indices)
			{
				long long off = m_startItemOffet;
				int idxCnt = (int)indices.size();
				for (int i = 0; i < idxCnt; i++)
				{
					auto& dim = m_dims[i];
					off += (indices[i] + dim.offset) * dim.dimProd;
				}
				off *= GetItemSize();
				return off;
			}
			long long GetItemSize()
			{
				long long size = 1;
				switch (m_dataType)
				{
				case X::TensorDataType::BOOL:
				case X::TensorDataType::BYTE:
				case X::TensorDataType::UBYTE:
					size = 1;
					break;
				case X::TensorDataType::SHORT:
				case X::TensorDataType::USHORT:
				case X::TensorDataType::HALFFLOAT:
					size = 2;
					break;
				case X::TensorDataType::INT:
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
				default:
					break;
				}
				return size;
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
			void DeepCopyDataFromList(List* pList,std::vector<long long>& indices,int level);
			void CalcDimProd()
			{
				int nd = (int)m_dims.size();
				long long a = 1;
				m_dims[nd - 1].dimProd = a;
				for (int i = nd - 1; i >= 1; i--)
				{
					//use stride instead of size becuase view of Tensor will change size 
					//but stride keeps same as original
					a *= m_dims[i].stride;
					m_dims[i - 1].dimProd = a;
				}
			}
		public:
			static void cleanup();
			Tensor();
			~Tensor();

			inline void SetName(std::string& n)
			{
				m_name = n;
			}
			inline std::string& GetName() { return m_name; }
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
			std::vector<TensorDim> GetDims() 
			{
				return m_dims;
			}
			inline virtual int GetDimCount() override
			{
				return (int)m_dims.size();
			}
			inline virtual long long GetDimSize(int dimIdx) override
			{
				return m_dims[dimIdx].size;
			}
			virtual void SetShape(std::vector<int> shapes) override
			{
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
			X::Value GetDataWithIndices(std::vector<long long>& indices);
			inline X::Value asType(int type)
			{
				TensorDataType dt = (TensorDataType)type;
				Tensor* pNewTensor = new Tensor();
				pNewTensor->SetDataType(dt);
				int dimCnt = (int)m_dims.size();
				std::vector<int> dims;
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
			inline X::Value permute(std::vector<int> axes)
			{
				Tensor* pNewTensor = new Tensor();
				pNewTensor->SetDataType(m_dataType);
				int dimCnt = (int)m_dims.size();
				std::vector<int> dims;
				for (int i=0;i< dimCnt;i++)
				{
					dims.push_back((int)m_dims[axes[i]].size);
				}
				pNewTensor->SetShape(dims);
				X::Value initData;
				pNewTensor->Create(initData);
				auto it_proc = [pNewTensor,this, dimCnt, axes](std::vector<long long>& indices)
				{
					std::vector<long long> target_indices;
					for (int i = 0; i < dimCnt; i++)
					{
						target_indices.push_back(indices[axes[i]]);
					}
					X::Value val = GetDataWithIndices(indices);
					pNewTensor->SetDataWithIndices(target_indices, val);
				};
				IterateAll(it_proc);
				return X::Value(pNewTensor);
			}
			
			inline void IterateAll(TensorIterateProc proc)
			{
				std::vector<long long> indices;
				indices.resize(m_dims.size());
				IterateLoop(indices, proc, 0);
			}
			//before call,indices need has same size of m_dims
			inline void IterateLoop(std::vector<long long>&indices, TensorIterateProc proc,int level=0)
			{
				if (m_dims.size() == 0)
				{
					return;
				}
				int lastDim = (int)m_dims.size()-1;
				auto& dim = m_dims[level];
				for (long long i = 0; i < dim.size; i++)
				{
					indices[level] = i;
					if (lastDim == level)
					{
						proc(indices);
					}
					else
					{
						IterateLoop(indices, proc, level + 1);
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
			virtual std::string ToString(bool WithFormat = false) override
			{
				AutoLock(m_lock);
				std::string strShapes;
				std::string *pstrElements = new std::string();
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

				auto it_proc = [this, pstrElements, v](std::vector<long long>& indices)
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
					pstrElements->append(v);
					pstrElements->append(",");
				};

				IterateAll(it_proc);			

				pstrElements->back() = 0; // to remove the last comma
				strOut += *pstrElements; 
				strOut += "]";
				strOut += ")";

				return strOut;
			}
			
		};
	}
}
