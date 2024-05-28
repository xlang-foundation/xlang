#include "msgthread.h"
#include "service_def.h"
#include "StubMgr.h"
#include "utility.h"
#include "manager.h"

#if (WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

#include <vector>
#include <iostream>
#include "RemoteObjectStub.h"

namespace X
{
#if (WIN32)
    BOOL ReadSlot(HANDLE hSlot, std::vector<pas_mesg_buffer>& msgs)
    {
        DWORD cbMessage, cMessage, cbRead;
        BOOL fResult;
        HANDLE hEvent;
        OVERLAPPED ov;

        cbMessage = cMessage = cbRead = 0;

        hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("EvtSlot"));
        if (NULL == hEvent)
        {
            return FALSE;
        }
        ov.Offset = 0;
        ov.OffsetHigh = 0;
        ov.hEvent = hEvent;

        fResult = GetMailslotInfo(hSlot, // mailslot handle 
            (LPDWORD)NULL,               // no maximum message size 
            &cbMessage,                   // size of next message 
            &cMessage,                    // number of messages 
            (LPDWORD)NULL);              // no read time-out 

        if (!fResult)
        {
            CloseHandle(hEvent);
            return FALSE;
        }

        if (cbMessage == MAILSLOT_NO_MESSAGE)
        {
            CloseHandle(hEvent);
            return TRUE;
        }


        while (cMessage != 0)  // retrieve all messages
        {
            char* buffer = new char[cbMessage];
            fResult = ReadFile(hSlot,
                buffer,
                cbMessage,
                &cbRead,
                &ov);

            if (!fResult)
            {
                CloseHandle(hEvent);
                delete[] buffer;
                return FALSE;
            }
            if (cbMessage >= sizeof(pas_mesg_buffer))
            {
                pas_mesg_buffer msg = *(pas_mesg_buffer*)buffer;
                msgs.push_back(msg);
            }
            delete[] buffer;
            fResult = GetMailslotInfo(hSlot,  // mailslot handle 
                (LPDWORD)NULL,               // no maximum message size 
                &cbMessage,                   // size of next message 
                &cMessage,                    // number of messages 
                (LPDWORD)NULL);              // no read time-out 

            if (!fResult)
            {
                return FALSE;
            }
        }
        CloseHandle(hEvent);
        return TRUE;
    }
#endif
    MsgThread::MsgThread()
    {
        RemoteObjectStub::I().Register();
    }
    void MsgThread::Stop()
    {
        mRun = false;
#if !(WIN32)
        RemoveMsgId();
#endif
        GThread::Stop();
    }

    void MsgThread::run()
    {
#if (WIN32)
        std::string msgKey(PAS_MSG_KEY);
        if (mPort != 0)
        {
            msgKey += tostring(mPort);
        }

        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);  // Grant access to everyone
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        HANDLE hSlot = CreateMailslot(
            msgKey.c_str(),
            0,
            MAILSLOT_WAIT_FOREVER,
            &sa);
        while (mRun)
        {
            std::vector<pas_mesg_buffer> msgs;
            if (ReadSlot(hSlot, msgs) && msgs.size() > 0)
            {
                for (auto m : msgs)
                {
                    if (m.mesg_type == (unsigned long long)PAS_MSG_TYPE::CreateSharedMem)
                    {
                        std::cout << "MsgThread,Get Message to Create Stub with key:"
                            << m.shmKey << std::endl;
                        RemotingManager::I().CreateStub(m.shmKey);
                    }
                }
            }
            else
            {
                Sleep(1);
            }
        }
        CloseHandle(hSlot);
#elif __ANDROID__

#else
        key_t key = (mPort ==0)?PAS_MSG_KEY:mPort;
        int msgid;
        msgid = msgget(key, 0666 | IPC_CREAT);
        mMsgLock.Lock();
        mMsgId = msgid;
        mMsgLock.Unlock();
        pas_mesg_buffer message;
        printf("Start MsgLoop\n");
        while (mRun)
        {
            // msgrcv to receive message
            //block call, canceled by RemoveMsgId
            auto size = msgrcv(msgid, &message, sizeof(message), 0, 0);
            if (size > 0)
            {
                if (message.mesg_type ==
                    (unsigned long long)PAS_MSG_TYPE::CreateSharedMem)
                {
                    std::cout << "MsgThread,Get Message to Create Stub with key:"
                        << message.shmKey << std::endl;
                    RemotingManager::I().CreateStub(message.shmKey);
                }
            }
            else
            {
                usleep(1000);//means connection will delay one ms
            }
        }
        RemoveMsgId();
        std::cout << "Exit MsgLoop" << std::endl;
#endif
        if (mPort != 0)
        {
            Manager::I().RemoveLrpcPort(mPort);
        }
    }

    void MsgThread::RemoveMsgId()
    {
#if (WIN32)

#elif __ANDROID__

#else
        mMsgLock.Lock();
        if (mMsgId != 0)
        {
            msgctl(mMsgId, IPC_RMID, NULL);
            mMsgId = 0;
        }
        mMsgLock.Unlock();
#endif
    }
}