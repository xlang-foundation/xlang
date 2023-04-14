#pragma once
#include "xpackage.h"
#include "xlang.h"
#include "ops_mgt.h"
#include "function.h"
#include "tensor.h"
#include "tensorop.h"
#include "tensor_expression.h"
#include <iostream>
#include "list.h"
#include <unistd.h>
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
			APISET().AddTensorBinaryOp("matmul", &CpuTensor::Matmul);
			APISET().AddTensorBinaryOp("matmul", &CpuTensor::Matmul);
			APISET().AddTensorBinaryOp("conv2d", &CpuTensor::Conv2d);
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
			if (t1_dims_size == 2 && t2_dims_size == 2) {//matrixes
				if (t1.GetDims()[1].size == t2.GetDims()[0].size) 
					IsProd = true;
			}
			else if (t1_dims_size <= 1 && t2_dims_size == 2) //one axes
			{
				if (t1.GetDims()[0].size == t2.GetDims()[1].size ) 
					IsProd = true;
			}
			else if (t1_dims_size == 2 && t2_dims_size <= 1) //one axes
			{
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
			std::vector<int> axes;
			if (params.size() > 0)
			{
				auto& p0 = params[0];
				if (p0.IsList())
				{
					X::Data::List* pList = dynamic_cast<X::Data::List*>(p0.GetObj());
					int axesCnt = (int)pList->Size();
					for (int i = 0; i < axesCnt; i++)
					{
						axes.push_back((int)pList->Get(i));
					}
				}
				else
				{
					return;
				}
			}
			else
			{
				return;
			}
			if (!input.IsObject())
			{
				return;
			}
			auto* pInputTensor = dynamic_cast<X::Data::Tensor*>(input.GetObj());
			if (pInputTensor == nullptr)
			{
				return;
			}
			auto* pNewTensor = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());
			if (pNewTensor == nullptr)
			{
				return;
			}
			pNewTensor->CreateBaseOnTensorWithPermute(pInputTensor, axes);
			int dimCnt = (int)pInputTensor->GetDimCount();
			std::vector<long long> orgial_indices(dimCnt);
			auto it_proc = [pNewTensor, pInputTensor, dimCnt, axes, &orgial_indices](std::vector<long long>& indices)
			{
				for (int i = 0; i < dimCnt; i++)
				{
					if (i < (int)axes.size())
					{
						orgial_indices[axes[i]] = indices[i];
					}
					else
					{
						orgial_indices[i] = indices[i];
					}
				}
				X::Value val = pInputTensor->GetDataWithIndices(orgial_indices);
				pNewTensor->SetDataWithIndices(indices, val);
			};
			pNewTensor->IterateAll(it_proc);
		}//permute

		bool IsNum(X::Value input) 
		{
			bool IsNum = false;
			auto ty = ((X::Value)input).GetType();
			auto val = 0;
			switch (ty)
			{
			case X::ValueType::Int64:
				IsNum = true;
				val = ((X::Value)input).GetLongLong();
				break;
			case X::ValueType::Double:
				IsNum = true;
				val = ((X::Value)input).GetDouble();
				break;
			default:  //todo, what about boolean?
				break;
			}
			return IsNum;
		}

		bool IsTensor(X::Value input) 
		{
			//std::cout << "In IsTensor()" << std::endl;
			bool IsTensor = false;
			auto ty = ((X::Value)input).GetType();
			if (ty == X::ValueType::Object) 
			{
				//std::cout << "In IsTensor(), is an object" << std::endl;
				auto* pObj = ((X::Value)input).GetObj();
				auto obj_ty = pObj->GetType();
				if (obj_ty == ObjType::Tensor || obj_ty == X::ObjType::TensorExpression) 
				{
					//std::cout << "In IsTensor(), is a tensor" << std::endl;
					IsTensor = true;
				}
				//else 
				//	std::cout << "In IsTensor(), is not a tensor" << std::endl;
			}
			//else 
			//	std::cout << "In IsTensor(), not an object" << std::endl;

			return IsTensor;		
		}
		inline std::tuple<bool, bool> IsNumAddable(X::Data::Tensor &t, const X::Value& operand) 
		{
			bool bAddable = false;
			bool IsNum = false;

			auto val = 0;
			auto ty = ((X::Value)operand).GetType();

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

			switch (t.GetDataType())
			{
			case X::TensorDataType::BOOL:
				break;
			case X::TensorDataType::BYTE:
				if (ty == X::ValueType::Int64 && val >= -128 && val <= 127) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::UBYTE:
				if (ty == X::ValueType::Int64 && val <= 255) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::SHORT:
				if (ty == X::ValueType::Int64 && val >= -32768 && val <= 32767) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::USHORT:
				if (ty == X::ValueType::Int64 && val <= 65535) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::INT:
				if (ty == X::ValueType::Int64 && val >= -2147483648 && val <= 2147483647) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::UINT:
				if (ty == X::ValueType::Int64 && val <= 4294967295)
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::LONGLONG:
				if (ty == X::ValueType::Int64 && val >= -9,223,372,036,854,775,808 && val <= 9,223,372,036,854,775,807) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::ULONGLONG:
				if (ty == X::ValueType::Int64 && val <= 18,446,744,073,709,551,615) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::HALFFLOAT:
				if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val <= 65535) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::FLOAT:
				if ((ty == X::ValueType::Double || ty == X::ValueType::Double) && val <= 4294967295) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::DOUBLE:
			case X::TensorDataType::CFLOAT:
				if (ty == X::ValueType::Double || ty == X::ValueType::Double) 
				{
					bAddable = true;
				}
				break;
			case X::TensorDataType::CDOUBLE:
				break;
			default:
				break;
			}
			return {bAddable, IsNum};
		}

		inline bool IsTensorAddableNew(X::Data::Tensor &t1, X::Data::Tensor &t2) 
		{
			int dim_count1 = t1.GetDimCount(); //T(4,5,6,7)  -- 4 dimensions, 4*5*6*7 elements
			int dim_count2 = t2.GetDimCount(); //T(6,7)      -- 2 dimensions, 6*7 elements
			int dim_size1, dim_size2;
			for (auto i = 0; i < MIN_VAL(dim_count1, dim_count2); i++) // i = 0, 1 
			{
				dim_size1 = t1.GetDimSize(dim_count1 - i - 1); // dims[3] = 7, dims[2] = 6
				dim_size2 = t2.GetDimSize(dim_count2 - i - 1); // dims[1] = 7, dims[0] = 6
				if (dim_size1 != dim_size2)
					return false;
			}	
			return true;
		}

		void Add(X::ARGS& params, X::KWARGS& kwParams,X::Value input1,X::Value input2,X::Value& retVal)
		{

			//if (!IsTensor(retVal)) {
			//	std::cout << "In tensor_cpu.h::Add(),returned" << std::endl;
		 	//	return;
			//}
			
			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			AutoLock(m_lock);

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				//std::cout << "In tensor_cpu.h::Add(), input2 is " << input2.ToString()<< std::endl;

				if (!IsNum(input2))	//the other must be a number
					return;
				X::Value& input = input2;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val += input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_add);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Value& input = input1;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val += input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_add);
			}
			else  //both tensors
			{
				X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				long long tot_element_count_1 = pTensor1->GetCount();
				long long tot_element_count_2 = pTensor2->GetCount();
				if (tot_element_count_1 < tot_element_count_2)//make sure T1 has more elements than T2
				{	
					X::Data::Tensor* temp_t = pTensor1;
					pTensor1 = pTensor2;
					pTensor2 = temp_t;
				}

				pRetVal->CreateBaseOnTensor(pTensor1);
				bAddable = IsTensorAddableNew(*pTensor1, *pTensor2);
				if (bAddable)
				{
					//X::Value val_1, val_2, val_ret;
					X::Value val_1, val_2;
					long long tot_element_count_1 = pTensor1->GetCount();
					long long tot_element_count_2 = pTensor2->GetCount();
					long long cur_element_count_1 = 0, cur_element_count_2 = 0; 

					while (cur_element_count_1 < tot_element_count_1)
					{
						if (cur_element_count_2 == tot_element_count_2) 
						{
							cur_element_count_2 = 0;
						}
						val_1 = pTensor1->GetDataWithOffset(cur_element_count_1*pTensor1->GetItemSize());
						val_2 = pTensor2->GetDataWithOffset(cur_element_count_2*pTensor2->GetItemSize());
						val_1 += val_2;
						pRetVal->SetDataWithOffset(cur_element_count_1*pTensor2->GetItemSize(), val_1);
						cur_element_count_1 ++;
						cur_element_count_2 ++;
					}
				} //bAddable
			} // both tensors
		}// Add

		void Minus(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			//if (!IsTensor(retVal)) {
			//	std::cout << "In tensor_cpu.h::Minus(),returned" << std::endl;
		 	//	return;
			//}
			
			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			AutoLock(m_lock);

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Value& input = input2;

				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_minus = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val += input;  //t1-10 is changed to t1+(-10)
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_minus);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Value& input = input1;

				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_minus = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = input;
					val -= pTensor->GetDataWithIndices(indices);
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_minus);
			}
			else  //both tensors
			{
				X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				long long tot_element_count_1 = pTensor1->GetCount();
				long long tot_element_count_2 = pTensor2->GetCount();
				if (tot_element_count_1 < tot_element_count_2)//make sure T1 has more elements than T2
				{	
					X::Data::Tensor* temp_t = pTensor1;
					pTensor1 = pTensor2;
					pTensor2 = temp_t;
				}

				pRetVal->CreateBaseOnTensor(pTensor1);
				bAddable = IsTensorAddableNew(*pTensor1, *pTensor2);
				if (bAddable)
				{
					//X::Value val_1, val_2, val_ret;
					X::Value val_1, val_2;
					long long tot_element_count_1 = pTensor1->GetCount();
					long long tot_element_count_2 = pTensor2->GetCount();
					long long cur_element_count_1 = 0, cur_element_count_2 = 0; 

					while (cur_element_count_1 < tot_element_count_1)
					{
						//if (cur_element_count_1 % tot_element_count_2 == 0) 
						if (cur_element_count_2 == tot_element_count_2) 
						{
							cur_element_count_2 = 0;
						}
						//std::cout << "In Minus(), current index1 ="<<cur_element_count_1<<", current index2 ="<<cur_element_count_2<< std::endl;
						val_1 = pTensor1->GetDataWithOffset(cur_element_count_1*pTensor1->GetItemSize());
						val_2 = pTensor2->GetDataWithOffset(cur_element_count_2*pTensor2->GetItemSize());
						val_1 -= val_2;
						pRetVal->SetDataWithOffset(cur_element_count_1*pTensor2->GetItemSize(), val_1);
						cur_element_count_1 ++;
						cur_element_count_2 ++;
					}
				} //bAddable
			}//both tensors
		} // Minus

		void Multiply(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			AutoLock(m_lock);

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				//std::cout << "In tensor_cpu.h::Multiply(), input2 is " << input2.ToString()<< std::endl;
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Value& input = input2;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val *= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_add);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Value& input = input1;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val *= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_add);
			}
			else  //both tensors
			{
				X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				long long tot_element_count_1 = pTensor1->GetCount();
				long long tot_element_count_2 = pTensor2->GetCount();
				if (tot_element_count_1 < tot_element_count_2)//make sure T1 has more elements than T2
				{	
					X::Data::Tensor* temp_t = pTensor1;
					pTensor1 = pTensor2;
					pTensor2 = temp_t;
					tot_element_count_1 = pTensor1->GetCount();
					tot_element_count_2 = pTensor2->GetCount();
				}
				pRetVal->CreateBaseOnTensor(pTensor1);
				bAddable = IsTensorAddableNew(*pTensor1, *pTensor2);
				if (bAddable)
				{
					//X::Value val_1, val_2, val_ret;
					X::Value val_1, val_2;
					long long cur_element_count_1 = 0, cur_element_count_2 = 0; 

					while (cur_element_count_1 < tot_element_count_1)
					{
						//if (cur_element_count_1 % tot_element_count_2 == 0) 
						if (cur_element_count_2 == tot_element_count_2) 
						{
							cur_element_count_2 = 0;
						}
						val_1 = pTensor1->GetDataWithOffset(cur_element_count_1*pTensor1->GetItemSize());
						val_2 = pTensor2->GetDataWithOffset(cur_element_count_2*pTensor2->GetItemSize());
						val_1 *= val_2;
						pRetVal->SetDataWithOffset(cur_element_count_1*pTensor2->GetItemSize(), val_1);
						cur_element_count_1 ++;
						cur_element_count_2 ++;
					}
				} //bAddable

			} // both tensors
		} //Multiply

		void Matmul(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
			X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
			X::Data::Tensor* pRetVal  = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		
			bool bMulable = IsProdTensor(*pTensor1, *pTensor2);
			if (bMulable)			
			{//tensor only, verified in IsAddable()
				auto it_proc_tensor_mul_matrix = [pTensor1, pTensor2, pRetVal]()
				{
					//Matrix1 (m,n), Matrix2 (u,v), n = u, after production, new Matrix shape (m,v)
					int m = pTensor1->GetDims()[0].size; //rows of matrix1
					int n = pTensor1->GetDims()[1].size; //columns of matrix1
					int u = pTensor2->GetDims()[0].size; //rows of matrix2
					int v = pTensor2->GetDims()[1].size; //columns of matrix2
					int i, j, k;
					X::Value val_1, val_2, val;
					std::vector<long long> indices1, indices2, indices;
					indices.resize(2);
					indices1.resize(2);
					indices2.resize(2);
					pRetVal->CreateBaseOnTensor(pTensor1);

					for ( i = 0; i < m; i++) {
						for (j = 0; j < v; j ++) {
							indices[0] = i;
							indices[1] = j;
							val = 0;
							for (k =0; k<n; k++) { //c(i,j) = a(i,0)*b(0,j)+ a(i,1)*b(1,j)+ ...+a(i,n-1)*b(n-1,j)
								indices1[0] = i;
								indices1[1] = k;
								indices2[0] = k;
								indices2[1] = j;
								val_1 = pTensor1->GetDataWithIndices(indices1);
								val_2 = pTensor2->GetDataWithIndices(indices2);								
								val_1 *= val_2;
								val += val_1;
							}
							pRetVal->SetDataWithIndices(indices, val);
						}
					}
				};
				auto it_proc_tensor_mul_vector = [pTensor1, pTensor2, pRetVal]()
				{
					int m = pTensor1->GetDims()[0].size; //rows of matrix
					int n = pTensor1->GetDims()[1].size; //columns of matrix
					int v = pTensor2->GetDims()[0].size; //vector
					if (n!=v)  //To do, error handling
						return;


					std::vector<int> dims;
					dims.push_back(m);
					TensorDataType dataType = pTensor1->GetDataType();
					pRetVal->SetDataType(dataType);
					pRetVal->CreateBaseOnShape(dims);

					int i, j;
					X::Value val_1, val_2, val;
					std::vector<long long> indices1, indices2, indices;
					indices.resize(1);
					indices1.resize(2);
					indices2.resize(1);
					for (i = 0; i < m; i ++) {
						indices[0] = i;
						val = 0;
						for ( j = 0; j < n; j++) {//Ret(i) = sigma {matrix[i,j]*Vector[j]}
							indices1[0] = i;
							indices1[1] = j;
							indices2[0] = j;
							val_1 = pTensor1->GetDataWithIndices(indices1);
							val_2 = pTensor2->GetDataWithIndices(indices2);								
							val_1 *= val_2;
							val += val_1;
						}
						pRetVal->SetDataWithIndices(indices, val);
					}
				};

				if (pTensor1->GetDimCount() == 2 && pTensor2->GetDimCount() == 2) //both are 2D matrixes 
					it_proc_tensor_mul_matrix();
				else if (pTensor1->GetDimCount() == 1 && pTensor2->GetDimCount() == 2)
					it_proc_tensor_mul_vector();
				else if (pTensor1->GetDimCount() == 2 && pTensor2->GetDimCount() == 1)
					it_proc_tensor_mul_vector();
				else
					std::cout<<"Tensor multiplication can't be performed"<< std::endl;
		

			}// matrix 

		} //matmul

		void Divide(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "in tensor_cpu.h::Divide()" << std::endl;

			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			AutoLock(m_lock);

			if (!IsTensor1 && !IsTensor2)
			{
				std::cout << "In tensor_cpu.h::Divide(), none is tensor, returned 2 =" << std::endl;
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				std::cout << "In tensor_cpu.h::Divide(), input1 is tensor, input2 is not a tensor" << std::endl;
				//std::cout << "In tensor_cpu.h::Multiply(), input2 is " << input2.ToString()<< std::endl;
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Value& input = input2;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_div = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val /= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_div);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				std::cout << "In tensor_cpu.h::Divide(), input1 is not a tensor, input2 is a tensor" << std::endl;
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Value& input = input1;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_div = [pTensor, input, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val /= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				//std::tie (bAddable, bIsNum) = IsNumAddable(*pTensor1, input2);
				//if (!bAddable)
				//	return;
				pTensor->IterateAll(it_proc_scaler_div);
			}
			else  //both tensors
			{
				X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				long long tot_element_count_1 = pTensor1->GetCount();
				long long tot_element_count_2 = pTensor2->GetCount();
				if (tot_element_count_1 < tot_element_count_2)//make sure T1 has more elements than T2
				{	
					X::Data::Tensor* temp_t = pTensor1;
					pTensor1 = pTensor2;
					pTensor2 = temp_t;
					tot_element_count_1 = pTensor1->GetCount();
					tot_element_count_2 = pTensor2->GetCount();
				}
				pRetVal->CreateBaseOnTensor(pTensor1);
				bAddable = IsTensorAddableNew(*pTensor1, *pTensor2);
				std::cout << "In tensor_cpu.h::Divide(), IsTensorAddableNew = " << bAddable << std::endl;
				if (bAddable)
				{
					//X::Value val_1, val_2, val_ret;
					X::Value val_1, val_2;
					long long cur_element_count_1 = 0, cur_element_count_2 = 0; 
					std::cout << "In Divide(), total elements in t1 ="<<tot_element_count_1<<", total elements in t2 ="<<tot_element_count_2<< std::endl;

					while (cur_element_count_1 < tot_element_count_1)
					{
						//if (cur_element_count_1 % tot_element_count_2 == 0) 
						if (cur_element_count_2 == tot_element_count_2) 
						{
							cur_element_count_2 = 0;
						}
						//std::cout << "In Divide(), current index1 ="<<cur_element_count_1<<", current index2 ="<<cur_element_count_2<< std::endl;
						val_1 = pTensor1->GetDataWithOffset(cur_element_count_1*pTensor1->GetItemSize());
						val_2 = pTensor2->GetDataWithOffset(cur_element_count_2*pTensor2->GetItemSize());
						//std::cout << "In Divide(), val1="<<val_1.GetLongLong()<<",val2 ="<<val_2.GetLongLong()<< std::endl;
						//val_ret = val_1.GetLongLong() / val_2.GetLongLong();
						//val_ret = val_1 / val_2;
						//std::cout << "In Divide(), new val1="<<val_ret.GetLongLong()<< std::endl;
						val_1 /= val_2;
						//val_ret = val_1;
						//pRetVal->SetDataWithOffset(cur_element_count_1*pTensor2->GetItemSize(), val_ret);
						pRetVal->SetDataWithOffset(cur_element_count_1*pTensor2->GetItemSize(), val_1);
						cur_element_count_1 ++;
						cur_element_count_2 ++;
					}
				} //bAddable

			} // both tensors
		} //Divide

		void Conv2d(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "in tensor_cpu.h::conv2d()" << std::endl;
			//we need the mode - full, same, valid
			
			X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());  
			X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj()); //core or filter
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			int m = pTensor1->GetDims()[0].size; //rows of matrix1
			int n = pTensor1->GetDims()[1].size; //columns of matrix1
			int u = pTensor2->GetDims()[0].size; //rows of matrix2
			int v = pTensor2->GetDims()[1].size; //columns of matrix2

			if ( m+u-1 < 0 || n+v-1 < 0)
				return;

			std::vector<int> dims;
			dims.push_back(m+u-1);
			dims.push_back(n+v-1);
			TensorDataType dataType = pTensor1->GetDataType();
			pRetVal->SetDataType(dataType);
			pRetVal->CreateBaseOnShape(dims);

			int i, j, k, l;
			X::Value val_1, val_2, val;
			std::vector<long long> indices1, indices2, indices;
	
			for ( i = 0; i < m+u-1; i++) {
				for (j = 0; j < n+v-1; j ++) {
					//indices[0] = i;
					//indices[1] = j;
					indices.push_back(i);
					indices.push_back(j);
					val = 0;
					for (k = 0; k < m; k++) { 
						for (l = 0; l < n; l++) { 
							if (i-k >=0 && i-k < u && j-l>=0 && j-l < v) 
							{
								//indices1[0] = k;
								//indices1[1] = l;
								//indices2[0] = i-k;
								//indices2[1] = j-l;
								indices1.push_back(k);
								indices1.push_back(l);
								indices2.push_back(i-k);
								indices2.push_back(j-l);
								val_1 = pTensor1->GetDataWithIndices(indices1);
								val_2 = pTensor2->GetDataWithIndices(indices2);								
								val_1 *= val_2;
								val += val_1;
							} //if
						} //for l
					} //for k
					pRetVal->SetDataWithIndices(indices, val);
				}//for j
			}//for i
		}

		void Relu(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "in tensor_cpu.h::Relu()" << std::endl;

		} //Relu


		void MaxPool2d(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "in tensor_cpu.h::MaxPool2d()" << std::endl;
			/*
			for (size_t y = 0; y < out_height; ++y) {
				for (size_t x = 0; x < out_width; ++x) {
					for (size_t i = 0; i < pool_y; ++i) {
						for (size_t j = 0; j < pool_x; ++j) {
							for (size_t c = 0; c < depth; ++c) {
								float value = in[y * pool_y + i][x * pool_x + j][c];
								out[y][x][c] = max(out[y][x][c], value);
							}
						}
					}
				}
			}
			*/

		}  // MaxPool2d

	}; //class CpuTensor
} //namespace X


