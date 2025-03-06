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
#include "object.h"
#include "port.h"
#include "dict.h"
#include "list.h"

namespace X
{
	namespace Data
	{
#define CHAR_START 32 //space
#define CHAR_END   126//~

		class Binary:
			virtual public XBin,
			virtual public Object
		{
		protected:
			char* m_data = nullptr;
			bool m_OwnData = true;//if true, this object will delete m_data in destructor
			size_t m_size;
		public:
			static void Init();
			static void cleanup();

			//if bOwnData is true,means the data passed in
			// will be keep in this object, and will be deleted in destructor
			//but this data must alloced in same heap
			Binary(char* data, size_t size,bool bOwnData):
				XBin(0)
			{//new copy
				m_t = ObjType::Binary;
				m_OwnData = bOwnData;
				if (data == nullptr && size>0)
				{
					m_data = new char[size];
					m_OwnData = true;
				}
				else
				{
					m_data = data;
				}
				m_size = size;
			}
			virtual Binary& operator +=(X::Value& r) override
			{
				if (r.IsObject() && r.GetObj()->GetType() == X::ObjType::Binary)
				{
					auto pBin = dynamic_cast<Binary*>(r.GetObj());
					if (pBin)
					{
						size_t newSize = m_size + pBin->Size();
						char* newData = new char[newSize];
						memcpy(newData, m_data, m_size);
						memcpy(newData + m_size, pBin->Data(), pBin->Size());
						if (m_OwnData)
						{
							delete m_data;
						}
						m_data = newData;
						m_size = newSize;
					}
				}
				return *this;
			}
			virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
			{
				Object::ToBytes(rt, pContext,stream);
				stream << m_size;
				stream.append(m_data, m_size);
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				stream >> m_size;
				if (m_data)
				{
					delete m_data;
				}
				m_data = new char[m_size];
				m_OwnData = true;
				stream.CopyTo(m_data, m_size);
				return true;
			}
			virtual bool FromString(const char* strCoded) override
			{
				if (m_data)
				{
					delete m_data;
				}
				auto size = strlen(strCoded);
				if (size > 2 && strCoded[0] == 'b' && strCoded[1] == '\'')
				{
					m_data = new char[size];//bytes converted from thi string,
					//which size should be less or eqaul to this string's size
					m_OwnData = true;
					auto i = 2;
					size_t byteIndex = 0;
					bool bCorrect = true;
					while(i<size)
					{
						unsigned char ch = strCoded[i];
						if (ch == '\\')
						{
							if ((i + 4) <= size 
								&& strCoded[i + 1] == 'x')
							{
								ch = ((strCoded[i + 2]-'0') << 8) + strCoded[i + 3]-'0';
								i += 4;
								m_data[byteIndex++] = ch;
							}
							else
							{
								bCorrect = false;
								break;
							}
						}
						else if (ch >= CHAR_START && ch <= CHAR_END)
						{
							m_data[byteIndex++] = ch;
							i++;
						}
						else
						{
							bCorrect = false;
							break;
						}
					}
					m_size = byteIndex;
					return true;
				}
				else
				{
					return false;
				}
			}
			virtual const char* ToString(bool WithFormat = false) override
			{
				std::string retStr="b'";
				for (size_t i = 0; i < m_size; i++)
				{
					unsigned char ch = m_data[i];
					if (ch >= CHAR_START && ch <= CHAR_END)
					{
						retStr += ch;
					}
					else
					{
						const int online_len = 5;
						char strBuf[online_len];
						SPRINTF(strBuf, online_len, "\\x%02X",ch);
						retStr += strBuf;
					}
				}
				retStr += "'";
				return GetABIString(retStr);
			}
			FORCE_INLINE virtual char* Data() override { return m_data; }
			virtual char* BorrowDta() override 
			{ 
				char* p = m_data;
				m_OwnData = false;
				m_data = nullptr;
				return p;
			}
			FORCE_INLINE virtual long long  Size()  override { return m_size; }
			~Binary()
			{
				if (m_OwnData && m_data!=nullptr)
				{
					delete m_data;
				}
			}
			virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue) override
			{
				return true;
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count)
			{

				AutoLock autoLock(m_lock);
				if(Size() == 0)
				{
					List* pOutList = new List();
					pOutList->IncRef();
					return pOutList;
				}

				if (startIndex < 0 || startIndex >= Size())
				{
					return nullptr;
				}
				if (count == -1)
				{
					count = Size() - startIndex;
				}
				if ((startIndex + count) > Size())
				{
					return nullptr;
				}
				List* pOutList = new List();
				pOutList->IncRef();
				for (long long i = 0; i < count; i++)
				{
					long long idx = startIndex + i;
					X::Value val((unsigned char)m_data[idx]);
					Dict* dict = new Dict();
					auto objIds = CombinObjectIds(IdList, (unsigned long long)idx);
					dict->Set("Id", objIds);
					//Data::Str* pStrName = new Data::Str(it.first);
					//dict->Set("Name", X::Value(pStrName));
					std::string valType = "byte";
					Data::Str* pStrType = new Data::Str(valType);
					dict->Set("Type", X::Value(pStrType));
					if (!val.IsObject() || (val.IsObject() &&
						dynamic_cast<Object*>(val.GetObj())->IsStr()))
					{
						dict->Set("Value", val);
					}
					else if (val.IsObject())
					{
						X::Value objId((unsigned long long)val.GetObj());
						dict->Set("Value", objId);
						X::Value valSize(val.GetObj()->Size());
						dict->Set("Size", valSize);
					}
					X::Value valDict(dict);
					pOutList->Add(rt, valDict);
				}
				return pOutList;
			}
		};
	}
}