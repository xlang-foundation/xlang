#include "xhost_impl.h"
#include "xlang.h"
#include "str.h"
#include "manager.h"
#include "function.h"
#include "prop.h"
#include "package.h"
#include "dict.h"
#include "list.h"
#include "set.h"
#include "complex.h"
#include "bin.h"
#include "BlockStream.h"
#include "Hosting.h"
#include "event.h"
#include "remote_object.h"
#include "msgthread.h"
#include "import.h"
#include "expr_scope.h"
#include "RemoteObjectStub.h"
#include "tensor.h"
#include "tensorop.h"
#include "tensor_graph.h"

namespace X 
{
	X::XHost* g_pXHost = nullptr;
	X::XHost* CreatXHost()
	{
		g_pXHost = new XHost_Impl();
		return g_pXHost;
	}
	void DestoryXHost()
	{
		if (g_pXHost)
		{
			delete g_pXHost;
		}
	}
	void XHost_Impl::AddSysCleanupFunc(CLEANUP f)
	{
		Manager::I().AddCleanupFunc(f);
	}
	XRuntime* XHost_Impl::CreateRuntime()
	{
		XlangRuntime* rt = new XlangRuntime();
		G::I().BindRuntimeToThread(rt);
		return dynamic_cast<XRuntime*>(rt);
	}
	XRuntime* XHost_Impl::GetCurrentRuntime()
	{
		return G::I().GetCurrentRuntime();
	}
	XStr* XHost_Impl::CreateStr(const char* data, int size)
	{
		Data::Str* pStrObj = data== nullptr? new Data::Str(size):new Data::Str(data, size);
		pStrObj->IncRef();
		return pStrObj;
	}
	bool XHost_Impl::RegisterPackage(const char* name,PackageCreator creator)
	{
		return X::Manager::I().Register(name,creator);
	}
	bool XHost_Impl::RegisterPackage(const char* name,Value& objPackage)
	{
		return X::Manager::I().Register(name,objPackage);
	}
	Value XHost_Impl::QueryMember(XRuntime* rt, XObj* pObj, const char* name)
	{
		Data::Object* pRealObj = dynamic_cast<Data::Object*>(pObj);
		if (pRealObj == nullptr)
		{
			return X::Value();
		}
		X::Value retValue;
		AST::Scope* pScope = dynamic_cast<AST::Scope*>(pRealObj);
		if (pScope)
		{
			std::string strName(name);
			int idx = pScope->AddOrGet(strName, true);
			if (idx >= 0)
			{
				pScope->Get((XlangRuntime*)rt, pRealObj, idx, retValue);
			}
		}
		return retValue;
	}
	bool XHost_Impl::QueryPackage(XRuntime* rt, const char* name, Value& objPackage)
	{
		std::string strName(name);
		return X::Manager::I().QueryAndCreatePackage((XlangRuntime*)rt,strName, objPackage);
	}
	Value XHost_Impl::CreatePackageWithUri(const char* packageUri)
	{
		X::Value varPackage;
		std::string uri(packageUri);
		auto parts = split(uri, '|');
		if (parts.size() < 2)
		{
			return varPackage;
		}

		if (g_pXHost->Import(GetCurrentRuntime(), parts[1].c_str(), parts[0].c_str(), nullptr, varPackage))
		{
			for (int i = 2; i < parts.size(); i++)
			{
				varPackage = varPackage[parts[i].c_str()]();
			}
		}
		return varPackage;
	}
	XPackage* XHost_Impl::CreatePackage(void* pRealObj)
	{
		auto* pPack = new AST::Package(pRealObj);
		pPack->Scope::IncRef();
		return dynamic_cast<XPackage*>(pPack);
	}
	XPackage* XHost_Impl::CreatePackageProxy(XPackage* pPackage, void* pRealObj)
	{
		auto* pPack = new AST::PackageProxy(dynamic_cast<AST::Package*>(pPackage),pRealObj);
		pPack->Scope::IncRef();
		return dynamic_cast<XPackage*>(pPack);
	}
	XEvent* XHost_Impl::CreateXEvent(const char* name)
	{
		std::string strName(name);
		auto* pEvt = new X::ObjectEvent(strName);
		pEvt->IncRef();
		return dynamic_cast<XEvent*>(pEvt);
	}
	XFunc* XHost_Impl::CreateFunction(const char* name,U_FUNC& func, X::XObj* pContext)
	{
		std::string strName(name);
		AST::ExternFunc* extFunc = new AST::ExternFunc(strName,"",func, pContext);
		auto* pFuncObj = new X::Data::Function(extFunc,true);
		pFuncObj->IncRef();
		return dynamic_cast<XFunc*>(pFuncObj);
	}
	XFunc* XHost_Impl::CreateFunctionEx(const char* name, U_FUNC_EX& func, X::XObj* pContext)
	{
		std::string strName(name);
		AST::ExternFunc* extFunc = new AST::ExternFunc(strName, func, pContext);
		auto* pFuncObj = new X::Data::Function(extFunc,true);
		pFuncObj->IncRef();
		return dynamic_cast<XFunc*>(pFuncObj);
	}
	XProp* XHost_Impl::CreateProp(const char* name, U_FUNC& setter, U_FUNC& getter)
	{
		std::string strName(name);
		std::string strSetName = "set_" + strName;
		std::string strGetName = "get_" + strName;

		AST::ExternFunc* extFunc_set = new AST::ExternFunc(strSetName,"", setter);
		AST::ExternFunc* extFunc_get = new AST::ExternFunc(strGetName,"", getter);
		auto* pPropObj = new X::Data::PropObject(extFunc_set, extFunc_get);
		pPropObj->IncRef();
		return dynamic_cast<XProp*>(pPropObj);
	}
	/*   TODO:
		some pointer convert to Data::Object and pass to Value
		have to use this way to get right Object

	*/
	XObj* XHost_Impl::ConvertObjFromPointer(void* pObjectPtr)
	{
		return (XObj*)(X::Data::Object*)(pObjectPtr);
	}
	XList* XHost_Impl::CreateList()
	{
		auto* pList = new X::Data::List();
		pList->IncRef();
		return pList;
	}
	XTensor* XHost_Impl::CreateTensor()
	{
		auto* pTensor = new X::Data::Tensor();
		pTensor->IncRef();
		return pTensor;
	}
	XTensorOperator* XHost_Impl::CreateTensorOperator(Tensor_OperatorHandler op, bool isUnaryOp)
	{
		auto* pTensor = new X::Data::TensorOperator(op, isUnaryOp);
		pTensor->IncRef();
		return pTensor;
	}
	XTensorGraph* XHost_Impl::CreateTensorGraph()
	{
		auto* pTensorGraph = new X::Data::TensorGraph();
		pTensorGraph->IncRef();
		return pTensorGraph;
	}
	
	XDict* XHost_Impl::CreateDict()
	{
		auto* pDict =  new X::Data::Dict();
		pDict->IncRef();
		return pDict;
	}
	XSet* XHost_Impl::CreateSet()
	{
		auto* pSet =  new X::Data::mSet();
		pSet->IncRef();
		return pSet;
	}
	XComplex* XHost_Impl::CreateComplex()
	{
		auto* pComplex =  new X::Data::Complex();
		pComplex->IncRef();
		return pComplex;
	}
	const char* XHost_Impl::StringifyString(const char* str)
	{
		std::string outStr =  ::StringifyString(str);
		auto len = outStr.size() + 1;
		char* pNewStr = new char[len];//must use ReleaseString to delete it
		memcpy(pNewStr, outStr.data(), len);
		return (const char*)pNewStr;
	}
	void XHost_Impl::ReleaseString(const char* str)
	{
		delete str;
	}
	XBin* XHost_Impl::CreateBin(char* data, size_t size, bool bOwnData)
	{
		auto* pObjBin = new Data::Binary(data, size, bOwnData);
		pObjBin->IncRef();
		return pObjBin;
	}
	X::XLStream* XHost_Impl::CreateStream(const char* buf, long long size)
	{
		X::BlockStream* pStream = nullptr;
		if (buf == nullptr && size == 0)
		{
			pStream = new X::BlockStream();
		}
		else
		{
			pStream = new X::BlockStream((char*)buf,size,false);
		}
		return dynamic_cast<X::XLStream*>(pStream);
	}
	void XHost_Impl::ReleaseStream(X::XLStream* pStream)
	{
		auto* pBlockStream = dynamic_cast<X::BlockStream*>(pStream);
		delete pBlockStream;
	}
	XRemoteObject* XHost_Impl::CreateRemoteObject(XProxy* proxy)
	{
		auto* pRObj = new RemoteObject(proxy);
		pRObj->Object::IncRef();
		return pRObj;
	}
	bool XHost_Impl::ToBytes(X::Value& input, X::Value& output)
	{
		X::BlockStream stream;
		stream << input;
		auto size = stream.Size();
		char* pData = new char[size];
		stream.FullCopyTo(pData, size);
		X::Data::Binary* pBinOut = new X::Data::Binary(pData, size,true);
		output = X::Value(pBinOut);
		return true;
	}
	bool XHost_Impl::FromBytes(X::Value& input, X::Value& output)
	{
		if (!input.IsObject() || input.GetObj()->GetType() != ObjType::Binary)
		{
			return false;
		}
		Data::Binary* pBin = dynamic_cast<Data::Binary*>(input.GetObj());
		X::BlockStream stream(pBin->Data(), pBin->Size(),false);
		stream.ScopeSpace().SetContext((XlangRuntime*)pBin->RT(), pBin->Parent());
		stream >> output;
		return true;
	}
	bool XHost_Impl::ConvertToBytes(X::Value& v, X::XLStream* pStreamExternal)
	{
		X::XLangStream* pStream = nullptr;
		if (pStreamExternal == nullptr)
		{
			pStream = new X::BlockStream();
		}
		else
		{
			pStream = new XLangStream();
			pStream->SetProvider(pStreamExternal);
		}
		(*pStream) << v;
		if (pStream)
		{
			delete pStream;
		}
		return true;
	}
	bool XHost_Impl::ConvertFromBytes(X::Value& v, X::XLStream* pStreamExternal)
	{
		X::XLangStream stream;
		stream.SetProvider(pStreamExternal);
		stream >> v;
		return true;
	}
	bool XHost_Impl::WriteToStream(char* data, long long size, X::XLStream* pStream)
	{
		X::XLangStream stream;
		stream.SetProvider(pStream);
		return stream.append(data,size);
	}
	bool XHost_Impl::ReadFromStream(char* buffer, long long size, X::XLStream* pStream)
	{
		X::XLangStream stream;
		stream.SetProvider(pStream);
		return stream.CopyTo(buffer, size);
	}
	bool XHost_Impl::RunCode(const char* moduleName, const char* code, int codeSize,X::Value& retVal)
	{
		std::vector<X::Value> passInParams;
		return X::Hosting::I().Run(moduleName, code,
			codeSize, passInParams,retVal);
	}
	bool XHost_Impl::RunModuleInThread(const char* moduleName, 
		const char* code, int codeSize, X::ARGS& args, X::KWARGS& kwargs)
	{
		std::string strModuleName(moduleName);
		std::string strCode(code, codeSize);
		std::vector<X::Value> passinParams;
		for (auto& a : args)
		{
			passinParams.push_back(a);
		}
		return X::Hosting::I().RunAsBackend(strModuleName, strCode, passinParams);
	}
	bool XHost_Impl::RunCodeLine(const char* codeLine,int codeSize,X::Value& retVal)
	{
		return X::Hosting::I().RunCodeLine(codeLine,codeSize, retVal);
	}
	const char* XHost_Impl::GetInteractiveCode()
	{
		std::string code;
		X::Hosting::I().GetInteractiveCode(code);
		auto len = code.size() + 1;
		char* pNewStr = new char[len];//must use ReleaseString to delete it
		memcpy(pNewStr, code.data(), len);
		return (const char*)pNewStr;
	}

	long XHost_Impl::OnEvent(const char* evtName, EventHandler handler)
	{
		auto* pEvt = X::EventSystem::I().Query(evtName);
		if (pEvt == nullptr)
		{//Create it
			pEvt = X::EventSystem::I().Register(evtName);
		}
		return pEvt->Add(handler);
	}
	void XHost_Impl::OffEvent(const char* evtName, long Cookie)
	{
		auto* pEvt = X::EventSystem::I().Query(evtName);
		if (pEvt)
		{
			pEvt->Remove(Cookie);
		}
	}
	Value XHost_Impl::GetAttr(const X::Value& v, const char* attrName)
	{
		Value retVal;
		if (v.IsObject())
		{
			Data::Object* pObj = dynamic_cast<Data::Object*>((XObj*)v);
			if (pObj)
			{
				auto* pBag = pObj->GetAttrBag();
				if (pBag)
				{
					retVal = pBag->Get(attrName);
				}
			}
		}
		return retVal;
	}
	void XHost_Impl::SetAttr(const X::Value& v, const char* attrName, X::Value& attrVal)
	{
		if (v.IsObject())
		{
			Data::Object* pObj = dynamic_cast<Data::Object*>((XObj*)v);
			if (pObj)
			{
				auto* pBag = pObj->GetAttrBag();
				if (pBag)
				{
					pBag->Set(attrName,attrVal);
				}
			}
		}
	}
	AppEventCode XHost_Impl::HandleAppEvent(int signum)
	{
		return Hosting::I().HandleAppEvent(signum);
	}
	bool XHost_Impl::Lrpc_Listen(int port, bool blockMode)
	{
		bool bOK = true;
		Manager::I().AddLrpcPort(port);
		MsgThread::I().SetPort(port);
		if (blockMode)
		{
			MsgThread::I().run();
			Manager::I().RemoveLrpcPort(port);
		}
		else
		{
			bOK = MsgThread::I().Start();
		}
		return bOK;
	}
	bool XHost_Impl::Import(XRuntime* rt, const char* moduleName, 
		const char* from, const char* thru, X::Value& objPackage)
	{
		AST::Import* pImp = new AST::Import(moduleName, from, thru);
		//todo: CHECK here if pImp will be released by out of scope
		AST::ExecAction action;
		bool bOK = pImp->Exec((XlangRuntime*)rt,action,nullptr, objPackage);
		if (objPackage.IsObject())
		{
			objPackage.GetObj()->SetContext(rt, nullptr);
		}
		return bOK;
	}
	bool XHost_Impl::CreateScopeWrapper(XCustomScope* pScope)
	{
		Data::ExpressionScope* pExprScope = new Data::ExpressionScope(pScope);
		pScope->SetScope((void*)pExprScope);
		return true;
	}
	bool XHost_Impl::DeleteScopeWrapper(XCustomScope* pScope)
	{
		Data::ExpressionScope* pExprScope = (Data::ExpressionScope*)pScope->GetScope();
		if (pExprScope)
		{
			delete pExprScope;
		}
		return true;
	}
	bool XHost_Impl::SetExpressionScope(XCustomScope* pScope, X::Value& expr)
	{
		if (!expr.IsObject())
		{
			return false;
		}
		Data::Expr* pExprObj = dynamic_cast<Data::Expr*>(expr.GetObj());
		if (pExprObj == nullptr)
		{
			return false;
		}
		AST::Expression* pExpr = pExprObj->Get();
		if (pExpr == nullptr)
		{
			return false;
		}
		Data::ExpressionScope* pExprScope = (Data::ExpressionScope*)pScope->GetScope();
		pExpr->SetScope(pExprScope);
		return true;
	}
	bool XHost_Impl::RunExpression(X::Value& expr, X::Value& result)
	{
		if (!expr.IsObject())
		{
			return false;
		}
		Data::Expr* pExprObj = dynamic_cast<Data::Expr*>(expr.GetObj());
		if (pExprObj == nullptr)
		{
			return false;
		}
		AST::Expression* pExpr = pExprObj->Get();
		if (pExpr == nullptr)
		{
			return false;
		}
		AST::ExecAction action;
		bool bOK = pExpr->Exec(nullptr,action,nullptr, result);
		return bOK;
	}
	bool XHost_Impl::ExtractNativeObjectFromRemoteObject(X::Value& remoteObj,
		X::Value& nativeObj)
	{
		return RemoteObjectStub::I().ExtractNativeObjectFromRemoteObject(remoteObj, nativeObj);
	}
	void XHost_Impl::RegisterUIThreadRunHandler(UI_THREAD_RUN_HANDLER handler, void* pContext)
	{
		m_uiThreadRunHandler = handler;
		m_uiThreadRunContext = pContext;
	}
	UI_THREAD_RUN_HANDLER XHost_Impl::GetUIThreadRunHandler()
	{
		return m_uiThreadRunHandler;
	}
	void* XHost_Impl::GetUIThreadRunContext()
	{
		return m_uiThreadRunContext;
	}
}
