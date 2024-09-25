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

namespace X
{
	namespace IPC
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
	} // namespace IPC
} // namespace X