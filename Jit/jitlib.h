#pragma once
#include <string>
#include "jitblock.h"
#include "list.h"
#include "exp.h"
#include "op.h"
#include "port.h"

//one XLang moudle( one file) use one CodeGen to generate code
//so inside this file, will include Func and/or class code

namespace X 
{
	namespace Jit
	{
		class JitCompiler;
		typedef void (*Jit_Load_Proc)(void* pHost, int** funcIdList,void*** funcs, const char*** hash_list,int* cnt);
		enum class LangType
		{
			cpp,
			cs,
			java,
			golang,
			cuda,
			compute_shader,
			Count
		};
		struct ParamInfo
		{
			std::string name;
			std::string type;
			X::Value defaultValue;
		};
		struct FuncInfo
		{
			std::string name;
			std::string hash;//if compiled,then set this
			std::string has_from_lib;
			LangType langType;
			std::string code;
			std::vector<ParamInfo> params;
			std::string retType;
			bool isExternImpl = false;
			std::vector<std::string> externImplFileNameList;
			AST::Jit_Stub_Proc stub = nullptr;
			AST::JitBlock* jitBlock = nullptr;
		};

		class JitLib
		{
			std::vector<JitCompiler*> m_compilers;
			std::string m_path;
			std::string m_moduleName;//module name without ext
			std::vector<FuncInfo> m_funcs;
			bool m_buildWithDebug = false;
			std::string m_XLangIncludePath;
		public:
			FORCE_INLINE void SetFuncStub(int* funcIdList, void** funcs, const char** funcHashList,int cnt)
			{
				int funcNum = (int)m_funcs.size();
				for (int i = 0; i < cnt; i++)
				{
					int funcId = funcIdList[i];
					if (funcId < funcNum)
					{
						auto& funcInfo = m_funcs[funcId];
						funcInfo.stub = (AST::Jit_Stub_Proc)funcs[i];
						funcInfo.has_from_lib = funcHashList[i];
						funcInfo.jitBlock->SetJitStub(funcInfo.stub);
					}
				}
			}
			std::string QuotePath(std::string& strSrc);
			FORCE_INLINE bool IsBuildWithDebug() { return m_buildWithDebug; }
			FORCE_INLINE std::string& GetXLangIncludePath() { return m_XLangIncludePath; }
			FORCE_INLINE auto& GetFuncs()
			{
				return m_funcs;
			}
			FORCE_INLINE std::string& Path()
			{
				return m_path;
			}
			FORCE_INLINE std::string& ModuleName()
			{
				return m_moduleName;
			}
			FORCE_INLINE void SetXLangEngPath(std::string& path)
			{
				m_XLangIncludePath = path + Path_Sep + "Api";
			}
			JitLib(std::string& moduleName)
			{
				std::string right;
				auto pos = moduleName.rfind('\\');
				if (pos == moduleName.npos)
				{
					pos = moduleName.rfind('/');
				}
				if (pos != moduleName.npos)
				{
					m_path = moduleName.substr(0, pos);
					right = moduleName.substr(pos + 1);
				}
				else
				{
					right = moduleName;
				}
				pos = right.rfind('.');
				if (pos != right.npos)
				{
					m_moduleName = right.substr(0, pos);
				}
				else
				{
					m_moduleName = right;
				}
				for (int i = 0; i < (int)LangType::Count; i++)
				{
					m_compilers.push_back(nullptr);
				}
			}
			~JitLib();
			FORCE_INLINE bool ExtractFuncInfo(AST::JitBlock* pBlock)
			{
				FuncInfo funcInfo;
				std::string funcName = pBlock->GetName();
				auto* params = pBlock->GetParams();
				auto& paramList = params->GetList();
				auto& strRetType = pBlock->GetRetType();

				funcInfo.langType = LangType::cpp;//TODO:
				funcInfo.name = funcName;
				funcInfo.retType = strRetType;
				funcInfo.jitBlock = pBlock;

				funcInfo.code = pBlock->GetCode();
				for (auto* i : paramList)
				{
					std::string strVarName;
					std::string strVarType;
					Value defaultValue;
					switch (i->m_type)
					{
					case AST::ObType::Var:
					{
						auto* varName = dynamic_cast<AST::Var*>(i);
						String& szName = varName->GetName();
						strVarName = std::string(szName.s, szName.size);
					}
					break;
					case AST::ObType::Assign:
					{
						auto* assign = dynamic_cast<AST::Assign*>(i);
						auto* varName = dynamic_cast<AST::Var*>(assign->GetL());
						String& szName = varName->GetName();
						strVarName = std::string(szName.s, szName.size);
						AST::Expression* defVal = assign->GetR();
						auto* pExprForDefVal = new Data::Expr(defVal);
						defaultValue = Value(pExprForDefVal);
					}
					break;
					case AST::ObType::Param:
					{
						auto* param = dynamic_cast<AST::Param*>(i);
						param->Parse(strVarName, strVarType, defaultValue);
					}
					break;
					}
					funcInfo.params.push_back({ strVarName ,strVarType,defaultValue });
				}
				m_funcs.push_back(funcInfo);
				return true;
			}
			bool AddBlock(AST::JitBlock* pBlock)
			{
				ExtractFuncInfo(pBlock);
				return true;
			}
			bool Build();
		};
	}
}