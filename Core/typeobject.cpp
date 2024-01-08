#include "typeobject.h"
#include "function.h"
#include "package.h"
#include "scope.h"
#include "xclass.h"
#include "xclass_object.h"

namespace X
{
	namespace Data
	{
		class TypeObjectScope :
			virtual public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			TypeObjectScope() :
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
			~TypeObjectScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::StackFrame();
				m_stackFrame->SetVarCount(3);
				std::string strName;
				{
					strName = "getMembers";
					auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
						X::ARGS& params,
						X::KWARGS& kwParams,
						X::Value& retValue)
					{
						auto* pObj = dynamic_cast<TypeObject*>(pContext);
						retValue = pObj ? pObj->GetMembers(dynamic_cast<XlangRuntime*>(rt)) : X::Value();
						return (pObj!=nullptr);
					};
					X::U_FUNC func(f);
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName, 
						"getMembers() returns a list of members", func);
					auto* pFuncObj = new X::Data::Function(extFunc);
					pFuncObj->IncRef();
					int idx = Scope::AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
			}
		};
		static TypeObjectScope* _TypeObjectScope = nullptr;
		void TypeObject::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			bases.push_back(_TypeObjectScope);
		}
		void TypeObject::Init()
		{
			_TypeObjectScope = new TypeObjectScope();
		}
		void TypeObject::cleanup()
		{
			if (_TypeObjectScope)
			{
				_TypeObjectScope->clean();
				delete _TypeObjectScope;
				_TypeObjectScope = nullptr;
			}
		}
		X::Value Data::TypeObject::GetMembers(X::XlangRuntime* rt)
		{
			if (!m_realObj.IsObject())
			{
				return X::Value();
			}
			auto* pObj = dynamic_cast<Object*>(m_realObj.GetObj());
			//if pObj is Package or PackageProxy will use its m_memberInfos
			//to make a list of members;
			//if pObj is a XClassObject, will query its scope's vars,
			//and also use its stackframe to decide member's type
			//else will use its scope's vars to make a list of members
			//but members type will be unknown
			X::List list;
			if (pObj->GetType() == X::ObjType::Package)
			{
				X::AST::Package* pPack = nullptr;
				X::AST::PackageProxy* pPackProxy = dynamic_cast<X::AST::PackageProxy*>(pObj);
				if (pPackProxy)
				{
					pPack = pPackProxy->GetPackage();
				}
				else
				{
					pPack = dynamic_cast<X::AST::Package*>(pObj);
				}
				if (pPack == nullptr)
				{
					return X::Value();
				}
				auto& members = pPack->GetMemberInfo();
				auto typeToStr = [](PackageMemberType type)
				{
					switch (type)
					{
					case X::PackageMemberType::Func:
					case X::PackageMemberType::FuncEx:
						return "Function";
					case X::PackageMemberType::Prop:
						return "Property";
					case X::PackageMemberType::Const:
						return "Constant";
					case X::PackageMemberType::ObjectEvent:
						return "Event";
					case X::PackageMemberType::Class:
						return "Class";
					case X::PackageMemberType::ClassInstance:
						return "Singleton";
					default:
						break;
					}
					return "unknown";
				};
				for (auto& m : members)
				{
					X::Dict dict;
					dict->Set("name", m.name);
					dict->Set("doc", m.doc);
					dict->Set("type", typeToStr(m.type));
					list += dict;
				}
			}
			else if (pObj->GetType() == X::ObjType::XClassObject)
			{
				auto* pClassObj = dynamic_cast<XClassObject*>(pObj);
				AST::Scope* pClassScope = pClassObj->GetClassObj()->GetMyScope();
				auto vars = pClassScope->GetVarMap();
				for (auto& it : vars)
				{
					X::Dict dict;
					X::Value val;
					pClassScope->Get(rt, pObj, it.second, val);
					std::string name = it.first;
					dict->Set("Name", name);
					auto valType = val.GetValueType();
					dict->Set("Type",valType);
					list += dict;
				}
			}
			else
			{
				std::vector<AST::Scope*> bases;
				pObj->GetBaseScopes(bases);
			}
			return list;
		}
	}
}