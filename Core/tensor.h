#pragma once
#include "object.h"

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
namespace X
{
	namespace Data
	{
		struct TensorDim
		{
			long long offset = 0;//in another Tensor's Row's offset
			long long size;
			long long stride;
			long long dimProd;
		};
		class Tensor:
			virtual public XTensor,
			virtual public Object
		{
			char* m_data=nullptr;
			std::vector<TensorDim> m_dims;
			TensorDataType m_dataType;

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
			void SetDataWithIndices(std::vector<long long>& indices,X::Value& val);
			void CalcDimProd()
			{
				int nd = (int)m_dims.size();
				long long a = 1;
				m_dims[nd - 1].dimProd = a;
				for (int i = nd - 1; i >= 1; i--)
				{
					a *= m_dims[i].size;
					m_dims[i - 1].dimProd = a;
				}
			}
		public:
			static void cleanup();
			Tensor();
			~Tensor();
			virtual long long Size() override
			{
				return GetCount() * GetItemSize();
			}
			virtual char* GetData() override
			{
				return m_data;
			}
			virtual void SetShape(std::vector<int> shapes) override
			{
				for (auto i : shapes)
				{
					m_dims.push_back(TensorDim{ i,i });
				}
			}
			virtual void SetDataType(TensorDataType t) override
			{
				m_dataType = t;
			}
			virtual bool Create(X::Value& initData) override;
			static AST::Scope* GetBaseScope();
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			virtual std::string ToString(bool WithFormat = false) override
			{
				AutoLock(m_lock);
				std::string strShapes;
				char v[1000];
				int dcnt = (int)m_dims.size();
				for (int i=0;i<dcnt;i++)
				{
					auto& d = m_dims[i];
					if (i > 0)
					{
						strShapes += ",";
					}
					snprintf(v, sizeof(v), "%ld",d.size);
					strShapes += v;
				}
				std::string strOut = "Tensor(size=(" + strShapes + "),";
				strOut +="[todo:output data";
				strOut += "]";
				strOut += ")";
				return strOut;
			}
		};
	}
}
