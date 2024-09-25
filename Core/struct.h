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
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		enum class CType 
		{
			c_struct,
			c_char,
			c_wchar,
			c_byte,
			c_ubyte,
			c_short,
			c_ushort,
			c_int,
			c_uint,
			c_long,
			c_ulong,
			c_longlong,
			c_ulonglong,
			c_float,
			c_double,
			c_bool,
			c_xvalue,
			c_void_p,  // Pointer type, simplification for general pointers
			c_invalid
		};
		enum class DataType
		{
			c_struct,
			c_union
		};

		class XlangStruct :
			virtual public XStruct,
			virtual public Object
		{
			DataType m_type = DataType::c_struct;
			bool m_bOwnData = true;
			char* m_pData = nullptr;//hold Structs data
			int m_size = 0;
			Obj_Func_Scope<0> m_fieldScope;
			static const size_t typeSizes[];
			static const std::string typeNames[];
		public:
			struct Field {
				std::string name;
				CType type;
				bool isPointer;
				int bits;  // Number of bits for the field, applicable for bit fields
				size_t offset;// Offset of the field from the start of the structure
				
				Field(const std::string& name, CType type, bool isPointer = false, int bits = 0)
					: name(name), type(type), isPointer(isPointer), bits(bits) {}
			};
			inline void addField(const std::string& name, CType type, bool isPointer = false, int bits = 0) {
				m_fields.emplace_back(name, type, isPointer, bits);
			}
			virtual void addField(
				const char* name, const char* type, 
				bool isPointer = false, int bits = 0) override
			{
				std::string strName(name);
				std::string strType(type);
				addField(strName, strType, isPointer, bits);
			}
			inline void addField(const std::string& name, std::string& type, 
				bool isPointer = false, int bits = 0) {
				auto ty = getCTypeFromName(type);
				if (ty != CType::c_invalid)
				{
					m_fields.emplace_back(name, ty, isPointer, bits);
				}
			}
			virtual bool Build() override;
		private:
			// List of fields in the struct
			std::vector<Field> m_fields;
			size_t calculateUnionSize() const {
				size_t maxSize = 0;
				for (const auto& field : m_fields) {
					size_t size = typeSizes[static_cast<int>(field.type)];
					if (field.isPointer) {
						size = sizeof(void*);
					}
					maxSize = maxSize>size? maxSize:size;
				}
				return maxSize;
			}

			size_t calculateStructureSize() {
				size_t totalSize = 0;
				size_t maxAlignment = 0;

				for (auto& field : m_fields) {
					size_t size = typeSizes[static_cast<int>(field.type)];
					size_t alignment = size;  // Simplistic alignment assumption

					if (field.isPointer) {
						size = sizeof(void*);
						alignment = size;
					}

					// Align the current offset
					totalSize = (totalSize + alignment - 1) & ~(alignment - 1);
					field.offset = totalSize;  // Store the current offset before adding the size
					totalSize += size;  // Add size of the current field
					maxAlignment = maxAlignment>alignment? maxAlignment: alignment;
				}

				// Final alignment of the structure
				totalSize = (totalSize + maxAlignment - 1) & ~(maxAlignment - 1);
				return totalSize;
			}

			static CType getCTypeFromName(const std::string& name) {
				for (int i = 0; i < static_cast<int>(CType::c_invalid); ++i) {
					if (typeNames[i] == name) {
						return static_cast<CType>(i);
					}
				}
				return CType::c_invalid;
			}
		public:
			static void Init();
			static void cleanup();
			XlangStruct():
				XStruct(0),
				Object()
			{
				m_t = ObjType::Struct;
			}
			XlangStruct(char* data,int size, bool asRef):
				XlangStruct()
			{
				m_size = size;
				if (asRef)
				{
					m_pData = data;
					m_bOwnData = false;
				}
				else if(size>0)
				{
					m_bOwnData = true;
					m_pData = new char[size];
					if (data)
					{
						memcpy(m_pData, data, size);
					}
				}
			}
			~XlangStruct()
			{
				if (m_bOwnData && m_pData)
				{
					delete m_pData;
				}
			}
			bool GetFieldValue(int fieldIndex, Value& v) 
			{
				if (fieldIndex < 0 || fieldIndex >= m_fields.size()) {
					return false;  // Return false if the index is out of range
				}

				const auto& field = m_fields[fieldIndex];  // Use the correct field vector
				// Calculate the address from which the data should be read
				const char* fieldAddress = m_pData + field.offset;

				switch (field.type) {
				case CType::c_char:
					v = *(reinterpret_cast<const char*>(fieldAddress));
					break;
				case CType::c_wchar:
					v = *(reinterpret_cast<const wchar_t*>(fieldAddress));
					break;
				case CType::c_byte:
					v = *(reinterpret_cast<const char*>(fieldAddress));
					break;
				case CType::c_ubyte:
					v = *(reinterpret_cast<const unsigned char*>(fieldAddress));
					break;
				case CType::c_short:
					v = *(reinterpret_cast<const short*>(fieldAddress));
					break;
				case CType::c_ushort:
					v = *(reinterpret_cast<const unsigned short*>(fieldAddress));
					break;
				case CType::c_int:
					v = *(reinterpret_cast<const int*>(fieldAddress));
					break;
				case CType::c_uint:
					v = *(reinterpret_cast<const unsigned int*>(fieldAddress));
					break;
				case CType::c_long:
					v = *(reinterpret_cast<const long*>(fieldAddress));
					break;
				case CType::c_ulong:
					v = *(reinterpret_cast<const unsigned long*>(fieldAddress));
					break;
				case CType::c_longlong:
					v = *(reinterpret_cast<const long long*>(fieldAddress));
					break;
				case CType::c_ulonglong:
					v = *(reinterpret_cast<const unsigned long long*>(fieldAddress));
					break;
				case CType::c_float:
					v = *(reinterpret_cast<const float*>(fieldAddress));
					break;
				case CType::c_double:
					v = *(reinterpret_cast<const double*>(fieldAddress));
					break;
				case CType::c_bool:
					v = *(reinterpret_cast<const bool*>(fieldAddress));
					break;
				case CType::c_xvalue:
					v = *(reinterpret_cast<const X::Value*>(fieldAddress));
					break;
				case CType::c_void_p:
					v = *(reinterpret_cast<const void* const*>(fieldAddress));
					break;
				default:
					return false;  // Return false for unsupported types
				}
				return true;  // Return true on successful assignment
			}

			void SetFieldValue(int fieldIndex, Value& v) {
				auto& field = m_fields[fieldIndex];  // Use the correct field vector
				// Calculate the address where the data should be written
				char* fieldAddress = m_pData + field.offset;

				switch (field.type) {
				case CType::c_char:
					*reinterpret_cast<char*>(fieldAddress) = (char)v;
					break;
				case CType::c_wchar:
					*reinterpret_cast<wchar_t*>(fieldAddress) = (unsigned short)v;
					break;
				case CType::c_byte:
					*reinterpret_cast<char*>(fieldAddress) = (char)v;
					break;
				case CType::c_ubyte:
					*reinterpret_cast<unsigned char*>(fieldAddress) = (unsigned char)v;
					break;
				case CType::c_short:
					*reinterpret_cast<short*>(fieldAddress) = (short)v;
					break;
				case CType::c_ushort:
					*reinterpret_cast<unsigned short*>(fieldAddress) = (unsigned short)v;
					break;
				case CType::c_int:
					*reinterpret_cast<int*>(fieldAddress) = (int)v;
					break;
				case CType::c_uint:
					*reinterpret_cast<unsigned int*>(fieldAddress) = (unsigned int)v;
					break;
				case CType::c_long:
					*reinterpret_cast<long*>(fieldAddress) = (long)v;
					break;
				case CType::c_ulong:
					*reinterpret_cast<unsigned long*>(fieldAddress) = (unsigned long)v;
					break;
				case CType::c_longlong:
					*reinterpret_cast<long long*>(fieldAddress) = (long long)v;
					break;
				case CType::c_ulonglong:
					*reinterpret_cast<unsigned long long*>(fieldAddress) = (unsigned long long)v;
					break;
				case CType::c_float:
					*reinterpret_cast<float*>(fieldAddress) = (float)v;
					break;
				case CType::c_double:
					*reinterpret_cast<double*>(fieldAddress) = (double)v;
					break;
				case CType::c_bool:
					*reinterpret_cast<bool*>(fieldAddress) = (bool)v;
					break;
				case CType::c_xvalue:
					*reinterpret_cast<X::Value*>(fieldAddress) = v;
					break;
				case CType::c_void_p:
					*reinterpret_cast<void**>(fieldAddress) = (void*)(uintptr_t)v;  // Using uintptr_t for pointer conversion
					break;
				default:
#if not defined(BARE_METAL)				
					throw std::invalid_argument("Unsupported field type.");
#endif				
					break;	
				}
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			FORCE_INLINE virtual long long Size() { return m_size; }
			virtual char* Data() override
			{
				return m_pData;
			}
			virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
			{
				AutoLock autoLock(m_lock);
				Object::ToBytes(rt, pContext, stream);
				stream << m_size;
				if (m_size)
				{
					stream.append(m_pData, m_size);
				}
				return true;
			}
			virtual bool FromBytes(X::XLangStream& stream) override
			{
				AutoLock autoLock(m_lock);
				stream >> m_size;
				if (m_bOwnData && m_pData)
				{
					delete m_pData;
					m_pData = nullptr;
				}
				if (m_size)
				{
					m_pData = new char[m_size];
					m_bOwnData = true;
					stream.CopyTo(m_pData, m_size);
				}
				return true;
			}
		};
		class XlangStructField :
			virtual public Object
		{
			std::string m_name;
			int m_index = 0;
		public:
			XlangStructField(std::string& name,int index) :
				Object()
			{
				m_name = name;
				m_index = index;
				m_t = ObjType::StructField;
			}
			bool GetValue(XRuntime* rt0, XObj* pContext, Value& v)
			{
				if (pContext == nullptr || pContext->GetType() != X::ObjType::Struct)
				{
					return false;
				}
				XlangStruct* pStruct = dynamic_cast<XlangStruct*>(pContext);
				return pStruct->GetFieldValue(m_index, v);
			}
			bool SetValue(XRuntime* rt0, XObj* pContext, Value& v)
			{
				if (pContext == nullptr || pContext->GetType() != X::ObjType::Struct)
				{
					return false;
				}
				XlangStruct* pStruct = dynamic_cast<XlangStruct*>(pContext);
				pStruct->SetFieldValue(m_index, v);
				return true;
			}
		};
	}
}