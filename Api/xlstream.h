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