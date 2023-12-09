#pragma once
#include "object.h"

namespace X
{
	namespace Data
	{
		//base class for all tensor's ops
		class TensorOperator :
			virtual public XTensorOperator,
			virtual public Object
		{
			std::string m_name;

			X::Value m_opAction;//like a function to work with tensor
			Tensor_OperatorHandler m_opHandler;
			bool m_isUnaryOp = false;
		public:
			TensorOperator(Tensor_OperatorHandler op, bool isUnaryOp):XTensorOperator(0),Object()
			{
				m_t = ObjType::TensorOperator;
				m_opHandler = op;
				m_isUnaryOp = isUnaryOp;
			}
			~TensorOperator()
			{

			}
			virtual FORCE_INLINE void SetName(const char* n) override
			{
				m_name = n;
			}
			FORCE_INLINE std::string& GetName() { return m_name; }
			FORCE_INLINE bool IsUnaryOp() { return m_isUnaryOp; }
			FORCE_INLINE Tensor_OperatorHandler GetOpHandler()
			{
				return m_opHandler;
			}
			//for Tensor Operator,don't need to create a new instance
			//for exampl t1*T.mul()*t2, when calc T.mul()*t2, T.mul() already a new instance
			//can be used to accept t2
			//so overide back to call Object's impl.
			virtual XObj* Clone() override
			{
				return Object::Clone();
			}
			void SetOpAction(X::Value& action)
			{
				m_opAction = action;
			}
		};
	}
}