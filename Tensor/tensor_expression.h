#pragma once
#include "object.h"
#include "ops_mgt.h"

namespace X
{
	namespace Data
	{
		enum class Tensor_Operator
		{
			None, Add, Minus, Mul, Div,
			Count
		};
		class TensorExpression :
			virtual public XTensorExpression,
			virtual public Object
		{
			std::string m_name;

			X::Value m_leftVal;
			X::Value m_rightVal; //maybe a Tensor or TensorOperator
			Tensor_Operator m_op = Tensor_Operator::None;
		public:
			TensorExpression() :XTensorExpression(0), XObj(), Object()
			{
				m_t = ObjType::TensorExpression;
			}
			void SetLeftVal(X::Value& val)
			{
				m_leftVal = val;
			}
			void SetRightVal(X::Value& val, Tensor_Operator op)
			{
				m_rightVal = val;
				m_op = op;
			}
			inline void SetName(std::string& n)
			{
				m_name = n;
			}
			inline std::string& GetName() { return m_name; }
			inline X::Value& GetLeftValue() { return m_leftVal; }
			inline X::Value& GetRightValue() { return m_rightVal; }
			inline Tensor_Operator GetOp() { return m_op; }

			virtual bool Multiply(const X::Value& r, X::Value& retVal) override
			{
				auto* newTensor = new TensorExpression();
				X::Value left(this);
				newTensor->SetLeftVal(left);
				X::Value right(r);
				newTensor->SetRightVal(right, Tensor_Operator::Mul);
				std::string newName = OpsManager::I().GenNewName();
				newTensor->SetName(newName);
				retVal = newTensor;
				return true;
			}
			virtual bool Add(const X::Value& r, X::Value& retVal) override
			{
				auto* newTensor = new TensorExpression();
				X::Value left(this);
				newTensor->SetLeftVal(left);
				X::Value right(r);
				newTensor->SetRightVal(right, Tensor_Operator::Add);
				//if left has name, then add new tensor with a name
				std::string newName = OpsManager::I().GenNewName();
				newTensor->SetName(newName);
				retVal = newTensor;
				return true;
			}
		};
	}
}

