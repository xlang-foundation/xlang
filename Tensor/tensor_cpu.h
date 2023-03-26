#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "ops_mgt.h"
#include "function.h"
#include "tensor.h"
#include "tensorop.h"
#include "tensor_expression.h"
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
			bool IsSimilar = true;
			int t1_dims_size = (int)(t1.GetDims().size());
			int t2_dims_size = (int)(t2.GetDims().size());
			if (t1_dims_size != t2_dims_size) {//
				IsSimilar = false;
			}
			else {
				for (int i = 0; i < t1_dims_size; i++) {
					if (t1.GetDims()[i].size != t2.GetDims()[i].size) {
						IsSimilar = false;
						break;
					}
				}
			}
			return IsSimilar;
		}
		bool IsProdTensor (X::Data::Tensor& t1, X::Data::Tensor& t2) 
		{
			bool IsProd = false;
			int t1_dims_size = (int)(t1.GetDims().size());
			int t2_dims_size = (int)(t2.GetDims().size());
			if (t1_dims_size <= 1) //one axes
			{
				if (t1.GetDims()[0].size == t2.GetDims()[0].size) 
					IsProd = true;
			}
			else if (t1_dims_size == 2 && t2_dims_size == 2) {//matrixes
				if (t1.GetDims()[1].size == t2.GetDims()[0].size) 
					IsProd = true;
			}
			return IsProd;		
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
			AddMinusMulDiv(input1, input2, retVal, X::Data::Tensor_Operator::Add);
		}
		void Minus(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			AddMinusMulDiv(input1, input2, retVal, X::Data::Tensor_Operator::Minus);
		}
		void Multiply(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "Call Multiply" << std::endl;
			AddMinusMulDiv(input1, input2, retVal, X::Data::Tensor_Operator::Mul);
		}
		void Divide(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "Call Div" << std::endl;
			AddMinusMulDiv(input1, input2, retVal, X::Data::Tensor_Operator::Div);
		}

		inline std::tuple<bool, bool> IsAddable(X::Data::Tensor &t, const X::Value& operand, X::Data::Tensor_Operator op) 
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
					if (op == X::Data::Tensor_Operator::Add || op == X::Data::Tensor_Operator::Minus)
						Addable = IsSimilarTensor(t, *pTensor);	
					if (op == X::Data::Tensor_Operator::Mul)					
						Addable = IsProdTensor(t, *pTensor);
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
					if (ty == X::ValueType::Int64 && val >= -128 && val <= 127) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::UBYTE:
					if (ty == X::ValueType::Int64 && val <= 255) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::SHORT:
					if (ty == X::ValueType::Int64 && val >= -32768 && val <= 32767) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::USHORT:
					if (ty == X::ValueType::Int64 && val <= 65535) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::INT:
					if (ty == X::ValueType::Int64 && val >= -2147483648 && val <= 2147483647) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::UINT:
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
					if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val <= 65535) 
					{
						Addable = true;
					}
					break;
				case X::TensorDataType::FLOAT:
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
			return {Addable, IsNum};
		}

		inline void AddMinusMulDiv(X::Value& input1, X::Value& input2, X::Value& retVal, X::Data::Tensor_Operator op)
		{
			bool bIsAddable =false;
			bool bIsNum = false;
			//input1 and retVal must be tensors
			if (!input1.IsObject() || !retVal.IsObject())
				return;
				
			X::Data::Tensor* pInput1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());
			
			std::tie (bIsAddable, bIsNum) = IsAddable(*pInput1, input2, op);
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
					auto it_proc_scaler_mul = [pInput1, input2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						val *= input2;
						pRetVal->SetDataWithIndices(indices, val);
					};
					auto it_proc_scaler_div = [pInput1, input2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						val /= input2;
						pRetVal->SetDataWithIndices(indices, val);
					};
					if (op == X::Data::Tensor_Operator::Add)
						pInput1->IterateAll(it_proc_scaler_add);
					else if (op == X::Data::Tensor_Operator::Minus)
						pInput1->IterateAll(it_proc_scaler_minus);
					else if (op == X::Data::Tensor_Operator::Mul)
						pInput1->IterateAll(it_proc_scaler_add);
					else if (op == X::Data::Tensor_Operator::Div)
						pInput1->IterateAll(it_proc_scaler_add);

				} //scaler
				else 
				{//tensor only, verified in IsAddable()
					X::Data::Tensor* pInput2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
					auto it_proc_tensor_add = [pInput1, pInput2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						X::Value val_operand = pInput2->GetDataWithIndices(indices);
						val += val_operand;
						std::cout << "In it_proc_tensor_add(), new val ="<< val.GetLongLong() << std::endl;
						pInput1->SetDataWithIndices(indices, val);
						//pRetVal->SetDataWithIndices(indices, val);
					};
					auto it_proc_tensor_minus = [pInput1, pInput2, pRetVal](std::vector<long long>& indices)
					{
						X::Value val = pInput1->GetDataWithIndices(indices);
						X::Value val_operand = pInput2->GetDataWithIndices(indices);
						val -= val_operand;
						std::cout << "In it_proc_tensor_minus(), new val ="<< val.GetLongLong() << std::endl;
						pInput1->SetDataWithIndices(indices, val);
					};
					auto it_proc_tensor_mul = [pInput1, pInput2, pRetVal]()
					{
						/*
						X::Value val = pInput1->GetDataWithIndices(indices);
						X::Value val_operand = pInput2->GetDataWithIndices(indices);
						val -= val_operand;
						std::cout << "In it_proc_tensor_minus(), new val ="<< val.GetLongLong() << std::endl;
						pInput1->SetDataWithIndices(indices, val);
						*/

						//Matrix1 (m,n), Matrix2 (u,v), n = u, after production, new Matrix shape (m,v)
						int m = pInput1->GetDims()[0].size; //rows of matrix1
						int n = pInput1->GetDims()[1].size; //columns of matrix1
						int u = pInput2->GetDims()[0].size; //rows of matrix2
						int v = pInput2->GetDims()[1].size; //columns of matrix2
						int i, j, k;
						X::Value val_1, val_2, val;
						std::vector<long long> indices1, indices2, indices;
						indices.resize(2);
						indices1.resize(2);
						indices2.resize(2);
						for ( i = 0; i < m; i++) {
							for (j = 0; j < v; j ++) {
								//indices.push_back(i);
								//indices.push_back(j);
								indices.assign(i,j);
								val = 0;
								for (k =0; k<n; k++) { //c(i,j) = a(i,0)*b(0,j)+ a(i,1)*b(1,j)+ ...+a(i,n-1)*b(n-1,j)
									//indices1.push_back(i);
									//indices1.push_back(k);
									//indices2.push_back(k);
									//indices2.push_back(j);
									indices1.assign(i,k);
									indices2.assign(k,j);
									val_1 = pInput1->GetDataWithIndices(indices1);
									val_2 = pInput2->GetDataWithIndices(indices2);								
									val += val_1 * val_2;
									//indices1.clear();
									//indices2.clear();
								}
								pRetVal->SetDataWithIndices(indices, val);
								//indices.clear();
							}
						}
					};
					if (op == X::Data::Tensor_Operator::Add) 
						pInput1->IterateAll(it_proc_tensor_add);
					else if (op == X::Data::Tensor_Operator::Minus) 
						pInput1->IterateAll(it_proc_tensor_minus);
					else if (op == X::Data::Tensor_Operator::Mul) 
						it_proc_tensor_mul();
					
					pRetVal = pInput1;
					//*pRetVal = pInput1->Clone();

					std::vector<long long> indices_x;
					X::Value val_x;
					std::cout << "-------------------pInput1--------------------" << std::endl;
					for (int i=0;i<(int)pInput1->GetDims().size();i++)
					{
						std::cout << "return tensor dims:("<<i<< ") = "<<pInput1->GetDims()[i].size << std::endl;
					}
					for (int i=0;i<pInput1->GetDims()[0].size;i++)
					{
						for (int j=0;j<pInput1->GetDims()[1].size;j++)
						{
							indices_x.assign(i,j);
							val_x = pInput1->GetDataWithIndices(indices_x);
							std::cout << "(" << i << "," << j<<")="<< val_x.GetLongLong() <<"," << std::endl;
						}
					}

					std::cout << "-------------------pRetVal--------------------" << std::endl;
					int dcnt = (int)pRetVal->GetDims().size();
					for (int i=0;i<dcnt;i++)
					{
						std::cout << "return tensor dims:("<<i<< ") = "<<pRetVal->GetDims()[i].size << std::endl;
					}
					for (int i=0;i<pRetVal->GetDims()[0].size;i++)
					{
						for (int j=0;j<pRetVal->GetDims()[1].size;j++)
						{
							indices_x.assign(i,j);
							val_x = pRetVal->GetDataWithIndices(indices_x);
							std::cout << "(" << i << "," << j<<")="<< val_x.GetLongLong() <<"," << std::endl;
						}
					}
					std::cout << "------------------------------------------" << std::endl;



				} //tensor
			} //bIsAddable
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