#pragma once

#include "XLangStream.h"

namespace X
{
	class SMSwapBuffer;
	class SwapBufferStream :
		public X::XLangStream
	{
	public:
		SwapBufferStream();
		~SwapBufferStream();
		void SetSMSwapBuffer(SMSwapBuffer* p);

		// Inherited via GrusStream
		virtual int BlockNum() override;
		virtual blockInfo& GetBlockInfo(int index) override;
		virtual bool NewBlock() override;
		virtual bool MoveToNextBlock() override;
		virtual void Refresh() override;
		virtual bool CanBeOverrideMode() override
		{
			return false;
		}
		virtual void ReInit() override
		{
			XLangStream::ReInit();
			m_blockInfo = { nullptr,0,0 };
			m_blockNum = 0;
		}

	private:
		void Reset();
		SMSwapBuffer* m_pSwapBuffer = nullptr;
		blockInfo m_blockInfo = { nullptr,0,0 };
		int m_blockNum = 0;
	};
}