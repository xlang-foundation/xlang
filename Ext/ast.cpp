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

#include "ast.h"
#include "Hosting.h"
#include "block.h"
#include "var.h"
#include "func.h"
#include "module.h"
#include "import.h"
#include "../Jit/md5.h"

namespace X
{
    namespace AST
    {
        AstWrapper::~AstWrapper()
        {
        }
        X::Value AstWrapper::LoadFromStringImpl(std::string& moduleName, std::string& xcode)
        {
            unsigned long long moduleKey = 0;
            auto* pModule = Hosting::I().Load(moduleName.c_str(), xcode.c_str(), (int)xcode.size(), moduleKey, md5(xcode));
            if (pModule == nullptr)
            {
                return X::Value();
            }
            X::XPackageValue<AstNode> valNode;
            (*valNode).SetNode(pModule);
            return valNode;
        }

        X::Value AstWrapper::LoadFromString(std::string xcode)
        {
            static std::string defaultModuleName("default");
            return LoadFromStringImpl(defaultModuleName, xcode);
        }
        X::Value AstWrapper::LoadFromFile(std::string fileName)
        {
            std::string xcode;
            LoadStringFromFile(fileName, xcode);
            return LoadFromStringImpl(fileName,xcode);
        }
        X::Value AstNode::getNodevalue(Expression* pNode)
        {
            return X::Value();
        }
        X::Value AstNode::getNodeChildren(Expression* pNode)
        {
            if(pNode == nullptr)
            {
                return X::Value();
			}
            //check if pNode is a a Block
            Block* pBlock = dynamic_cast<Block*>(pNode);
            if (pBlock)
            {
                X::List list;
                auto& body = pBlock->GetBody();
                for (auto* node : body)
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(node);
                    list += valNode;
                }
                //if it is Module, need to append FORCE_INLINE comments 
                if (pNode->m_type == ObType::Module)
				{
					Module* pModule = dynamic_cast<Module*>(pNode);
					if (pModule)
					{
						auto& comments = pModule->GetInlineComments();
						for (auto* node : comments)
						{
							X::XPackageValue<AstNode> valNode;
							(*valNode).SetNode(node);
							list += valNode;
						}
					}
				}

                return list;
            }
            X::Value retVal;
            auto type = pNode->m_type;
            switch (type)
            {
            case X::AST::ObType::Dot:
            case X::AST::ObType::Assign:
            case X::AST::ObType::Pair:
            case X::AST::ObType::In:
            case X::AST::ObType::PipeOp:
            case X::AST::ObType::BinaryOp:
            {
                X::List list;
                BinaryOp* pOp = dynamic_cast<BinaryOp*>(pNode);
                if(pOp && pOp->GetL())
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(pOp->GetL());
                    list += valNode;
                }
                if (pOp && pOp->GetR())
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(pOp->GetR());
                    list += valNode;
                }
                retVal = list;
            }
                break;
            case X::AST::ObType::Decor:
            case X::AST::ObType::UnaryOp:
            {
                X::List list;
                UnaryOp* pOp = dynamic_cast<UnaryOp*>(pNode);

                if (pOp && pOp->GetR())
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(pOp->GetR());
                    list += valNode;
                }
                retVal = list;
            }
                break;
            case X::AST::ObType::Var:
                break;
            case X::AST::ObType::Str:
                break;
            case X::AST::ObType::Const:
                break;
            case X::AST::ObType::Number:
                break;
            case X::AST::ObType::Double:
                break;
            case X::AST::ObType::Param:
            {
                X::List list;
                Param* pOp = dynamic_cast<Param*>(pNode);
                if (pOp && pOp->GetName())
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(pOp->GetName());
                    list += valNode;
                }
                if (pOp && pOp->GetType())
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(pOp->GetType());
                    list += valNode;
                }
                retVal = list;
            }
                break;
            case X::AST::ObType::List:
            {
                X::List list;
                List* pOp = dynamic_cast<List*>(pNode);
                auto& expList = pOp->GetList();
                for(auto* exp : expList)
                {
                    X::XPackageValue<AstNode> valNode;
                    (*valNode).SetNode(exp);
                    list += valNode;
                }
                retVal = list;
            }
                break;
            case X::AST::ObType::Func:
            case X::AST::ObType::BuiltinFunc:
            {
                Func* pFunc = dynamic_cast<Func*>(m_pNode);
                if (pFunc)
                {
                    X::List list;
                    {
                        X::XPackageValue<AstNode> valNode;
                        (*valNode).SetNode(pFunc->GetParams());
                        list += valNode;
                    }
                    {
                        X::XPackageValue<AstNode> valNode;
                        (*valNode).SetNode(pFunc->GetRetType());
                        list += valNode;
                    }
                    retVal = list;
                }
            }
                break;
            case X::AST::ObType::Module:
                break;
            case X::AST::ObType::Block:
                break;
            case X::AST::ObType::Class:
                break;
            case X::AST::ObType::From:
                break;
            case X::AST::ObType::ColonOp:
                break;
            case X::AST::ObType::CommaOp:
                break;
            case X::AST::ObType::SemicolonOp:
                break;
            case X::AST::ObType::FeedOp:
                break;
            case X::AST::ObType::ActionOp:
                break;
            case X::AST::ObType::As:
                break;
            case X::AST::ObType::For:
                break;
            case X::AST::ObType::While:
                break;
            case X::AST::ObType::If:
                break;
            case X::AST::ObType::ExternDecl:
                break;
            case X::AST::ObType::AwaitOp:
                break;
            case X::AST::ObType::Thru:
                break;
            case X::AST::ObType::Import:
            {
                Import* pImport = dynamic_cast<Import*>(m_pNode);
                if (pImport)
                {
                    X::List list;
                    if(pImport->GetFrom())
                    {
                        X::XPackageValue<AstNode> valNode;
                        (*valNode).SetNode(pImport->GetFrom());
                        list += valNode;
                    }
                    if(pImport->GetThru())
                    {
                        X::XPackageValue<AstNode> valNode;
                        (*valNode).SetNode(pImport->GetThru());
                        list += valNode;
                    }
                    if(pImport->GetImports())
                    {
                        X::XPackageValue<AstNode> valNode;
                        (*valNode).SetNode(pImport->GetImports());
                        list += valNode;
                    }
                    retVal = list;
                }
            }
                break;
            case X::AST::ObType::NamespaceVar:
                break;
            default:
                break;
            }
            return retVal;
        }
        X::Value AstNode::Access(X::Port::vector<X::Value>& IdxAry)
        {
            return X::Value();
        }
        X::Value AstNode::get_parent()
        {
            if (m_pNode && m_pNode->GetParent())
            {
                X::XPackageValue<AstNode> valNode;
                (*valNode).SetNode(m_pNode->GetParent());
                return valNode;
            }
            return X::Value();
        }
        X::Value AstNode::get_name()
        {
            if (m_pNode == nullptr)
            {
                return X::Value();
            }
            auto type = m_pNode->m_type;
            switch (type)
            {
            case X::AST::ObType::Base:
                break;
            case X::AST::ObType::InlineComment:
            {
                InlineComment* pObj = dynamic_cast<InlineComment*>(m_pNode);
                if (pObj)
                {
                    auto str = pObj->GetString();
                    return X::Value(str);
                }
            }
                break;
            case X::AST::ObType::Assign:
            {
                Assign* pAssign = dynamic_cast<Assign*>(m_pNode);
                if (pAssign->GetL())
                {
                    Var* pVar = dynamic_cast<Var*>(pAssign->GetL());
                    if (pVar)
                    {
                        std::string name = pVar->GetNameString();
                        return X::Value(name);
                    }
                }
            }
                break;
            case X::AST::ObType::BinaryOp:
                break;
            case X::AST::ObType::UnaryOp:
                break;
            case X::AST::ObType::PipeOp:
                break;
            case X::AST::ObType::In:
                break;
            case X::AST::ObType::Var:
            {
                Var* pVar = dynamic_cast<Var*>(m_pNode);
                if (pVar)
                {
                    auto name = pVar->GetNameString();
                    return X::Value(name);
                }
            }

                break;
            case X::AST::ObType::Str:
            {
                Str* pStr = dynamic_cast<Str*>(m_pNode);
                if (pStr)
                {
                    std::string strVal(pStr->GetChars(), pStr->Size());
                    return X::Value(strVal);
                }
            }
                break;
            case X::AST::ObType::Const:
                break;
            case X::AST::ObType::Number:
            {
                Number* pNumber = dynamic_cast<Number*>(m_pNode);
                if (pNumber)
                {
                    return X::Value(pNumber->GetVal());
                }
            }
                break;
            case X::AST::ObType::Double:
            {
                Double* pDouble = dynamic_cast<Double*>(m_pNode);
                if (pDouble)
                {
                    return X::Value(pDouble->GetVal());
                }
            }
                break;
            case X::AST::ObType::Param:
                break;
            case X::AST::ObType::List:
                break;
            case X::AST::ObType::Pair:
                break;
            case X::AST::ObType::Dot:
                break;
            case X::AST::ObType::Decor:
                break;
            case X::AST::ObType::Func:
            case X::AST::ObType::BuiltinFunc:
            {
                Func* pFunc = dynamic_cast<Func*>(m_pNode);
                if (pFunc)
                {
                    auto name = pFunc->GetNameString();
                    return X::Value(name);
                }
            }
                break;
            case X::AST::ObType::Module:
            {
                Module* pObj = dynamic_cast<Module*>(m_pNode);
                if (pObj)
                {
                    return X::Value(pObj->GetModuleName());
                }
            }
                break;
            case X::AST::ObType::Block:
                break;
            case X::AST::ObType::Class:
                break;
            case X::AST::ObType::From:
                break;
            case X::AST::ObType::ColonOp:
                break;
            case X::AST::ObType::CommaOp:
                break;
            case X::AST::ObType::SemicolonOp:
                break;
            case X::AST::ObType::FeedOp:
                break;
            case X::AST::ObType::ActionOp:
                break;
            case X::AST::ObType::As:
                break;
            case X::AST::ObType::For:
                break;
            case X::AST::ObType::While:
                break;
            case X::AST::ObType::If:
                break;
            case X::AST::ObType::ExternDecl:
                break;
            case X::AST::ObType::AwaitOp:
                break;
            case X::AST::ObType::Thru:
                break;
            case X::AST::ObType::Import:
                break;
            case X::AST::ObType::NamespaceVar:
                break;
            default:
                break;
            }
    
            return X::Value();
        }
        X::Value AstNode::get_value()
        {
            if (m_pNode == nullptr)
            {
                return X::Value();
            }
            auto type = m_pNode->m_type;
            switch (type)
            {
            case X::AST::ObType::Assign:
            {
                Assign* pAssign = dynamic_cast<Assign*>(m_pNode);
                if (pAssign->GetR())
                {
                    Expression* pValExp = pAssign->GetR();
                    ExecAction action;
                    X::Value val;
                    ExpExec(pValExp,nullptr, action, nullptr, val);
                    return val;
                }
            }
				break;
            default:
                break;
            }
            return X::Value();
        }
        X::Value AstNode::get_operator_type()
        {
            auto type = m_pNode->m_type;
            AST::Operator* pOp = nullptr;
            switch (type)
            {
            case X::AST::ObType::Assign:
                pOp = (AST::Operator*)m_pNode;
                break;
            case X::AST::ObType::BinaryOp:
                pOp = (AST::Operator*)m_pNode;
                break;
            case X::AST::ObType::UnaryOp:
                pOp = (AST::Operator*)m_pNode;
                break;
            default:
                break;
            }

            if (pOp)
            {
                OP_ID opId = pOp->GetId();

                // Convert operator ID to string
                switch (opId)
                {
                case OP_ID::None:
                    return X::Value("None");
                case OP_ID::Parenthesis_L:
                    return X::Value("(");
                case OP_ID::Brackets_L:
                    return X::Value("[");
                case OP_ID::Curlybracket_L:
                    return X::Value("{");
                case OP_ID::TableBracket_L:
                    return X::Value("[");  // Assuming table bracket is also [
                case OP_ID::Slash:
                    return X::Value("/");
                case OP_ID::Colon:
                    return X::Value(":");
                case OP_ID::Comma:
                    return X::Value(",");
                case OP_ID::Tab:
                    return X::Value("\t");
                    // Assignment operators
                case OP_ID::Equ:
                    return X::Value("=");
                case OP_ID::AddEqu:
                    return X::Value("+=");
                case OP_ID::MinusEqu:
                    return X::Value("-=");
                case OP_ID::MulEqu:
                    return X::Value("*=");
                case OP_ID::DivEqu:
                    return X::Value("/=");
                case OP_ID::ModEqu:
                    return X::Value("%=");
                case OP_ID::FloorDivEqu:
                    return X::Value("//=");
                case OP_ID::PowerEqu:
                    return X::Value("**=");
                case OP_ID::AndEqu:
                    return X::Value("&=");
                case OP_ID::OrEqu:
                    return X::Value("|=");
                case OP_ID::NotEqu:
                    return X::Value("^=");
                case OP_ID::RightShiftEqu:
                    return X::Value(">>=");
                case OP_ID::LeftShitEqu:
                    return X::Value("<<=");
                    // Comparison operators
                case OP_ID::Equal:
                    return X::Value("==");
                case OP_ID::NotEqual:
                    return X::Value("!=");
                case OP_ID::Great:
                    return X::Value(">");
                case OP_ID::Less:
                    return X::Value("<");
                case OP_ID::GreatAndEqual:
                    return X::Value(">=");
                case OP_ID::LessAndEqual:
                    return X::Value("<=");
                    // Logical operators
                case OP_ID::And:
                    return X::Value("and");
                case OP_ID::Or:
                    return X::Value("or");
                    // Control flow
                case OP_ID::Break:
                    return X::Value("break");
                case OP_ID::Continue:
                    return X::Value("continue");
                case OP_ID::Pass:
                    return X::Value("pass");
                case OP_ID::ReturnType:
                    return X::Value("->");
                case OP_ID::ReturnOp:
                    return X::Value("return");
                case OP_ID::Count:
                    return X::Value("Count");
                default:
                    return X::Value("Unknown");
                }
            }

            return X::Value(""); // Return empty string if no operator was found
        }
        X::Value AstNode::get_type()
        {
            if (m_pNode == nullptr)
            {
                return X::Value();
            }
            auto type = m_pNode->m_type;
            switch (type)
            {
            case X::AST::ObType::InlineComment:
                return X::Value("Comment");
                break;
            case X::AST::ObType::Assign:
                return X::Value("Assign");
                break;
            case X::AST::ObType::BinaryOp:
                return X::Value("BinaryOp");
                break;
            case X::AST::ObType::UnaryOp:
                return X::Value("UnaryOp");
                break;
            case X::AST::ObType::PipeOp:
                return X::Value("PipeOp");
                break;
            case X::AST::ObType::In:
                return X::Value("In");
                break;
            case X::AST::ObType::Var:
                return X::Value("Var");
                break;
            case X::AST::ObType::Str:
                return X::Value("Str");
                break;
            case X::AST::ObType::Const:
                return X::Value("Const");
                break;
            case X::AST::ObType::Number:
                return X::Value("Number");
                break;
            case X::AST::ObType::Double:
                return X::Value("Double");
                break;
            case X::AST::ObType::Param:
                return X::Value("Param");
                break;
            case X::AST::ObType::List:
                return X::Value("List");
                break;
            case X::AST::ObType::Pair:
                return X::Value("Pair");
                break;
            case X::AST::ObType::Dot:
                return X::Value("Dot");
                break;
            case X::AST::ObType::Decor:
                return X::Value("Decor");
                break;
            case X::AST::ObType::Func:
                return X::Value("Func");
                break;
            case X::AST::ObType::BuiltinFunc:
                return X::Value("BuiltinFunc");
                break;
            case X::AST::ObType::Module:
                return X::Value("Module");
                break;
            case X::AST::ObType::Block:
                return X::Value("Block");
                break;
            case X::AST::ObType::Class:
                return X::Value("Class");
                break;
            case X::AST::ObType::From:
                return X::Value("From");
                break;
            case X::AST::ObType::ColonOp:
                return X::Value("ColonOp");
                break;
            case X::AST::ObType::CommaOp:
                return X::Value("CommaOp");
                break;
            case X::AST::ObType::SemicolonOp:
                return X::Value("SemicolonOp");
                break;
            case X::AST::ObType::FeedOp:
                return X::Value("FeedOp");
                break;
            case X::AST::ObType::ActionOp:
                return X::Value("ActionOp");
                break;
            case X::AST::ObType::As:
                return X::Value(":As");
                break;
            case X::AST::ObType::For:
                return X::Value("For");
                break;
            case X::AST::ObType::While:
                return X::Value("While");
                break;
            case X::AST::ObType::If:
                return X::Value("If");
                break;
            case X::AST::ObType::ExternDecl:
                return X::Value("ExternDecl");
                break;
            case X::AST::ObType::AwaitOp:
                return X::Value("AwaitOp");
                break;
            case X::AST::ObType::Thru:
                return X::Value("Thru");
                break;
            case X::AST::ObType::Import:
                return X::Value("Import");
                break;
            case X::AST::ObType::NamespaceVar:
                return X::Value("NamespaceVar");
                break;
            default:
                break;
            }
            return X::Value();
        }
    }
}
//Test Load Xlang code
#if 0
{
    std::string x_code;
    std::string path("C:\\ToGithub\\CantorAI\\Galaxy\\test\\testsimple.x");
    LoadStringFromFile(path, x_code);
    X::Value moduleObj;
    X::g_pXHost->LoadCode("test", x_code.c_str(), x_code.size(), moduleObj);
    X::Module module(moduleObj);
    X::Value findExp = module->Search("my_name=");
    module->Search("cantor.log");
    x_code = findExp.ToString();
}
#endif