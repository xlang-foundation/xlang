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

#include "SwapBufferStream.h"
#include "SMSwapBuffer.h"

namespace X
{
	namespace IPC
	{
		SwapBufferStream::SwapBufferStream()
		{
		}

		SwapBufferStream::~SwapBufferStream()
		{
		}
		void SwapBufferStream::SetSMSwapBuffer(SMSwapBuffer* p)
		{
			m_pSwapBuffer = p;
			Reset();
		}

		void SwapBufferStream::Reset()
		{
			if (m_pSwapBuffer)
			{
				m_blockNum = 1;//when reset the SwapBuffer, we can start count block number from 1
				m_blockInfo.buf = m_pSwapBuffer->GetBuffer();
				m_blockInfo.block_size = m_pSwapBuffer->BufferSize();
			}
			else
			{
				m_blockNum = 0;
			}
		}
		int SwapBufferStream::BlockNum()
		{
			return m_blockNum;
		}

		blockInfo& SwapBufferStream::GetBlockInfo(int index)
		{
			return m_blockInfo;
		}

		bool SwapBufferStream::NewBlock()
		{
			PayloadFrameHead& head = m_pSwapBuffer->GetHead();
			//because request new block, means previous block is full,
			//so set to BufferSize of m_pSwapBuffer
			head.blockSize = m_pSwapBuffer->BufferSize();
			head.payloadType = PayloadType::Send;

			m_pSwapBuffer->EndWrite();//Notify another side
			bool bOK = m_pSwapBuffer->BeginRead(SHORT_WAIT_TIMEOUT);//wait for ack back
			if (!bOK)
			{
				throw XLangStreamException(-100);
			}
			Refresh();
			bOK = (head.payloadType == PayloadType::Ack);
			m_pSwapBuffer->EndRead();
			if (bOK)
			{
				m_pSwapBuffer->BeginWrite();
				m_blockNum++;
			}
			return bOK;
		}

		bool SwapBufferStream::MoveToNextBlock()
		{
			PayloadFrameHead& head = m_pSwapBuffer->GetHead();
			m_pSwapBuffer->BeginWrite();
			head.payloadType = PayloadType::Ack;
			m_pSwapBuffer->EndWrite();
			bool bOK = m_pSwapBuffer->BeginRead(SHORT_WAIT_TIMEOUT);//wait for ack back
			if (!bOK)
			{
				//throw GrusStreamException(-100);
				return false;
			}
			Refresh();
			m_blockNum++;
			bOK = (head.payloadType == PayloadType::Send ||
				head.payloadType == PayloadType::SendLast);
			if (!bOK)
			{
				bOK = bOK;
			}
			return bOK;
		}

		void SwapBufferStream::Refresh()
		{
			PayloadFrameHead& head = m_pSwapBuffer->GetHead();
			m_blockInfo.block_size = head.blockSize;
			m_blockInfo.data_size = head.blockSize;
		}
	} // namespace IPC
} // namespace X
