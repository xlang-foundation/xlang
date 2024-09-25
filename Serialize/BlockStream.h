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
