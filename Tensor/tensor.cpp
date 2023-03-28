#include "tensor.h"
#include "list.h"
#include "dict.h"
#include "port.h"
#include "function.h"
#include "ops_mgt.h"
#include "tensor_expression.h"
#include "utility.h"

namespace X
{
	namespace Data
	{
		class TensorScope :
			virtual public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			TensorScope() :
				Scope()
			{
				Init();
			}
			void clean()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
					m_stackFrame = nullptr;
				}
			}
			~TensorScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::StackFrame(this);
				//add datatype
				std::vector<std::string> dataTypeList = {
					"bool",
					"int8","uint8",
					"int16","uint16",
					"half",
					"int","uint",
					"int64","uint64",
					"float",
					"double",
					"cfloat",
					"cdouble"
				};
				int const_cnt = (int)dataTypeList.size();
				int func_cnt = 3;
				m_stackFrame->SetVarCount(const_cnt+ func_cnt);
				for(int i = 0;i < const_cnt; i++)
				{
					int idx = AddOrGet(dataTypeList[i], false);
					X::Value  val(i);
					m_stackFrame->Set(idx, val);
				}
				std::string strName;
				{
					strName = "randwithshape";
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"randwithshape(shapes in list)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							X::ARGS& params,
							X::KWARGS& kwParams,
							X::Value& retValue)
							{
								Tensor* pObj = dynamic_cast<Tensor*>(pContext);
								auto& p0 = params[0];
								if (p0.IsList())
								{
									List* pList = dynamic_cast<List*>(p0.GetObj());
									int axesCnt = (int)pList->Size();
									double dMin = 0;
									double dMax = 1;
									long long llMin = LLONG_MIN;
									long long llMax = LLONG_MAX;
									TensorDataType dt = TensorDataType::DOUBLE;
									auto it = kwParams.find(Tensor_DType);
									if (it != kwParams.end())
									{
										dt = (TensorDataType)(int)it->second;
									}
									if (dt == TensorDataType::DOUBLE)
									{
										it = kwParams.find(Tensor_Max);
										if (it != kwParams.end())
										{
											dMax = (double)it->second;
										}
										it = kwParams.find(Tensor_Min);
										if (it != kwParams.end())
										{
											dMin = (double)it->second;
										}
									}
									else
									{
										it = kwParams.find(Tensor_Max);
										if (it != kwParams.end())
										{
											llMax = (long long)it->second;
										}
										it = kwParams.find(Tensor_Min);
										if (it != kwParams.end())
										{
											llMin = (long long)it->second;
										}
									}
									Tensor* pNewTensor = new Tensor();
									pNewTensor->SetDataType(dt);
									std::vector<int> dims(axesCnt);
									for (int i = 0; i < axesCnt; i++)
									{
										dims[i]=(int)pList->Get(i);
									}
									pNewTensor->SetShape(dims);
									X::Value initData;
									pNewTensor->Create(initData);
									auto it_proc = [pNewTensor,dMin,dMax](std::vector<long long>& indices)
									{
										X::Value val = randDouble(dMin, dMax);
										pNewTensor->SetDataWithIndices(indices, val);
									};
									auto it_proc_int64 = [pNewTensor,llMin,llMax](std::vector<long long>& indices)
									{
										X::Value val= rand64(llMin,llMax);
										pNewTensor->SetDataWithIndices(indices, val);
									};
									if (dt == TensorDataType::DOUBLE)
									{
										pNewTensor->IterateAll(it_proc);
									}
									else
									{
										pNewTensor->IterateAll(it_proc_int64);
									}
									retValue = X::Value(pNewTensor);
								}
								return true;
							}));
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
				{
					strName = "astype";
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"astype(type)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							X::ARGS& params,
							X::KWARGS& kwParams,
							X::Value& retValue)
							{
								Tensor* pObj = dynamic_cast<Tensor*>(pContext);
								retValue = pObj->asType((int)params[0]);
								return true;
							}));
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
				{
					strName = "Conv2d";
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"tensor1*Conv2d()*kernel_tensor",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							X::ARGS& params,
							X::KWARGS& kwParams,
							X::Value& retValue)
							{
								Tensor_OperatorHandler handler;
								TensorOperator* pOp = new TensorOperator(handler,false);
								X::Value action;
								pOp->SetOpAction(action);
								retValue = X::Value(pOp);
								return true;
							}));
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
			}
			// Inherited via Scope
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
				LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
		};
		static TensorScope _TensorScope;
		void Tensor::cleanup()
		{
			_TensorScope.clean();
		}

		Tensor::Tensor():XTensor(0),
			Object()
		{
			m_t = ObjType::Tensor;

		}
		bool Tensor::Get(std::vector<Data::TensorIndex>& IdxAry, X::Value& retVal)
		{
			//check if point to one item,not a tensor
			std::vector<long long> indices;
			int newStartDim = 0;
			for (auto& idx : IdxAry)
			{
				if (idx.i == idx.j)
				{
					newStartDim++;
					indices.push_back(idx.i);
				}
				else
				{
					break;
				}
			}
			//all same index,point to an item, not a tensor
			if (newStartDim == (int)m_dims.size())
			{
				retVal = GetDataWithIndices(indices);
			}
			else
			{
				auto* pNewTensor = new X::Data::Tensor();
				pNewTensor->m_dataType = m_dataType;
				//keep this tensor as New Tensor's ref
				IncRef();
				pNewTensor->m_pTensorToOwneData = this;
				pNewTensor->m_data = m_data;
				long long newStartItemOffet = 0;
				for (int i = 0; i < newStartDim; i++)
				{
					TensorDim& dimInfo = m_dims[i];
					newStartItemOffet += (IdxAry[i].i+ dimInfo.offset) * dimInfo.dimProd;
				}
				pNewTensor->m_startItemOffet = newStartItemOffet;
				for (int i = newStartDim; i < (int)m_dims.size(); i++)
				{
					TensorDim& dimInfo = m_dims[i];
					TensorDim newDimInfo;
					newDimInfo.dimProd = 0;//init as 0, calc later
					newDimInfo.stride = dimInfo.stride;
					if (i < (int)IdxAry.size())
					{
						auto& idxInfo = IdxAry[i];
						newDimInfo.offset = dimInfo.offset+idxInfo.i;
						//stride keep same,because the memory is from the first tensor
						long long newSize = 0;
						if (idxInfo.j < 0 && idxInfo.i>0)
						{
							newSize = dimInfo.size + idxInfo.j - idxInfo.i + 1;
						}
						else if (idxInfo.j > 0 && idxInfo.i < 0)
						{
							newSize = idxInfo.j - (dimInfo.size + idxInfo.i) + 1;
						}
						else
						{
							newSize = idxInfo.j - idxInfo.i + 1;
						}
						newDimInfo.size = newSize;
					}
					else
					{
						newDimInfo.offset = dimInfo.offset;
						newDimInfo.size = dimInfo.size;
					}
					pNewTensor->m_dims.push_back(newDimInfo);
				}
				pNewTensor->CalcDimProd();
				retVal = X::Value(pNewTensor);
			}
			return true;
		}
		
		X::Value Tensor::GetDataWithIndices(std::vector<long long>& indices)
		{
			std::cout << "In GetDataWithIndices(), indices.size = " << indices.size()<<", indice[0] = "<<indices[0] << std::endl;
			long long addr = CalcItemOffset(indices);
			X::Value retVal;
			char* pAddr = m_data + addr;
			switch (m_dataType)
			{
			case X::TensorDataType::BOOL:
				retVal = X::Value((bool)*(char*)pAddr);
				break;
			case X::TensorDataType::BYTE:
				retVal = X::Value(*(char*)pAddr);
				break;
			case X::TensorDataType::UBYTE:
				retVal = X::Value(*(char*)pAddr);
				break;
			case X::TensorDataType::SHORT:
				retVal = X::Value((int)*(short*)pAddr);
				break;
			case X::TensorDataType::USHORT:
				retVal = X::Value((int)*(unsigned short*)pAddr);
				break;
			case X::TensorDataType::HALFFLOAT:
				retVal = X::Value((float)*(short*)pAddr);
				break;
			case X::TensorDataType::INT:
				retVal = X::Value(*(int*)pAddr);
				break;
			case X::TensorDataType::UINT:
				retVal = X::Value(*(unsigned int*)pAddr);
				break;
			case X::TensorDataType::LONGLONG:
				retVal = X::Value(*(long long*)pAddr);
				break;
			case X::TensorDataType::ULONGLONG:
				retVal = X::Value(*(unsigned long long*)pAddr);
				break;
			case X::TensorDataType::FLOAT:
				retVal = X::Value(*(float*)pAddr);
				break;
			case X::TensorDataType::DOUBLE:
				retVal = X::Value(*(double*)pAddr);
				break;
			case X::TensorDataType::CFLOAT:
				break;
			case X::TensorDataType::CDOUBLE:
				break;
			default:
				break;
			}
			return retVal;
		}
		void Tensor::SetDataWithIndices(std::vector<long long>& indices,X::Value& val)
		{
			std::cout << "In SetDataWithIndices(), indices.size = " << indices.size()<<", indice[0] = "<<indices[0] << ", Val = " << val.GetLongLong() << std::endl;

			long long addr = CalcItemOffset(indices);
			X::Value retVal;
			char* pAddr = m_data + addr;
			//Set value to this addr 
			switch (m_dataType)
			{
			case X::TensorDataType::BOOL:
				*(char*)pAddr = (char)(bool)val;
				break;
			case X::TensorDataType::BYTE:
				*(char*)pAddr = (char)(int)val;
				break;
			case X::TensorDataType::UBYTE:
				*(unsigned char*)pAddr = (unsigned char)(int)val;
				break;
			case X::TensorDataType::SHORT:
				*(short*)pAddr = (short)(int)val;
				break;
			case X::TensorDataType::USHORT:
				*(unsigned short*)pAddr = (unsigned short)(int)val;
				break;
			case X::TensorDataType::HALFFLOAT:
				*(unsigned short*)pAddr = (unsigned short)(float)val;
				break;
			case X::TensorDataType::INT:
				*(int*)pAddr = (int)val;
				break;
			case X::TensorDataType::UINT:
				*(unsigned int*)pAddr = (unsigned int)val;
				break;
			case X::TensorDataType::LONGLONG:
				*(long long*)pAddr = (long long)val;
				break;
			case X::TensorDataType::ULONGLONG:
				*(unsigned long long*)pAddr = (unsigned long long)val;
				break;
			case X::TensorDataType::FLOAT:
				*(float*)pAddr = (float)val;
				break;
			case X::TensorDataType::DOUBLE:
				*(double*)pAddr = (double)val;
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

		}
		void Tensor::DeepCopyDataFromList(List* pList, std::vector<long long>& indices, int level)
		{
			long long size = pList->Size();
			for (long long i = 0; i < size; i++)
			{
				auto item = pList->Get(i);
				indices[level] = i;
				if (item.IsList())
				{
					auto* pList_NextLevel = dynamic_cast<List*>(item.GetObj());
					DeepCopyDataFromList(pList_NextLevel, indices, level+1);
				}
				else
				{
					//last layer
					SetDataWithIndices(indices, item);
				}
			}
		}
		List* Tensor::FlatPack(XlangRuntime* rt, XObj* pContext, 
			std::vector<std::string>& IdList, int id_offset, 
			long long startIndex, long long count)
		{
			AutoLock(m_lock);
			int idSize = (int)IdList.size();
			std::vector<long long> indices;
			for(int i= id_offset;i< idSize;i++)
			{
				long long index = 0;
				SCANF(IdList[i].c_str(), "%lld", &index);
				indices.push_back(index);
			}
			int dimCount = (int)m_dims.size();
			int lastDimIndex = (int)indices.size();
			long long lastDimSize = m_dims[lastDimIndex].size;
			if (startIndex < 0 || startIndex >= lastDimSize)
			{
				return nullptr;
			}
			if (count == -1)
			{
				count = lastDimSize - startIndex;
			}
			if ((startIndex + count) > lastDimSize)
			{
				return nullptr;
			}
			indices.push_back(startIndex);
			List* pOutList = new List();
			pOutList->IncRef();
			for (long long i = 0; i < count; i++)
			{
				long long idx = startIndex + i;
				Dict* dict = new Dict();
				auto objIds = CombinObjectIds(IdList, (unsigned long long)idx);
				dict->Set("Id", objIds);

				if (lastDimIndex < (dimCount - 1))
				{
					//still return Tensor as ValueType
					Data::Str* pStrType = new Data::Str(GetTypeString());
					dict->Set("Type", X::Value(pStrType));
					X::Value objId((unsigned long long)dynamic_cast<XObj*>(this));
					dict->Set("Value", objId);
					X::Value valSize(m_dims[lastDimIndex+1].size);
					dict->Set("Size", valSize);
				}
				else
				{
					indices[indices.size() - 1] = idx;
					X::Value val = GetDataWithIndices(indices);
					auto valType = val.GetValueType();
					Data::Str* pStrType = new Data::Str(valType);
					dict->Set("Type", X::Value(pStrType));

					if (!val.IsObject() || (val.IsObject() &&
						dynamic_cast<Object*>(val.GetObj())->IsStr()))
					{
						dict->Set("Value", val);
					}
					else if (val.IsObject())
					{
						X::Value objId((unsigned long long)val.GetObj());
						dict->Set("Value", objId);
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
				}

				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
		X::Value Tensor::UpdateItemValue(XlangRuntime* rt, XObj* pContext, std::vector<std::string>& IdList, int id_offset, std::string itemName, X::Value& val)
		{
			return X::Value();
		}
		bool Tensor::Create(X::Value& initData)
		{
			X::Value dimData = initData;
			while (dimData.IsList())
			{
				List* pList = dynamic_cast<List*>(dimData.GetObj());
				long long dimSize = pList->Size();
				m_dims.push_back(TensorDim{ 0,dimSize,dimSize });
				if (dimSize > 0)
				{
					dimData = pList->Get(0);
					if (!dimData.IsList())
					{
						auto ty = dimData.GetType();
						switch (ty)
						{
						case X::ValueType::Int64:
							m_dataType = TensorDataType::LONGLONG;
							break;
						case X::ValueType::Double:
							m_dataType = TensorDataType::DOUBLE;
							break;
						default:
							break;
						}
						break;
					}
				}
			}
			CalcDimProd();
			long long totalSize = GetCount() * GetItemSize();
			if (totalSize > 0)
			{
				m_data = new char[totalSize];
				m_dataSize = totalSize;
			}
			int dim = (int)m_dims.size();
			//copy data from initData
			if (initData.IsList())
			{
				std::vector<long long> indices;
				indices.resize(dim);
				List* pList = dynamic_cast<List*>(initData.GetObj());
				DeepCopyDataFromList(pList, indices, 0);
			}

			return true;
		}
		Tensor::~Tensor()
		{
			if (m_pTensorToOwneData)
			{
				m_pTensorToOwneData->DecRef();
			}
			else if(m_data != nullptr)
			{
				delete m_data;
			}
		}
		Tensor& Tensor::operator *=(X::Value& r)
		{
			//todo:
			//SetRightVal(r, Tensor_Operator::Mul);
			return *this;
		}
		AST::Scope* Tensor::GetBaseScope() { return &_TensorScope; }
		void Tensor::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(&_TensorScope);
		}
		bool Tensor::Multiply(const X::Value& r, X::Value& retVal)
		{
			auto* newTensor = new TensorExpression();
			X::Value left(this);
			newTensor->SetLeftVal(left);
			X::Value right(r);
			newTensor->SetRightVal(right, Tensor_Operator::Mul);
			std::string newName = OpsManager::I().GenNewName();
			newTensor->SetName(newName);
			retVal = newTensor;
			return true;
		}
		bool Tensor::Add(const X::Value& r, X::Value& retVal)
		{
			auto* newTensor = new TensorExpression();
			X::Value left(this);
			newTensor->SetLeftVal(left);
			X::Value right(r);
			newTensor->SetRightVal(right, Tensor_Operator::Add);
			//if left has name, then add new tensor with a name
			std::string newName = OpsManager::I().GenNewName();
			newTensor->SetName(newName);
			retVal = newTensor;
			return true;
		}
	}
}