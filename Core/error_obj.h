#pragma once
#include "object.h"
#include <string>
namespace X
{
	namespace Data
	{
		class Error :
			virtual public XError,
			virtual public Object
		{
		protected:
			std::string m_s;
			int m_code;
		public:
			static void Init();
			static void cleanup();
			Error(int code, std::string& str)
			{
				m_t = ObjType::Error;
				m_s = str;
				m_code = code;
			}
			FORCE_INLINE virtual const char* GetInfo() override
			{
				return m_s.c_str();
			}
			FORCE_INLINE virtual int GetCode() override
			{
				return m_code;
			}
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
		};
	} // namespace Data
} // namespace X