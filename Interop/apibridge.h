#pragma once
#include "xpackage.h"
#include "xlang.h"

namespace X
{
	namespace Interop
	{
		enum class BridgeCallType
		{
			CreateClass,
			CallFunction,
		};
		typedef void* (*CreateOrGetClassInstanceDelegate)(void* createFuncOrInstance);
		typedef void (*InvokeMethodDelegate)(
			void* classInstance,        // Pointer to the object (class instance)
			const char* methodName,     // Method name as a C-style string
			X::Value* variantArray,      // Pointer to the first element of a Variant array
			int arrayLength,            // Length of the Variant array
			X::Value* outReturnValue     // Pointer to a Variant to store the return value
			);

		extern CreateOrGetClassInstanceDelegate g_createOrGetClassInstanceDelegate;
		extern InvokeMethodDelegate g_invokeMethodDelegate;
		class ApiBridge
		{
		public:
			bool mSingleInstance = false;
			void* mCreateFuncOrInstance = nullptr;
			X::XPackageAPISet<ApiBridge> mApis;
			X::XPackage* __pPack_ = nullptr;
			X::XPackageAPISet<ApiBridge>& APISET()
			{
				return mApis;
			}
		public:
			ApiBridge()
			{
			}
			~ApiBridge()
			{
			}
			bool CreatePackage()
			{
				return mApis.Create(this);
			}
			X::XPackage* CreatePackProxy()
			{
				auto* pInstance = g_createOrGetClassInstanceDelegate(mCreateFuncOrInstance);
				auto* pPackProxy = g_pXHost->CreatePackageProxy(__pPack_, pInstance);
				return pPackProxy;
			}
			void SetCreateFuncOrInstance(bool singleInstance, void* createFuncOrInstance)
			{
				mSingleInstance = singleInstance;
				mCreateFuncOrInstance = createFuncOrInstance;
			}
			bool RegisterApi(X::PackageMemberType type,const char* funcName)
			{
				int memberId = -1;
				switch (type)
				{
				case X::PackageMemberType::Func:
					memberId = mApis.AllocSlot();
					mApis.SetDirectFunc(memberId,funcName, [this, memberId](
						X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
						X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
						{
							auto& memberInfo = mApis.GetMember(memberId);
							if (pContext->GetType() == X::ObjType::Package)
							{
								XPackage* pPack = dynamic_cast<XPackage*>(pContext);
								void* thisObj = pPack->GetEmbedObj();
								g_invokeMethodDelegate(thisObj, memberInfo.name.c_str(),
									params.Data(), params.size(), &retValue);
							}
							return true;
						});
					break;
				case X::PackageMemberType::Prop:
					break;
				case X::PackageMemberType::Const:
					break;
				case X::PackageMemberType::ObjectEvent:
					break;
				case X::PackageMemberType::Class:
					break;
				case X::PackageMemberType::ClassInstance:
					break;
				default:
					break;
				}
				return true;
			}
		};
	}
}
