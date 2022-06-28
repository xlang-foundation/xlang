#include "str.h"
#include "scope.h"
#include "list.h"
#include <string>
namespace X
{
	namespace Data
	{
		class StrOp :
			public AST::Scope
		{
			std::vector<AST::Value> m_funcs;
		public:
			StrOp()
			{
				m_Vars = 
				{ 
					{"find",0},
					{"rfind",1},
					{"slice",2},
					{"size",3},
					{"split",4}
				};
				{
					std::string name("find");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
							(AST::U_FUNC)([](X::Runtime* rt, void* pContext,
								ARGS& params,
								KWARGS& kwParams,
					X::AST::Value& retValue)
					{
						auto* pObj = (Str*)pContext;
						std::string x = params[0].ToString();
						size_t offset = 0;
						if (params.size() > 1)
						{
							offset = params[1].GetLongLong();
						}
						auto pos = pObj->Find(x, offset);
						retValue = AST::Value((long long)pos);
						return true;
					}));
					auto* pFuncObj = new Data::Function(extFunc);
					m_funcs.push_back(AST::Value(pFuncObj));
				}
				{
					std::string name("rfind");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						(AST::U_FUNC)([](X::Runtime* rt, void* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::AST::Value& retValue)
							{
								auto* pObj = (Str*)pContext;
								std::string x = params[0].ToString();
								size_t offset = std::string::npos;
								if (params.size() > 1)
								{
									offset = params[1].GetLongLong();
								}
								auto pos = pObj->RFind(x, offset);
								retValue = AST::Value((long long)pos);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc);
					m_funcs.push_back(AST::Value(pFuncObj));
				}
				{
					std::string name("slice");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						(AST::U_FUNC)([](X::Runtime* rt, void* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::AST::Value& retValue)
							{
								auto* pObj = (Str*)pContext;
								size_t start = 0;
								size_t end = -1;
								if (params.size() >= 1)
								{
									start = params[0].GetLongLong();
								}
								if (params.size() >= 2)
								{
									end = params[1].GetLongLong();
								}
								std::string retStr;
								pObj->Slice(start, end, retStr);
								Str* pNewStr = new Str((const char*)retStr.c_str(), retStr.size());
								retValue = AST::Value(pNewStr);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc);
					m_funcs.push_back(AST::Value(pFuncObj));
				}
				{
					std::string name("size");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						(AST::U_FUNC)([](X::Runtime* rt, void* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::AST::Value& retValue)
							{
								auto* pObj = (Str*)pContext;
								size_t  size = pObj->GetSize();
								retValue = AST::Value((long long)size);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc);
					m_funcs.push_back(AST::Value(pFuncObj));
				}
				{
					std::string name("split");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						(AST::U_FUNC)([](X::Runtime* rt, void* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::AST::Value& retValue)
							{
								std::string delim("\n");
								if (params.size() >= 1)
								{
									delim = params[0].ToString();
								}
								auto* pObj = (Str*)pContext;
								std::vector<std::string> li;
								pObj->Split(delim, li);
								auto* pList = new List(li);
								retValue = AST::Value(pList);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc);
					m_funcs.push_back(AST::Value(pFuncObj));
				}
			}
			~StrOp()
			{
				m_funcs.clear();
			}
			virtual Scope* GetParentScope()
			{
				return nullptr;
			}
			inline virtual bool Set(Runtime* rt, void* pContext,
				int idx, AST::Value& v)
			{
				return false;
			}

			inline virtual bool Get(Runtime* rt, void* pContext,
				int idx, AST::Value& v, AST::LValue* lValue = nullptr)
			{
				Str* pStrObj = (Str*)pContext;
				v = m_funcs[idx];
				return true;
			}
		};

		static StrOp _strop;
		AST::Scope* Str::GetScope()
		{
			return &_strop;
		}
	}
}