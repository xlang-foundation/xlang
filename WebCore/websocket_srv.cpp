//to avoid winsock head file issue
//put all boost include ahead others
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "websocket_srv.h"
#include "gthread.h"
#include "Locker.h"
#include "wait.h"

#include <iostream>
#include <vector>
#include "value.h"
#include "xhost.h"


namespace X
{
    namespace WebCore
    {
        namespace asio = boost::asio;
        namespace beast = boost::beast;
        using tcp = asio::ip::tcp;

        class WebSocketSessionImpl;

        class WebSocketSrvThread:
            public GThread
        {
            int mPort = 8080;
            bool mRun = true;
            WebSocketServer* mServer = nullptr;
            Locker m_lockSessions;
            std::vector<WebSocketSessionImpl*> m_Sessions;
        protected:
            virtual void run() override;
        public:
            WebSocketServer* GetServer()
			{
				return mServer;
			}
            WebSocketSrvThread(WebSocketServer* pServer)
            {
                mServer = pServer;
            }
            void RemoveSession(WebSocketSessionImpl* pSession);
            virtual void Stop() override
            {
                mRun = false;
                GThread::Stop();
            }
            FORCE_INLINE void SetPort(int port)
            {
                mPort = port;
            }
        };
        struct WSCallInfo
        {
            int id;
            std::string funcUri;
            std::vector<X::Value> parameters;
        };
        class WebSocketSessionImpl :
            public GThread2
        {
            bool mRun = true;
            beast::websocket::stream<tcp::socket>* m_wsSocket = nullptr;
            WebSocketSrvThread* mServer = nullptr;
            WebSocketSession* mRealSession = nullptr;//will be deleted by mSocketSession release
            X::Value mSocketSession;//as xlang's Value
            XWait*  m_QueueWait = nullptr;
            Locker m_lockQueue;
            std::vector<X::Value> m_OutQueue;

            //there are two threads use this session
            //so last one exited should delete this session
            int m_threadCount = 0;
            Locker m_lockThreadCount;
            void TryToRemoveSession()
            {
                bool CanRemove = false;
                WebSocketSrvThread* pKeepVarForSrv = mServer;
                WebSocketSessionImpl* pThis = this;
                m_lockThreadCount.Lock();
                m_threadCount--;
                CanRemove == (m_threadCount == 0);
                m_lockThreadCount.Unlock();
                if (CanRemove)
                {
                    pKeepVarForSrv->RemoveSession(pThis);
                }
            }
        public:
            WebSocketSessionImpl(WebSocketSrvThread* pServer, tcp::socket* socket)
            {
                mServer = pServer;
                m_wsSocket = new beast::websocket::stream<tcp::socket>(std::move(*socket));
                m_wsSocket->accept();
                WebSocketSession* pXlangSession = new WebSocketSession();
                pXlangSession->SetImpl((void*)this);
                mSocketSession = X::Value(WebSocketSession::APISET().GetProxy(pXlangSession), false);
                mRealSession = pXlangSession;
                m_QueueWait = new XWait();
            }
            ~WebSocketSessionImpl()
			{
				delete m_QueueWait;
				m_QueueWait = nullptr;
                delete m_wsSocket;
                m_wsSocket = nullptr;
			}
            FORCE_INLINE bool Write(const X::Value& data)
			{
                m_lockQueue.Lock();
                m_OutQueue.push_back(data);
                m_lockQueue.Unlock();
                m_QueueWait->Release();
				return true;
			}
            X::Value GetSession()
			{
				return mSocketSession;
			}
            virtual void Stop() override
            {
                mRun = false;
                GThread::Stop();
            }
            void PrepareStop()
            {
                mRun = false;
                m_QueueWait->Release();
            }
  
        protected:
            //read thread
            virtual void run() override
            {
                m_lockThreadCount.Lock();
                m_threadCount++;
                m_lockThreadCount.Unlock();
                try
                {
                    while (mRun)
                    {
                        beast::flat_buffer buffer;
                        m_wsSocket->read(buffer);
                        std::size_t data_len = buffer.size();
                        const char* data = static_cast<const char*>(buffer.data().data());
                        auto* stream = X::g_pXHost->CreateStream(data, data_len);
                        X::Value recvData;
                        recvData.FromBytes(stream);
                        X::g_pXHost->ReleaseStream(stream);
                        buffer.clear();

                        X::ARGS args(2);
                        args.push_back(mSocketSession);
                        args.push_back(recvData);
                        X::KWARGS kwargs;
                        //Fire is sync call in xlang
                        //so still in this stack frame
                        mServer->GetServer()->Fire(1, args, kwargs);//OnSessionReceive

                    }
                }
                catch (const std::exception& e)
                {
                    //LOG << "Error:" << e.what() << LINE_END;
                }
                PrepareStop();
                TryToRemoveSession();
            }
            //write thread
            virtual void run2() override
            {
                m_lockThreadCount.Lock();
                m_threadCount++;
                m_lockThreadCount.Unlock();
                try
                {

                    while (mRun)
                    {
                        bool bHaveData = false;
                        beast::flat_buffer buffer;
                        m_lockQueue.Lock();
                        auto it = m_OutQueue.begin();
                        if(it != m_OutQueue.end())
                        {
                            X::Value& data = *it;
                            X::Value outBin;
                            X::g_pXHost->ToBytes(data, outBin);
                            X::Bin bin(outBin);
                            buffer.commit(boost::asio::buffer_copy(
                                buffer.prepare(bin.Size()),
                                boost::asio::buffer(bin->Data(), bin.Size())));
                            m_OutQueue.erase(it);
                            bHaveData = true;
                        }
                        m_lockQueue.Unlock();
                        if (bHaveData)
                        {
                            m_wsSocket->text(false);
                            m_wsSocket->write(buffer.data());
                        }
					    else
						{
                            m_QueueWait->Wait(100);
						}
                    }
                }
                catch (const std::exception& e)
                {
                    //LOG << "Error:" << e.what() << LINE_END;
                }
                PrepareStop();
                TryToRemoveSession();
            }
        };

        void WebSocketSrvThread::RemoveSession(WebSocketSessionImpl* pSession)
        {
            X::ARGS args(1);
            args.push_back(pSession->GetSession());
            X::KWARGS kwargs;
            mServer->Fire(2, args, kwargs);//OnRemoveSession

            m_lockSessions.Lock();
            auto it = m_Sessions.begin();
            while (it != m_Sessions.end())
            {
                if (*it == pSession)
                {
                    m_Sessions.erase(it);
                    break;
                }
                else
                {
                    it++;
                }
            }
            m_lockSessions.Unlock();
            delete pSession;
        }

        void WebSocketSrvThread::run()
        {
            asio::io_context io_context;
            tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), mPort));
            while (mRun)
            {
                tcp::socket socket(io_context);
                acceptor.accept(socket);

                auto* pSesison = new WebSocketSessionImpl(this, &socket);
                m_lockSessions.Lock();
                m_Sessions.push_back(pSesison);
                m_lockSessions.Unlock();
                pSesison->Start();
                X::ARGS args(1);
                args.push_back(pSesison->GetSession());
                X::KWARGS kwargs;
                mServer->Fire(0, args, kwargs);
            }
            m_lockSessions.Lock();
            for (auto* pSession : m_Sessions)
            {
                X::ARGS args(1);
                args.push_back(pSession->GetSession());
                X::KWARGS kwargs;
                mServer->Fire(2, args, kwargs);//OnRemoveSession
                m_lockSessions.Unlock();
                pSession->Stop();
                delete pSession;
                m_lockSessions.Lock();

            }
            m_Sessions.clear();
            m_lockSessions.Unlock();
        }

        //WebSocketServer Impl.

        WebSocketServer::WebSocketServer(int port)
        {
            WebSocketSrvThread* pSrvThread = new WebSocketSrvThread(this);
            pSrvThread->SetPort(port);
            bool bOK = pSrvThread->Start();
            if (bOK)
            {
                m_pImpl =(void*)pSrvThread;
            }
            else
            {
                delete pSrvThread;
            }
        }
        WebSocketServer::~WebSocketServer()
        {
            if (m_pImpl)
            {
                WebSocketSrvThread* pSrvThread = (WebSocketSrvThread*)m_pImpl;
                pSrvThread->Stop();
                delete pSrvThread;
            }
        }
        WebSocketSession::WebSocketSession()
        {
        }
        WebSocketSession::~WebSocketSession()
        {
        }
        bool WebSocketSession::Write(X::Value& value)
        {
            return ((WebSocketSessionImpl*)m_pImpl)->Write(value);
        }
}
}