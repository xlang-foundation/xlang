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

#include <string>
#include "value.h"

namespace X
{
    enum class PAS_MSG_TYPE
    {
        CreateSharedMem = 1,
    };
    // structure for message queue
    struct pas_mesg_buffer
    {
        unsigned long long mesg_type;
        unsigned long long shmKey;
    };
#if (WIN32)
#define PAS_MSG_KEY TEXT("\\\\.\\mailslot\\galaxy_msgslot")
#else
#define PAS_MSG_KEY 0x12344
#endif

#define SM_BUF_SIZE 1024*1024*10

#define ROOT_OBJECT_ID 1

    typedef void* REMOTE_OBJECT_PTR;

    enum class RPC_CALL_TYPE
    {
        ShakeHands,
        CantorProxy_QueryRootObject,
        CantorProxy_QueryMember,
        CantorProxy_QueryMemberCount,
        CantorProxy_GetMemberObject,
        CantorProxy_ReleaseObject,
        CantorProxy_FlatPack,
        CantorProxy_UpdateItemValue,
        CantorProxy_Call,
        //--------------
        PushFrame,
        CreateDataView,
        DataViewPop,
        RemoveDataView,
        KVSet,
        KVGet,
        KVDelete,
        StartServer,
        ConnectToRemoteServer,
        RemoteImport,
        RemoteCall,
        RemoteObjectOp,
        SetWorkerStatus,
        SessionCleanup,
        QueryNodeId,
        RegisterTask,
        SubmitTask
    };

    enum class PayloadType:int
    {
        Send,//pass data to another side
        SendLast,//Last Payload
        Ack//another side acks
    };
    struct PayloadFrameHead
    {
        long long size;//total size
        PayloadType payloadType;
        unsigned int blockSize;//this block size
        unsigned int callIndex;//CallIndex for ack
        unsigned int callType;
        union
        {
            long long callReturnCode;
            void* context;
        };
    };

#define SHORT_WAIT_TIMEOUT 100000L
#define LONG_WAIT_TIMEOUT 1000000L

    typedef void* OBJECT_ID;

#if (WIN32)
#define SPRINTF sprintf_s
#else
#define SPRINTF snprintf
#endif

    enum class Cantor_DataType
    {
        DT_NULL,
        DT_LONG_LONG,
        DT_DOUBLE,
        DT_STRING
    };
    enum class KV_Action
    {
        Get,
        Set,
        Delete
    };

#if 0
    struct CantorVar
    {
        Cantor_DataType dt;
        int _align_ = 0;
        union
        {
            long long lval;
            double dVal;
        }v;
        std::string strVal;//strVal can't put into union because need to call destructor
    };
#else
    using CantorVar = X::Value;
#endif

    enum class WorkerStatus
    {
        None,
        Ready,
        Run,
        Stopped
    };

#define NODE_NAME_TAG "NodeName"
#define NODE_ID_TAG "NodeId"
}