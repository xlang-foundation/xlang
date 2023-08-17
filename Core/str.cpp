#include "str.h"
#include "scope.h"
#include "list.h"
#include "function.h"
#include <string>
#include <regex>
#include <sstream>
#include <iterator>
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
					{"splitWithChars",5},
					{"toupper",6},
					{"tolower",7},
					{"regex_replace",8},
				};
				{
					std::string name("find");
					auto f = [](X::XRuntime* rt,XObj* pThis, XObj* pContext,
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
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
							"pos = find(search_string)",func);
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("rfind");
					auto f = [](X::XRuntime* rt,XObj* pThis,XObj* pContext,
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
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"pos = rfind(search_string)",func);
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("slice");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
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
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"newStr = var_str.slice(startPos[,endPos])",func);
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("size");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						size_t  size = pStrObj->GetSize();
						retValue = X::Value((long long)size);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"size()",func);
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("split");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
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
						retValue = X::Value(pObjList, false);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"item_list = var_str.split(delimiter_str)",func);
					auto* pFuncObj = new Data::Function(extFunc,true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("splitWithChars");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
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
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"item_list = var_str.splitWithChars(delimiter_chars)",func);
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("toupper");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						auto str_abi = pStrObj->ToString();
						std::string strVal(str_abi);
						g_pXHost->ReleaseString(str_abi);
						std::transform(strVal.begin(),
							strVal.end(), strVal.begin(),
							[](unsigned char c) { return std::toupper(c); });
						auto* pNewStr = new Str(strVal);
						retValue = X::Value(pNewStr);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"new_str = toupper()",func);
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("tolower");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string strVal = pStrObj->ToString();
						std::transform(strVal.begin(),
							strVal.end(), strVal.begin(),
							[](unsigned char c) { return std::tolower(c); });
						auto* pNewStr = new Str(strVal);
						retValue = X::Value(pNewStr);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"new_str = tolower()",func);
					auto* pFuncObj = new Data::Function(extFunc, true);
					m_funcs.push_back(X::Value(pFuncObj));
				}
				{
					std::string name("regex_replace");
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						ARGS& params,
						KWARGS& kwParams,
						X::Value& retValue)
					{
						auto* pObj = dynamic_cast<Object*>(pContext);
						auto* pStrObj = dynamic_cast<Str*>(pObj);
						std::string pattern = params[0].ToString();
						const std::regex r(pattern);
						std::string target = params[1].ToString();
						auto str_abi = pStrObj->ToString();
						std::string org_str = str_abi;
						g_pXHost->ReleaseString(str_abi);
						std::stringstream result;
						std::regex_replace(std::ostream_iterator<char>(result),
							org_str.begin(), org_str.end(), r, target);
						auto* pNewStr = new Str(result.str());
						retValue = X::Value(pNewStr);
						return true;
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(name,
						"new_str = regex_replace(regex_expr,target_chars)",func);
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

		static StrOp* _strop =nullptr;
		inline void Str::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(_strop);
		}
		void Str::Init()
		{
			_strop = new StrOp();
		}
		void Str::cleanup()
		{
			if (_strop)
			{
				_strop->clean();
				delete _strop;
				_strop = nullptr;
			}
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