#pragma once

#include "xpackage.h"
#include "xlang.h"
#include "ops_mgt.h"
#include "function.h"
#include "tensor.h"
#include "tensorop.h"
#include <iostream>


namespace X
{
	class CpuTensor
	{
	
	public:
		BEGIN_PACKAGE(CpuTensor)
			APISET().AddTensorBinaryOp("add", &CpuTensor::Add);
			APISET().AddTensorBinaryOp("minus", &CpuTensor::Minus);
			APISET().AddTensorBinaryOp("mul", &CpuTensor::Multiply);
			APISET().AddTensorBinaryOp("div", &CpuTensor::Divide);
			APISET().AddTensorUnaryOp("permute", &CpuTensor::Permute);
		END_PACKAGE
	private:
		bool IsSimilarTensor (X::Data::Tensor& t1, X::Data::Tensor& t2) 
		{
			bool sim = true;
			int nd = (int)(t1.GetDims().size());
			int td = (int)(t2.GetDims().size());
			if (nd != td) {//
				sim = false;
			}
			else {
				for (int i = 0; i < nd; i++) {
					if (t1.GetDims()[i].size != t2.GetDims()[i].size) {
						sim = false;
						break;
					}
				}
			}
			return sim;
		}


	public:
		CpuTensor()
		{
		}
		void Permute(X::ARGS& params, X::KWARGS& kwParams,X::Value input, X::Value& retVal)
		{

		}
		void Add(X::ARGS& params, X::KWARGS& kwParams,X::Value input1,X::Value input2,X::Value& retVal)
		{
			AddMinus(input1, input2, retVal, X::Data::Tensor_Operator::Add);
		}
		void Minus(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			AddMinus(input1, input2, retVal, X::Data::Tensor_Operator::Minus);
		}
		void Multiply(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "Call Multiply" << std::endl;
			MulDiv(input1, input2, X::Data::Tensor_Operator::Mul);
			retVal = retVal;
		}
		void Divide(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "Call Div" << std::endl;
			MulDiv(input1, input2, X::Data::Tensor_Operator::Div);
			retVal = retVal;
		}

		inline std::tuple<bool, bool> IsAddable(X::Data::Tensor &t, const X::Value& operand) 
		{
			std::cout << "In IsAddable()" << std::endl;

			bool Addable = false;
			bool IsNum = false;

			auto ty = ((X::Value)operand).GetType();

			if (ty == X::ValueType::Object) {//only tensor, no list, set, dictionary, complex, etc.
				auto* pObj = ((X::Value)operand).GetObj();
				if (pObj->GetType() == ObjType::Tensor)
				{
					X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*> (pObj);
					Addable = IsSimilarTensor(t, *pTensor);					
				}
				std::cout << "In IsAddable(), Addable=" <<Addable <<", IsNum="<< IsNum<< std::endl;
				return {Addable, IsNum};
			}

			auto val = 0;
			switch (ty)
			{
			case X::ValueType::Int64:
				IsNum = true;
				val = ((X::Value)operand).GetLongLong();
				break;
			case X::ValueType::Double:
				IsNum = true;
				val = ((X::Value)operand).GetDouble();
				break;
			default:  //todo, what about boolean?
				break;
			}

			if (IsNum) //number only
			{
				switch (t.GetDataType())
				{
				case X::TensorDataType::BOOL:
					break;
				case X::TensorDataType::BYTE:
					//if (ty == X::ValueType::Int64 && val >= (-1)*pow(2,7) && val < pow(2,7) ) 
					if (ty == X::ValueType::Int64 && val >= -128 && val <= 127) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::UBYTE:
					//if (ty == X::ValueType::Int64 && val < pow(2,8)) 
					if (ty == X::ValueType::Int64 && val <= 255) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::SHORT:
					//if (ty == X::ValueType::Int64 && val >= (-1)*pow(2,15) && val < pow(2,15) ) 
					if (ty == X::ValueType::Int64 && val >= -32768 && val <= 32767) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::USHORT:
					//if (ty == X::ValueType::Int64 && val < pow(2,16)) 
					if (ty == X::ValueType::Int64 && val <= 65535) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::INT:
					//if (ty == X::ValueType::Int64 && val >= (-1)*pow(2,31) && val < pow(2,31)) 
					if (ty == X::ValueType::Int64 && val >= -2147483648 && val <= 2147483647) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::UINT:
					//if (ty == X::ValueType::Int64 && val < pow(2,32)) 
					if (ty == X::ValueType::Int64 && val <= 4294967295)
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::LONGLONG:
					if (ty == X::ValueType::Int64 && val >= -9,223,372,036,854,775,808 && val <= 9,223,372,036,854,775,807) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::ULONGLONG:
					if (ty == X::ValueType::Int64 && val <= 18,446,744,073,709,551,615) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::HALFFLOAT:
					//if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val < pow(2,16)) 
					if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val <= 65535) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::FLOAT:
					//if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val < pow(2,32)) 
					if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val <= 4294967295) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::DOUBLE:
				case X::TensorDataType::CFLOAT:
					if (ty == X::ValueType::Double || ty == X::ValueType::Double) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::CDOUBLE:
					break;
				default:
					break;
				}
			}
			std::cout << "In IsAddable(), Addable=" <<Addable <<", IsNum="<< IsNum<< std::endl;
			return {Addable, IsNum};
		}

		inline void AddMinus(X::Value& input1, X::Value& input2, X::Value& retVal, X::Data::Tensor_Operator op)
		{
			bool bIsAddable =false;
			bool bIsNum = false;
			//input1 and retVal must be tensors
			if (!input1.IsObject() || !retVal.IsObject())
				return;
				
			X::Data::Tensor* pInput1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());

			std::tie (bIsAddable, bIsNum) = IsAddable(*pInput1, input2);
			if (bIsAddable) {
				AutoLock(m_lock);
				if (bIsNum) 
				{
					auto it_proc_scaler_add = [pInput1, input2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						val += input2;
						pRetVal->SetDataWithIndices(indices, val);
					};
					auto it_proc_scaler_minus = [pInput1, input2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						val -= input2;
						pRetVal->SetDataWithIndices(indices, val);
					};
					if (op == X::Data::Tensor_Operator::Add) //only Add or Minus allowed
						pInput1->IterateAll(it_proc_scaler_add);
					else
						pInput1->IterateAll(it_proc_scaler_minus);

				} //scaler
				else 
				{//tensor only, verified in IsAddable()
					std::cout << "In it_proc_tensor_add(), "<< std::endl;
					X::Data::Tensor* pInput2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
					auto it_proc_tensor_add = [pInput1, pInput2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						X::Value val_operand = pInput2->GetDataWithIndices(indices);
						val += val_operand;
						std::cout << "In it_proc_tensor_add(), new val ="<< val.GetLongLong() << std::endl;
						pInput1->SetDataWithIndices(indices, val);
					};
					auto it_proc_tensor_minus = [pInput1, pInput2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						X::Value val_operand = pInput2->GetDataWithIndices(indices);
						val -= val_operand;
						std::cout << "In it_proc_tensor_minus(), new val ="<< val.GetLongLong() << std::endl;
						pInput1->SetDataWithIndices(indices, val);
					};
					if (op == X::Data::Tensor_Operator::Add) //only Add or Minus allowed
						pInput1->IterateAll(it_proc_tensor_add);
					else
						pInput1->IterateAll(it_proc_tensor_minus);
					
					pRetVal = pInput1;

				} //tensor
			} //bIsAddable
		}	
		inline void MulDiv(X::Value& input1, X::Value& input2, X::Data::Tensor_Operator op)
		{
			bool bIsAddable =false;
			bool bIsNum = false;
			//input1 must be a tensor
			if (!input1.IsObject())
				return;			
			X::Data::Tensor* pInput1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
			std::tie (bIsAddable, bIsNum) = IsAddable(*pInput1, input2);
			//Tensor tempT(*this);
			//Tensor *ptempT = dynamic_cast<Tensor*>clone();
	
			if (bIsAddable) {
				AutoLock(m_lock);
				if (bIsNum) 
				{
					auto it_proc = [this, pInput1, input2, op](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						val *= input2;
						pInput1->SetDataWithIndices(indices, val);
					};
					pInput1->IterateAll(it_proc);
				}
				else 
				{//tensor only, verified in IsAddable()
					X::Data::Tensor* pInput2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
					std::vector<int> axes;
					axes.push_back(1);
					axes.push_back(0);
					pInput2->permute(axes);
					auto it_proc_tensor = [this, pInput1, pInput2, op](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						//exchange m,n postions
						X::Value val_operand = pInput2->GetDataWithIndices(indices);
						val *= val_operand;
						pInput1->SetDataWithIndices(indices, val);
					};
					pInput1->IterateAll(it_proc_tensor);
				}

			}
		}
		/*
		Tensor& operator+(X::Value& val){
			AddMinus(val, X::Data::Tensor_Operator::Add);
			return *this;
		}
		virtual Tensor& operator+=(X::Value& val) override{
			AddMinus(val, X::Data::Tensor_Operator::Add);
			return *this;
		}
		Tensor& Add(X::Value& val){
			AddMinus(val, X::Data::Tensor_Operator::Add);
			return *this;
		}
		Tensor& operator-(X::Value& val){
			AddMinus(val, X::Data::Tensor_Operator::Minus);
			return *this;
		}
		virtual Tensor& operator-=(X::Value& val) override{
			AddMinus(val, X::Data::Tensor_Operator::Minus);
			return *this;
		}
		Tensor& Minus(X::Value& val){
			AddMinus(val, X::Data::Tensor_Operator::Minus);
			return *this;
		}
		Tensor& operator*(X::Value& val){
			printf ("in operator*");
			Multiply(val, X::Data::Tensor_Operator::Minus);
			return *this;
		}
		//virtual Tensor& operator*=(X::Value& val) override{
	    //	Multiply(val, X::Data::Tensor_Operator::Minus);
		//	return *this;
		//}
		Tensor& Mul(X::Value& val){
			printf ("in Mul");
			Multiply(val, X::Data::Tensor_Operator::Mul);
			return *this;
		}
		*/
	};
}