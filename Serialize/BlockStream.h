#pragma once

#include "XLangStream.h"
#include <vector>
namespace X
{
    class BlockStream :
        public XLangStream
    {
        struct Block_Info :
            public blockInfo
        {
            bool needReleasBuf;
        };
    public:
        BlockStream();
        BlockStream(char* buf, STREAM_SIZE size, bool needReleasBuf);

        ~BlockStream();
    protected:
        // Inherited via GrusStream
        virtual int BlockNum() override
        {
            return (int)m_blocks.size();
        }
        virtual blockInfo& GetBlockInfo(int index) override
        {
            return m_blocks[index];
        }
        virtual bool NewBlock() override;
        virtual bool MoveToNextBlock() override;
        virtual void Refresh() override;
    private:
        STREAM_SIZE BLOCK_SIZE = 32 * 1024;
        std::vector<Block_Info> m_blocks;
    };
}
