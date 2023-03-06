#include "tensor.h"
#include "list.h"

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
				int cnt = (int)dataTypeList.size();
				m_stackFrame->SetVarCount(cnt);
				for(int i = 0;i < cnt; i++)
				{
					int idx = AddOrGet(dataTypeList[i], false);
					X::Value  val(i);
					m_stackFrame->Set(idx, val);
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
		AST::Scope* Tensor::GetBaseScope() { return &_TensorScope; }
		void Tensor::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(&_TensorScope);
		}
	}
}