#include "httplib.h"
#include "ipc.h"
#include "gthread.h"
#include "Locker.h"
#include "wait.h"
#include "def.h"
#include "port.h"
#include "utility.h"

namespace X { namespace Dev {
#define DevOps_Pipe_Name "\\\\.\\pipe\\x.devops"

struct cmd_info
{
    std::string data;
    std::string retData;
    XWait* wait = nullptr;
};

struct XNodeInfo
{
    std::string Name;
};
struct SessionInfo
{
    std::string id;
    IpcSession* ToolSide=nullptr;//like vs code
    XNodeInfo* xNodeInfo = nullptr;
    std::vector<cmd_info*> m_cmds;
};
class InProcServer
{
    IpcServer m_ipcSrv;
    Locker m_sessionLock;
    std::unordered_map<std::string, SessionInfo*> m_SessionMap;
public:
    InProcServer():
        m_ipcSrv(DevOps_Pipe_Name, this,
        [](void* pContext, IpcSession* newSession)
        {
            ((InProcServer*)pContext)->OnNewSession(newSession);
        })
    {
    }
    void OnNewSession(IpcSession* newSession)
    {
        newSession->SetDataHandler(this,
            [](void* pContext, IpcSession* pSession,
                char* data, int size) {
                    ((InProcServer*)pContext)->OnData(pSession, data, size);
            });
        std::string id = FindIdleToolSession(newSession);
        newSession->SetId(id);
    }
    void RunCmd(IpcSession* pSession, std::string cmd)
    {
        std::string ack;
        if (cmd.starts_with("enumNodes"))
        {
            ack = "[\"New Local\",\
                \"Attach Local:101\",\
                \"Attach Local:102\",\
                \"Attach Local:103\"\
                ]";
        }
        pSession->Send((char*)ack.c_str(), (int)ack.size());
    }
    void OnData(IpcSession* pSession, char* data, int size)
    {
        std::string id = pSession->GetId();
        std::cout << data << std::endl;
        std::string cmdData(data, size);
        if (cmdData.starts_with("$cmd:"))
        {
            RunCmd(pSession,cmdData.substr(5));
            return;
        }
        cmd_info* cmd = new cmd_info();
        cmd->data = cmdData;
        cmd->wait = new XWait();
        m_sessionLock.Lock();
        auto it = m_SessionMap.find(id);
        if (it != m_SessionMap.end())
        {
            it->second->m_cmds.push_back(cmd);
        }
        else
        {//todo...

        }
        m_sessionLock.Unlock();
        cmd->wait->Wait(-1);
        std::string ack = cmd->retData;
        delete cmd->wait;
        RemoveCmd(id,cmd);
        delete cmd;
        pSession->Send((char*)ack.c_str(), (int)ack.size());
        std::cout << "back:" << ack<< std::endl;
    }
    bool Start()
    {
        m_ipcSrv.SetBufferSize(1024 * 32);
        m_ipcSrv.Start();
        return true;
    }
    void Stop()
    {
        m_ipcSrv.Stop();
    }
    void RemoveCmd(std::string& sessionId,cmd_info* pCmd)
    {
        m_sessionLock.Lock();
        auto it0 = m_SessionMap.find(sessionId);
        if (it0 != m_SessionMap.end())
        {
            auto* sInfo = it0->second;
            auto it = sInfo->m_cmds.begin();
            while (it != sInfo->m_cmds.end())
            {
                if (*it == pCmd)
                {
                    sInfo->m_cmds.erase(it);
                    break;
                }
                else
                {
                    ++it;
                }
            }
        }
        m_sessionLock.Unlock();
    }
    bool SendSessionNotifyInfo(std::string& sessionId, std::string& data)
    {
        m_sessionLock.Lock();
        auto it = m_SessionMap.find(sessionId);
        if (it != m_SessionMap.end())
        {
            SessionInfo* sInfo = it->second;
            if (sInfo && sInfo->ToolSide)
            {
                if (data == "end")
                {
                    sInfo->ToolSide->Close();
                    sInfo->ToolSide = nullptr;//todo...
                }
                else
                {
                    sInfo->ToolSide->Send((char*)data.c_str(),(int)data.size());
                }
            }
        }
        m_sessionLock.Unlock();
        return true;
    }
    cmd_info* GetCmd(std::string sessionId)
    {
        cmd_info* cmd = nullptr;
        m_sessionLock.Lock();
        auto it = m_SessionMap.find(sessionId);
        if (it != m_SessionMap.end())
        {
            SessionInfo* sInfo = it->second;
            if (sInfo->m_cmds.size() > 0)
            {
                cmd = sInfo->m_cmds[0];
            }
        }
        m_sessionLock.Unlock();
        return cmd;
    }
    std::string FindIdleToolSession(IpcSession* pSession)
    {
        std::string id;
        m_sessionLock.Lock();
        bool bMatched = false;
        for (auto it : m_SessionMap)
        {
            if (it.second->ToolSide == nullptr)
            {
                it.second->ToolSide = pSession;
                id = it.first;
                bMatched = true;
                break;
            }
        }
        if (!bMatched)
        {
            const int online_len = 1000;
            char strBuf[online_len];
            SPRINTF(strBuf, online_len, "%llu", (unsigned long long)rand64());
            id = strBuf;
            m_SessionMap.emplace(std::make_pair(id,
                new SessionInfo{
                .id = id,
                .ToolSide = pSession
                }));
        }
        m_sessionLock.Unlock();
        return id;
    }
    std::string ClientRegister(std::string& name)
    {
        std::string id;
        XNodeInfo* pNode = new XNodeInfo;
        pNode->Name = name;
        m_sessionLock.Lock();
        bool bMatched = false;
        for (auto it : m_SessionMap)
        {
            if (it.second->xNodeInfo == nullptr)
            {
                it.second->xNodeInfo = pNode;
                id = it.first;
                bMatched = true;
                break;
            }
        }
        if (!bMatched)
        {
            const int online_len = 1000;
            char strBuf[online_len];
            SPRINTF(strBuf, online_len, "%llu", (unsigned long long)rand64());
            id = strBuf;
            m_SessionMap.emplace(std::make_pair(id,
                new SessionInfo{
                .id = id,
                .xNodeInfo = pNode
                }));
        }
        m_sessionLock.Unlock();
        return id;
    }
};
}
}

void MainLoop()
{
    X::Dev::InProcServer* inProcSrv = new X::Dev::InProcServer();
    inProcSrv->Start();

    httplib::Server* srv = new httplib::Server();
    //srv->set_keep_alive_max_count(1); // Default is 5
    //srv->set_keep_alive_timeout(100);  // Default is 5
    srv->Get("/register", [inProcSrv, srv](const httplib::Request& req,
        httplib::Response& res)
        {
            std::string nameInfo;
            std::string id = inProcSrv->ClientRegister(nameInfo);
            res.set_content(id, "text/plain");
        });
    srv->Get("/get_cmd", [inProcSrv,srv](const httplib::Request& req,
        httplib::Response& res)
        {
            std::string id = req.get_header_value("sessionId");
            auto cmd_info = inProcSrv->GetCmd(id);
            if (!cmd_info)
            {
                res.set_header("cmd_id", "");
                res.set_content("None", "text/plain");
                return;
            }
            const int online_len = 1000;
            char strBuf[online_len];
            SPRINTF(strBuf, online_len, "%llu",(unsigned long long)(void*)cmd_info);
            res.set_header("cmd_id", strBuf);
            res.set_content(cmd_info->data, "text/plain");
        });
    srv->Post("/send_ack", [inProcSrv,srv](const httplib::Request& req,
        httplib::Response& res)
        {
            std::string sessionId = req.get_header_value("sessionId");
            if (!req.has_header("cmd_id"))
            {
                std::string notiInfo = req.body;
                inProcSrv->SendSessionNotifyInfo(sessionId, notiInfo);
            }
            else
            {
                auto cmdId = req.get_header_value("cmd_id");
                unsigned long long cmd_id = 0;
                SCANF(cmdId.c_str(), "%llu", &cmd_id);
                auto* pCmdInfo = (X::Dev::cmd_info*)cmd_id;
                if (pCmdInfo)
                {
                    pCmdInfo->retData = req.body;
                    pCmdInfo->wait->Release();
                }
                else
                {//todo...

                }
            }
            res.set_content("OK", "text/plain");
        });
    srv->listen("0.0.0.0", 3141);
    delete srv;
    inProcSrv->Stop();
    delete inProcSrv;
}