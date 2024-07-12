#pragma once

#include "object.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		enum class CType 
		{
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
			c_void_p,  // Pointer type, simplification for general pointers
			c_invalid
		};
		enum class DataType
		{
			c_struct,
			c_union
		};
		class XlangStructField :
			virtual public Object
		{
			std::string m_name;
		public:
			XlangStructField(std::string& name) :
				Object()
			{
				m_name = name;
				m_t = ObjType::StructField;
			}
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

				Field(const std::string& name, CType type, bool isPointer = false, int bits = 0)
					: name(name), type(type), isPointer(isPointer), bits(bits) {}
			};
			inline void addField(const std::string& name, CType type, bool isPointer = false, int bits = 0) {
				m_fields.emplace_back(name, type, isPointer, bits);
			}
			inline void addField(const std::string& name, std::string& type, bool isPointer = false, int bits = 0) {
				auto ty = getCTypeFromName(type);
				if (ty != CType::c_invalid)
				{
					m_fields.emplace_back(name, ty, isPointer, bits);
				}
			}
			bool Build()
			{
				int cnt = m_fields.size();
				m_fieldScope.InitWithNumber(cnt);
				for (int i = 0; i < cnt; i++)
				{
					auto& field = m_fields[i];
					XlangStructField* pField = new XlangStructField(field.name);
					X::Value objField(pField);
					m_fieldScope.AddObject(field.name.c_str(), objField);
				}
				if (m_type == DataType::c_struct)
				{
					m_size = calculateStructureSize();
				}
				else
				{
					m_size = calculateUnionSize();
				}
				return true;
			}
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

			size_t calculateStructureSize() const {
				size_t totalSize = 0;
				size_t maxAlignment = 0;

				for (const auto& field : m_fields) {
					size_t size = typeSizes[static_cast<int>(field.type)];
					size_t alignment = size;  // Simplistic alignment assumption

					if (field.isPointer) {
						size = sizeof(void*);
						alignment = size;
					}

					// Align the current offset
					totalSize = (totalSize + alignment - 1) & ~(alignment - 1);
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
				else
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
	}
}