#pragma once

#include <string.h> //for memcpy
#include <utility>

/*
 Xlang API will cross Shared Library boundary, so avoid to use STL
 this head file include replacement of STL
*/
namespace X
{
	namespace Port
	{
		template <typename T> class Function;

		template <typename R, typename... Args>
		class Function<R(Args...)>
		{
			typedef R(*invoke_fn_t)(char*, Args&&...);
			typedef void (*construct_fn_t)(char*, char*);
			typedef void (*destroy_fn_t)(char*);

			template <typename Functor>
			static R invoke_fn(Functor* fn, Args&&... args)
			{
				return (*fn)(std::forward<Args>(args)...);
			}
			template <typename Functor>
			static void construct_fn(Functor* construct_dst, Functor* construct_src)
			{
				new (construct_dst) Functor(*construct_src);
			}
			template <typename Functor>
			static void destroy_fn(Functor* f)
			{
				f->~Functor();
			}
			invoke_fn_t invoke_f;
			construct_fn_t construct_f;
			destroy_fn_t destroy_f;
			char* data_ptr =nullptr;
			size_t data_size=0;

			//for debug
			int m_duplicatedCount = 1;
			Function* copy_from = nullptr;
		public:
			Function()
				: invoke_f(nullptr)
				, construct_f(nullptr)
				, destroy_f(nullptr)
				, data_ptr(nullptr)
				, data_size(0)
			{}

			template <typename Functor>
			Function(Functor f)
				: invoke_f(reinterpret_cast<invoke_fn_t>(invoke_fn<Functor>))
				, construct_f(reinterpret_cast<construct_fn_t>(construct_fn<Functor>))
				, destroy_f(reinterpret_cast<destroy_fn_t>(destroy_fn<Functor>))
				, data_ptr(new char[sizeof(Functor)])
				, data_size(sizeof(Functor))
			{
				this->construct_f(this->data_ptr, reinterpret_cast<char*>(&f));
			}
			Function(Function const& rhs)
				: invoke_f(rhs.invoke_f)
				, construct_f(rhs.construct_f)
				, destroy_f(rhs.destroy_f)
				, data_size(rhs.data_size)
			{
				m_duplicatedCount = rhs.m_duplicatedCount + 1;
				copy_from = (Function*)&rhs;
				if (this->invoke_f)
				{
					this->data_ptr = new char[this->data_size];
					this->construct_f(this->data_ptr, rhs.data_ptr);
				}
			}
			inline operator bool() const
			{
				return (data_ptr != nullptr);
			}
			inline void operator = (const Function& v)
			{
				copy_from = (Function*)&v;
				m_duplicatedCount = v.m_duplicatedCount + 1;
				invoke_f = v.invoke_f;
				construct_f = v.construct_f;
				destroy_f = v.destroy_f;
				data_size = v.data_size;
				if (this->invoke_f)
				{
					this->data_ptr = new char[this->data_size];
					this->construct_f(this->data_ptr, v.data_ptr);
				}
			}
			~Function()
			{
				if (data_ptr != nullptr)
				{
					this->destroy_f(this->data_ptr);
					delete data_ptr;
					data_ptr = nullptr;
				}
			}
			R operator()(Args... args) const
			{
				return this->invoke_f(this->data_ptr, std::forward<Args>(args)...);
			}
		};

		template<typename T>
		class vector
		{
			T* m_data = nullptr;
			int m_size = 0;
			int m_curPos = 0;
			inline void Copy(T* dst, T* src, int size)
			{
				for (int i = 0; i < m_size; i++)
				{
					dst[i] = src[i];
				}
			}
		public:
			class iterator 
			{
			public:
				iterator(T* ptr) : ptr_(ptr) {}
				iterator operator++() { ptr_++; return *this; }
				bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
				T& operator*() { return *ptr_; }
			private:
				T* ptr_;
			};
			vector(int size)
			{
				if (size > 0)
				{
					m_data = new T[size];
					m_size = size;
				}
			}
			vector(const vector<T>& v)
			{
				m_size = v.m_size;
				if (m_size > 0)
				{
					m_data = new T[m_size];
					Copy(m_data, v.m_data, m_size);
				}
			}
			~vector()
			{
				if (m_data)
				{
					delete[] m_data;
				}
			}
			inline size_t size() { return m_size; }
			inline T* Data() { return m_data; }
			inline T& operator[](size_t idx)
			{
				return m_data[idx];
			}
			void operator= (const vector<T>& v)
			{
				m_size = v.m_size;
				if (m_size > 0)
				{
					m_data = new T[m_size];
					Copy(m_data, v.m_data, m_size);
				}
			}
			inline void push_back(T v)
			{
				m_data[m_curPos++] = v;
			}
			void clear()
			{
				m_size = 0;
				if (m_data)
				{
					delete[] m_data;
					m_data = nullptr;
				}
			}
			inline void resize(int size)
			{
				if (size != m_size)
				{
					if (size > 0)
					{
						auto* pNewData = new T[size];
						int copySize = size > m_size ? m_size : size;
						if (copySize > 0)
						{
							Copy(pNewData, m_data, copySize);
						}
						if (m_data)
						{
							delete[] m_data;
						}
						m_data = pNewData;
						m_size = size;
					}
					else
					{
						m_size = 0;
						if (m_data)
						{
							delete[] m_data;
							m_data = nullptr;
						}
					}
				}
			}
			inline void Close()
			{//finish put, will change size according m_curPos
				m_size = m_curPos;
			}
			iterator begin() { return iterator(m_data); }
			iterator end() { return iterator(m_data + m_size); }
		};
		//-1: Key must be a string, major used by KWARGS,optimized for fast key lookuping
		//but this map only accept add and lookup operators 
		//-2: for save memory, if the key is const char* from outside which means will hold this pointer
		//as the process lifetime, just keep a ref to it
		//if not, need to stat this string need to recreate here
		//so set ownKey as true
		template<typename T>
		class StringMap
		{
			struct MapItem
			{
				const char* key = nullptr;
				T val;
				bool ownKey = false;//if need to release key
				int next=-1;//have same first char will be linked,but use index in m_data
				void Free()
				{
					if (ownKey && key)
					{
						delete key;
					}
				}
				bool Match(const char* key2)
				{
					return (strcmp(key, key2) == 0);
				}
			};
			MapItem* m_data = nullptr;
			int m_size = 0;
			int m_curPos = 0;
		public:
			class iterator
			{
			public:
				iterator(MapItem* ptr) : ptr_(ptr) {}
				iterator operator++() { ptr_++; return *this; }
				bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
				MapItem& operator*() { return *ptr_; }
			private:
				MapItem* ptr_;
			};
			StringMap()
			{

			}
			StringMap(int size)
			{
				if (size > 0)
				{
					m_data = new MapItem[size];
					m_size = size;
				}
			}
			StringMap(const StringMap<T>& v)
			{
				m_size = v.m_size;
				if (m_size > 0)
				{
					m_data = new MapItem[m_size];
					for (int i = 0; i < m_size; i++)
					{
						MapItem& newItem = m_data[i];
						MapItem& oldItem = v.m_data[i];
						if (!oldItem.ownKey)
						{
							newItem.key = oldItem.key;
						}
						else
						{
							int keyLen = strlen(oldItem.key) + 1;
							newItem.key = new char[keyLen];
							memcpy((char*)newItem.key, oldItem.key, keyLen);
						}
						newItem.val = oldItem.val;
						newItem.ownKey = oldItem.ownKey;
						newItem.next = oldItem.next;
					}
				}
			}
			void operator= (const StringMap<T>& v)
			{
				m_size = v.m_size;
				if (m_size > 0)
				{
					m_data = new MapItem[m_size];
					for (int i = 0; i < m_size; i++)
					{
						MapItem& newItem = m_data[i];
						MapItem& oldItem = v.m_data[i];
						if (!oldItem.ownKey)
						{
							newItem.key = oldItem.key;
						}
						else
						{
							int keyLen = (int)strlen(oldItem.key) + 1;
							newItem.key = new char[keyLen];
							memcpy((char*)newItem.key, oldItem.key, keyLen);
						}
						newItem.val = oldItem.val;
						newItem.ownKey = oldItem.ownKey;
						newItem.next = oldItem.next;
					}
				}
				m_curPos = m_size;//move to end to make add as append
			}
			~StringMap()
			{
				if (m_data)
				{
					for (int i = 0; i < m_size; i++)
					{
						auto& item = m_data[i];
						item.Free();
					}
					delete[] m_data;
				}
			}
			inline void CopyItems(MapItem* pNewMap)
			{
				if (m_size > 0)
				{
					for (int i = 0; i < m_size; i++)
					{
						MapItem& newItem = pNewMap[i];
						MapItem& oldItem = m_data[i];
						newItem.key = oldItem.key;
						newItem.val = oldItem.val;
						newItem.ownKey = oldItem.ownKey;
						newItem.next = oldItem.next;
						oldItem.ownKey = false;//will keep key for new map
					}
				}
				if (m_data)
				{
					delete[] m_data;
				}
			}
			inline void resize(int size)
			{
				//we don't do new size more than old size,
				//then we can skip the link re-build flow
				if (size > m_size)
				{
					auto* pNewData = new MapItem[size];
					CopyItems(pNewData);
					m_data = pNewData;
					m_size = size;
				}
			}
			inline void Add(MapItem& it)
			{
				Add(it.key, it.val, !it.ownKey);
			}
			void Add(const char* key, T& val,bool needToCopyKey = false)
			{
				if (key == nullptr)
				{
					return;
				}
				if (m_curPos >= m_size)
				{
					MapItem* pNewMap = new MapItem[m_size + 1];
					CopyItems(pNewMap);
					m_size++;
					m_data = pNewMap;
				}
				//append in the end
				int pos = m_curPos++;
				MapItem& item = m_data[pos];
				if (!needToCopyKey)
				{
					item.key = key;
				}
				else
				{
					int keyLen = strlen(key)+1;
					item.key = new char[keyLen];
					memcpy((char*)item.key, key, keyLen);
				}
				item.val = val;
				item.ownKey = needToCopyKey;

				char c = key[0];
				//check each item, find the bucket
				for (int i = 0; i < pos; i++)
				{
					MapItem* item0 = &m_data[i];
					if (c == item0->key[0])
					{
						//find the last item in the link
						while (item0->next>=0)
						{
							item0 = &m_data[item0->next];
						}
						//join the link of the bucket
						item0->next = pos;
						break;
					}
				}
			}
			inline bool Has(const char* key)
			{
				return (find(key) != nullptr);
			}
			inline size_t size() { return m_size; }
			iterator begin() { return iterator(m_data); }
			iterator end() { return iterator(m_data + m_size); }
			inline bool matchKeys(const char* key1, const char* key2)
			{
				return (strcmp(key1, key2) == 0);
			}
			inline MapItem* find(const char* key)
			{
				if (key == nullptr)
				{
					return nullptr;
				}
				//we accept empty string as key 
				char c = key[0];
				MapItem* pMatchedItem = nullptr;
				for (int i = 0; i < m_size; i++)
				{
					MapItem* item = &m_data[i];
					if (c == item->key[0])
					{
						if (item->next == -1)
						{
							pMatchedItem = item;
							break;
						}
						//if there is a bucket,loop to do exact match
						while (item->next>=0)
						{
							if (matchKeys(key, item->key))
							{
								pMatchedItem = item;
								break;
							}
							item = &m_data[item->next];
						}
						if (pMatchedItem)
						{
							break;
						}
					}
				}
				return pMatchedItem;
			}
		};
	}
}
