#include "tensor.h"
#include "list.h"
#include "dict.h"
#include "port.h"
#include "function.h"
#include "tensorop.h"

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
					strName = "permute";
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"permute(list of indices)",
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
									std::vector<int> axes;
									for (int i = 0; i < axesCnt; i++)
									{
										axes.push_back((int)pList->Get(i));
									}
									retValue = pObj->permute(axes);
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
		X::Value Tensor::GetDataWithIndices(std::vector<long long>& indices)
		{
			long long addr = 0;
			int idxCnt = (int)indices.size();
			for (int i = 0; i < idxCnt; i++)
			{
				addr += indices[i] * m_dims[i].dimProd;
			}
			X::Value retVal;
			addr *= GetItemSize();
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
			long long addr = 0;
			int idxCnt = (int)indices.size();
			for (int i = 0; i < idxCnt; i++)
			{
				addr += indices[i] * m_dims[i].dimProd;
			}
			addr *= GetItemSize();
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
				m_dims.push_back(TensorDim{ dimSize,dimSize });
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
			if (m_data)
			{
				delete m_data;
			}
		}
		Tensor& Tensor::operator *=(X::Value& r)
		{
			printf ("in operator *=\n");
			Multiply(r, Tensor_Operator::Mul);
			SetRightVal(r, Tensor_Operator::Mul);
			return *this;
		}
		AST::Scope* Tensor::GetBaseScope() { return &_TensorScope; }
		void Tensor::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(&_TensorScope);
		}
	}
}