#pragma once
#include "object.h"
#include <functional>
#include <math.h>

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
			None,Add,Minus,Mul,Div
		};
		using TensorIterateProc = std::function<void(std::vector<long long>& indices)>;
		class Tensor:
			virtual public XTensor,
			virtual public Object
		{
			//for tensor operator
			X::Value m_rightVal; //maybe a Tensor or TensorOperator
			Tensor_Operator m_op = Tensor_Operator::None;
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

			bool IsSimilarTensor (const Tensor& t) {
				bool sim = true;
				int nd = (int)m_dims.size();
				int td = (int)t.m_dims.size();
				if (nd != td) {//
					sim = false;
				}
				else {
					for (int i = 0; i < nd; i++) {
						if (m_dims[i].size != t.m_dims[i].size) {
							sim = false;
							break;
						}
					}
				}
				return sim;
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

			inline bool NeedCalc()
			{
				return (m_op != Tensor_Operator::None);
			}
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
			
			inline bool IsAddable(const X::Value& operand) 
			{
				bool Addable = false;
				auto ty = ((X::Value)operand).GetType();

				if (ty == X::ValueType::Object) {//only tensor, no list, set, dictionary, complex, etc.
					auto* pObj = ((X::Value)operand).GetObj();
					if (pObj->GetType() == ObjType::Tensor)
					{
						Tensor* pTensor = dynamic_cast<Tensor*> (pObj);
						Addable = IsSimilarTensor(*pTensor);					
					}
					return Addable;
				}

				bool IsNum = false;
				auto val = 0;
				switch (ty)
				{
				case X::ValueType::Int64:
					IsNum = true;
					val = ((X::Value)operand).GetLongLong();
					break;
				case X::ValueType::Double:
					IsNum = true;
					val = ((X::Value)operand).GetDouble();
					break;
				default:  //todo, what about boolean?
					break;
				}

				if (IsNum) //number only
				{
					switch (m_dataType)
					{
					case X::TensorDataType::BOOL:
						break;
					case X::TensorDataType::BYTE:
						if (ty == X::ValueType::Int64 && val >= (-1)*pow(2,7) && val < pow(2,7) ) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::UBYTE:
						if (ty == X::ValueType::Int64 && val < pow(2,8)) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::SHORT:
						if (ty == X::ValueType::Int64 && val >= (-1)*pow(2,15) && val < pow(2,15) ) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::USHORT:
						if (ty == X::ValueType::Int64 && val < pow(2,16)) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::INT:
						if (ty == X::ValueType::Int64 && val >= (-1)*pow(2,31) && val < pow(2,31)) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::UINT:
						if (ty == X::ValueType::Int64 && val < pow(2,32)) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::LONGLONG:
					case X::TensorDataType::ULONGLONG:
						if (ty == X::ValueType::Int64) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::HALFFLOAT:
						if ((ty == X::ValueType::Int64 || ty == X::ValueType::Double) && val < pow(2,16)) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::FLOAT:
						if ((ty == X::ValueType::Int64 || ty == X::ValueType::Double) && val < pow(2,32)) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::DOUBLE:
					case X::TensorDataType::CFLOAT:
						if (ty == X::ValueType::Int64 || ty == X::ValueType::Double) 
						{
							Addable = true;
						}
						break;
					case X::TensorDataType::CDOUBLE:
						break;
					default:
						break;
					}
				}
				return Addable;
			}

/*
			inline void Add(const X::Value& operand)
			{
				AutoLock(m_lock);
			
				auto it_proc = [this, operand](std::vector<long long>& indices)
				{
					X::Value val = GetDataWithIndices(indices);
					if (operand.IsObject())
					{
						auto* pObj = val.GetObj();
						if (pObj->GetType() == ObjType::Tensor ) {
							Tensor* pTobj = dynamic_cast<Tensor*>(pObj);
							if (IsSimilarTensor(*pTobj)) {
								X::Value val_operand = pTobj->GetDataWithIndices(indices);
								//SetDataWithIndices(indices, val+= val_operand);
								SetDataWithIndices(indices, val_operand);
							}
						}
						else {
							//exceptions
							return;
						}
					}
					else {
						//SetDataWithIndices(indices, val+= operand);
						SetDataWithIndices(indices, operand);
					}
				};

				IterateAll(it_proc);
			}
*/			
			inline void Minus(const X::Value& operand)
			{
				AutoLock(m_lock);
			
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
