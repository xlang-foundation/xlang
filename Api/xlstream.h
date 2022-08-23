#ifndef _XLSTREAM_H_
#define _XLSTREAM_H_

namespace X
{
	struct blockInfo
	{
		char* buf;
		long long block_size;
		long long data_size;
	};
	class XLStream
	{
	public:
		virtual void Refresh() = 0;
		virtual int BlockNum() = 0;
		virtual blockInfo& GetBlockInfo(int index) = 0;
		virtual bool NewBlock() = 0;
		virtual bool MoveToNextBlock() = 0;
	};
}

#endif