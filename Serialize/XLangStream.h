#pragma once

#include <string>
#include <unordered_map>
#include <exception>
#include <string.h>
#include "xlstream.h"
#include "value.h"

namespace X 
{
    namespace AST { class Scope; }
    class Runtime;
    class XLangStreamException
        : public std::exception
    {
    public:
        XLangStreamException(int code)
        {
            m_code = code;
        }
        virtual const char* what() const throw()
        {
            return "XLangStream exception happened";
        }
        int Code()
        {
            return m_code;
        }
    private:
        int m_code = 0;
    };
    class ScopeSpace
    {
        friend class XLangStream;
        std::unordered_map<unsigned long long, void*> m_map;
        AST::Scope* m_curScope = nullptr;
        Runtime* m_rt = nullptr;
        XObj* m_pContext = nullptr;
    public:
        void SetCurrentScope(AST::Scope* p)
        {
            m_curScope = p;
        }
        void SetContext(Runtime* rt, XObj* pContext)
        {
            m_rt = rt;
            m_pContext = pContext;
        }
        Runtime* RT() { return m_rt; }
        XObj* Context() { return m_pContext; }
        AST::Scope* GetCurrentScope()
        {
            return m_curScope;
        }
        void Add(unsigned long long id, void* addr)
        {
            m_map.emplace(std::make_pair(id, addr));
        }
        void* Query(unsigned long long id)
        {
            auto it = m_map.find(id);
            if (it != m_map.end())
            {
                return it->second;
            }
            else
            {
                return nullptr;
            }
        }
    };
    class XLangStream :
        public XLStream
    {
        ScopeSpace m_scope_space;
    public:
        XLangStream();
        ~XLangStream();

        ScopeSpace& ScopeSpace() { return m_scope_space; }
        void SetProvider(XLStream* p)
        {
            m_pProvider = p;
            SetPos(p->GetPos());
        }
        void ResetPos()
        {
            curPos = { 0,0 };
        }
        virtual STREAM_SIZE Size() override
        {
            return m_size;
        }
        virtual bool FullCopyTo(char* buf, STREAM_SIZE bufSize) override;
        bool CopyTo(char* buf, STREAM_SIZE size);
        bool appendchar(char c);
        bool fetchchar(char& c);
        bool fetchstring(std::string& str);
        bool append(char* data, STREAM_SIZE size);
        inline bool Skip(STREAM_SIZE size)
        {
            return CopyTo(nullptr, size);
        }
        template<typename T>
        XLangStream& operator<<(T v)
        {
            append((char*)&v, sizeof(v));
            return *this;
        }
        XLangStream& operator<<(const char c)
        {
            appendchar(c);
            return *this;
        }
        XLangStream& operator<<(std::string v)
        {
            int size = (int)v.size() + 1;
            append((char*)v.c_str(), size);
            return *this;
        }
        XLangStream& operator<<(const char* str)
        {
            int size = (int)strlen(str) + 1;
            append((char*)str, size);
            return *this;
        }
        XLangStream& operator<<(X::Value& v);
        XLangStream& operator>>(X::Value& v);
        template<typename T>
        XLangStream& operator>>(T& v)
        {
            CopyTo((char*)&v, sizeof(v));
            return *this;
        }
        XLangStream& operator>>(std::string& v)
        {
            fetchstring(v);
            return *this;
        }
        XLangStream& operator>>(char& c)
        {
            fetchchar(c);
            return *this;
        }
        unsigned long long GetKey()
        {
            return m_streamKey;
        }
        inline virtual blockIndex GetPos() override
        {
            return curPos;
        }
        inline virtual void SetPos(blockIndex pos) override
        {
            curPos = pos;
            m_size = CalcSize(curPos);
        }
        STREAM_SIZE CalcSize(blockIndex pos);
        STREAM_SIZE CalcSize();
        void SetOverrideMode(bool b)
        {
            m_InOverrideMode = b;
        }
        bool IsEOS()
        {
            if ((BlockNum() - 1) == curPos.blockIndex)
            {
                blockInfo& blk = GetBlockInfo(curPos.blockIndex);
                return (blk.data_size == curPos.offset);
            }
            else
            {
                return false;
            }
        }
        virtual bool CanBeOverrideMode()
        {
            return true;
        }
        virtual void ReInit()
        {
            m_streamKey = 0;
            curPos = { 0,0 };
            m_size = 0;
            m_InOverrideMode = false;
        }
    protected:
        XLStream* m_pProvider = nullptr;//real impl.
        unsigned long long m_streamKey = 0;
        blockIndex curPos = { 0,0 };
        STREAM_SIZE m_size = 0;
        bool m_InOverrideMode = false;

        // Inherited via JitStream
        virtual void Refresh() override;
        virtual int BlockNum() override;
        virtual blockInfo& GetBlockInfo(int index) override;
        virtual bool NewBlock() override;
        virtual bool MoveToNextBlock() override;
    };
}
