#include "BlockStream.h"


namespace X
{
	BlockStream::BlockStream()
	{
		//m_pProvider = this;
		m_streamKey = 0;// GrusJitHost::I().RegisterStream(this);
	}
	BlockStream::BlockStream(char* buf, STREAM_SIZE size, bool needReleasBuf) :
		BlockStream()
	{
		Block_Info info;
		info.buf = buf;
		info.block_size = size;
		info.data_size = size;
		info.needReleasBuf = needReleasBuf;
		m_blocks.push_back(info);
		m_size = size;
	}
	BlockStream::~BlockStream()
	{
		//GrusJitHost::I().UnregisterStream(m_streamKey);

		for (auto p : m_blocks)
		{
			if (p.needReleasBuf)
			{
				delete p.buf;
			}
		}
		m_blocks.clear();
		m_size = 0;
		curPos = { 0,0 };
	}

	bool BlockStream::NewBlock()
	{
		char* p = new char[BLOCK_SIZE];
		if (p == nullptr)//out of memory
		{
			return false;
		}
		Block_Info info;
		info.buf = p;
		info.block_size = BLOCK_SIZE;
		info.data_size = 0;
		info.needReleasBuf = true;
		m_blocks.push_back(info);
		return true;
	}

	bool BlockStream::MoveToNextBlock()
	{
		return true;
	}

	void BlockStream::Refresh()
	{
	}
}
