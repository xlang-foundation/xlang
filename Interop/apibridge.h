/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include "xpackage.h"
#include "xlang.h"
#include <unordered_map>

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
			//from object's id to api bridge
			static std::unordered_map<void*, ApiBridge*> mMapApiBridge;
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
			void AddMap(void* pInstance)
			{
				mMapApiBridge.emplace(std::make_pair(pInstance, this));
			}
			bool FireObjectEvent(int evtId,X::Value* variantArray, int arrayLength)
			{
				X::ARGS args(arrayLength);
				X::KWARGS kwArgs;
				for (int i = 0; i < arrayLength; i++)
				{
					args.push_back(variantArray[i]);
				}
				mApis.Fire(__pPack_, evtId, args, kwArgs);
				return true;
			}
			static ApiBridge* GetApiBridge(void* pInstance)
			{
				auto it = mMapApiBridge.find(pInstance);
				if (it != mMapApiBridge.end())
				{
					return it->second;
				}
				return nullptr;
			}
			X::XPackage* CreatePackProxy()
			{
				auto* pInstance = g_createOrGetClassInstanceDelegate(mCreateFuncOrInstance);
				AddMap(pInstance);
				auto* pPackProxy = g_pXHost->CreatePackageProxy(__pPack_, pInstance);
				return pPackProxy;
			}
			void SetCreateFuncOrInstance(bool singleInstance, void* createFuncOrInstance)
			{
				mSingleInstance = singleInstance;
				mCreateFuncOrInstance = createFuncOrInstance;
			}
			bool RegisterApi(X::PackageMemberType type,const char* evtName)
			{
				int memberId = -1;
				switch (type)
				{
				case X::PackageMemberType::Func:
					memberId = mApis.AllocSlot();
					mApis.SetDirectFunc(memberId, evtName, [this, memberId](
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
					mApis.AddEvent(evtName);
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
