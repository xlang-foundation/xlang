#include "str.h"
#include "scope.h"
#include "list.h"
#include "function.h"
#include <string>
#include "constexpr.h"
namespace X
{
	namespace Data
	{
		class StrOp :
			public AST::Scope
		{
			std::vector<X::Value> m_funcs;
		public:
			StrOp()
			{
				m_Vars = 
				{ 
					{"find",0},
					{"rfind",1},
					{"slice",2},
					{"size",3},
					{"split",4},
					{"splitWithChars",5}
				};
				{
					std::string name("find");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
							"pos = find(search_string)",
							(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
								ARGS& params,
								KWARGS& kwParams,
					X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string x = params[0].ToString();
						size_t offset = 0;
						if (params.size() > 1)
						{
							offset = params[1].GetLongLong();
						}
						auto pos = pStrObj->Find(x, offset);
						retValue = X::Value((long long)pos);
						return true;
					}));
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("rfind");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"pos = rfind(search_string)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								auto* pObj = dynamic_cast<Object*>(pContext);
								auto* pStrObj = dynamic_cast<Str*>(pObj);
								std::string x = params[0].ToString();
								size_t offset = std::string::npos;
								if (params.size() > 1)
								{
									offset = params[1].GetLongLong();
								}
								auto pos = pStrObj->RFind(x, offset);
								retValue = X::Value((long long)pos);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("slice");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"newStr = var_str.slice(startPos[,endPos])",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								auto* pObj = dynamic_cast<Object*>(pContext);
								auto* pStrObj = dynamic_cast<Str*>(pObj);
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
								pStrObj->Slice(start, end, retStr);
								Str* pNewStr = new Str((const char*)retStr.c_str(), (int)retStr.size());
								retValue = X::Value(pNewStr);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("size");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"size()",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								auto* pObj = dynamic_cast<Object*>(pContext);
								auto* pStrObj = dynamic_cast<Str*>(pObj);
								size_t  size = pStrObj->GetSize();
								retValue = X::Value((long long)size);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("split");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"item_list = var_str.split(delimiter_str)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								std::string delim("\n");
								if (params.size() >= 1)
								{
									delim = params[0].ToString();
								}
								auto* pObj = dynamic_cast<Object*>(pContext);
								auto* pStrObj = dynamic_cast<Str*>(pObj);
								std::vector<std::string> li;
								pStrObj->Split(delim, li);
								auto* pList = new List(li);
								pList->IncRef();
								XObj* pObjList = dynamic_cast<XObj*>(pList);
								retValue = X::Value(pObjList,false);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("splitWithChars");
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"item_list = var_str.splitWithChars(delimiter_chars)",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							ARGS& params,
							KWARGS& kwParams,
							X::Value& retValue)
							{
								std::string delim("\n");
								if (params.size() >= 1)
								{
									delim = params[0].ToString();
								}
								auto* pObj = dynamic_cast<Object*>(pContext);
								auto* pStrObj = dynamic_cast<Str*>(pObj);
								std::vector<std::string> li;
								pStrObj->SplitWithChars(delim, li);
								auto* pList = new List(li);
								pList->IncRef();
								XObj* pObjList = dynamic_cast<XObj*>(pList);
								retValue = X::Value(pObjList, false);
								return true;
							}));
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
			}
			void clean()
			{
				m_funcs.clear();
			}
			~StrOp()
			{
			}
			virtual Scope* GetParentScope()
			{
				return nullptr;
			}
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v)
			{
				return false;
			}

			inline virtual bool Get(XlangRuntime* rt, XObj* pContext,
				int idx, X::Value& v, X::LValue* lValue = nullptr)
			{
				Str* pStrObj = dynamic_cast<Str*>(pContext);
				v = m_funcs[idx];
				return true;
			}
		};

		static StrOp _strop;
		void Str::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(&_strop);
		}
		void Str::cleanup()
		{
			_strop.clean();
		}
		bool Str::Iterate(X::XRuntime* rt, XObj* pContext,
			IterateProc proc, ARGS& params, KWARGS& kwParams,
			X::Value& retValue)
		{
			ConstExpr* pFilter = dynamic_cast<ConstExpr*>(params[0].GetObj());
			ConstExpr* pAction = dynamic_cast<ConstExpr*>(params[1].GetObj());
			size_t size = m_s.size();
			for (size_t i = 0; i < size; i++)
			{
				bool bEnable = false;
				pFilter->Run(this, i, bEnable);
				if (bEnable)
				{
					pAction->Run(this, i, bEnable);
				}
			}
			return true;
		}
	}
}