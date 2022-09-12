#ifndef _XLSTREAM_H_
#define _XLSTREAM_H_

namespace X
{
	typedef long long STREAM_SIZE;
	struct blockIndex
	{
		int blockIndex;
		STREAM_SIZE offset;
	};
	struct blockInfo
	{
		char* buf;
		long long block_size;
		long long data_size;
	};
	class XLStream
	{
	public:
		virtual STREAM_SIZE Size() = 0;
		virtual blockIndex GetPos() = 0;
		virtual void SetPos(blockIndex pos) = 0;
		virtual void Refresh() = 0;
		virtual int BlockNum() = 0;
		virtual blockInfo& GetBlockInfo(int index) = 0;
		virtual bool NewBlock() = 0;
		virtual bool MoveToNextBlock() = 0;
		virtual bool FullCopyTo(char* buf, STREAM_SIZE bufSize) = 0;
	};
}

#endif