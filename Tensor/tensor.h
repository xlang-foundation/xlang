#pragma once
#include "object.h"
#include <functional>
#include "tensorop.h"
#include "ops_mgt.h"

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
			long long size;
			long long stride;
			long long dimProd;
		};
		enum class Tensor_Operator
		{
			None,Add,Minus,Mul,Div,
			Count
		};
		using TensorIterateProc = std::function<void(std::vector<long long>& indices)>;
		class Tensor:
			virtual public XTensor,
			virtual public Object
		{
			//for debug
			std::string m_name;
			//for tensor operator
			X::Value m_leftVal;
			X::Value m_rightVal; //maybe a Tensor or TensorOperator
			Tensor_Operator m_op = Tensor_Operator::None;
			void SetLeftVal(X::Value& val)
			{
				m_leftVal = val;
			}
			void SetRightVal(X::Value& val, Tensor_Operator op)
			{
				m_rightVal = val;
				m_op = op;
			}
			//for data tensor
			long long m_dataSize = 0;
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
			X::Value GetDataWithIndices(std::vector<long long>& indices);
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

			inline void SetName(std::string& n)
			{
				m_name = n;
			}
			inline std::string& GetName() { return m_name; }
			inline bool NeedCalc()
			{
				return (m_op != Tensor_Operator::None);
			}
			inline X::Value& GetLeftValue() { return m_leftVal; }
			inline X::Value& GetRightValue() { return m_rightVal; }
			inline Tensor_Operator GetOp() { return m_op; }
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
			virtual void SetShape(std::vector<int> shapes) override
			{
				for (auto i : shapes)
				{
					m_dims.push_back(TensorDim{ i,i });
				}
			}
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
			virtual bool Multiply(const X::Value& r, X::Value& retVal) override
			{
				auto* newTensor = new Tensor();
				X::Value left(this);
				newTensor->SetLeftVal(left);
				X::Value right(r);
				newTensor->SetRightVal(right,Tensor_Operator::Mul);
#if _combine_name
				//if left has name, then add new tensor with a name
				if (!m_name.empty())
				{
					std::string rightName;
					if (r.IsObject() && r.GetObj()->GetType() == ObjType::Tensor)
					{
						Tensor* pRightTensor = dynamic_cast<Tensor*>(r.GetObj());
						if (pRightTensor)
						{
							rightName = pRightTensor->GetName();
						}
					}
					else if (r.IsObject() && r.GetObj()->GetType() == ObjType::TensorOperator)
					{
						TensorOperator* pRightTensorOp = dynamic_cast<TensorOperator*>(r.GetObj());
						if (pRightTensorOp)
						{
							rightName = pRightTensorOp->GetName();
						}
					}
					if (!rightName.empty())
					{
						std::string newName = '<'+m_name + "*" + rightName+ '>';
						newTensor->SetName(newName);
					}
				}
#else
				std::string newName = OpsManager::I().GenNewName();
				newTensor->SetName(newName);
#endif
				retVal = newTensor;
				return true;
			}
			virtual bool Add(const X::Value& r, X::Value& retVal) override
			{
				auto* newTensor = new Tensor();
				X::Value left(this);
				newTensor->SetLeftVal(left);
				X::Value right(r);
				newTensor->SetRightVal(right, Tensor_Operator::Add);
				//if left has name, then add new tensor with a name
#if _combine_name
				if (!m_name.empty())
				{
					std::string rightName;
					if (r.IsObject() && r.GetObj()->GetType() == ObjType::Tensor)
					{
						Tensor* pRightTensor = dynamic_cast<Tensor*>(r.GetObj());
						if (pRightTensor)
						{
							rightName = pRightTensor->GetName();
						}
					}
					else if (r.IsObject() && r.GetObj()->GetType() == ObjType::TensorOperator)
					{
						TensorOperator* pRightTensorOp = dynamic_cast<TensorOperator*>(r.GetObj());
						if (pRightTensorOp)
						{
							rightName = pRightTensorOp->GetName();
						}
					}
					if (!rightName.empty())
					{
						std::string newName = '<' + m_name + "+" + rightName + '>';
						newTensor->SetName(newName);
					}
				}
#else
				std::string newName = OpsManager::I().GenNewName();
				newTensor->SetName(newName);
#endif
				retVal = newTensor;
				return true;
			}
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
