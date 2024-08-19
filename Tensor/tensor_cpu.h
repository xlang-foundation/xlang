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
#include "port.h"

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
		FORCE_INLINE std::tuple<bool, bool> IsNumAddable(X::Data::Tensor &t, const X::Value& operand) 
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

		FORCE_INLINE bool IsTensorAddableNew(X::Data::Tensor &t1, X::Data::Tensor &t2) 
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
			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input2, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val += input2;
					pRetVal->SetDataWithIndices(indices, val);
				};
				pTensor->IterateAll(it_proc_scaler_add);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				if (!IsNum(input1))	//the other must be a number
					return;

				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input1, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val += input1;
					pRetVal->SetDataWithIndices(indices, val);
				};
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
					/*
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
					*/
					int leftDimCount = pTensor1->GetDimCount() - pTensor2->GetDimCount();
					int tensor1_dims = pTensor1->GetDimCount();

					auto it_proc_tensor_add = [pTensor1, pTensor2, tensor1_dims, leftDimCount, pRetVal](std::vector<long long>& indices1)
					{
						auto it_proc_tensor_add_a = [pTensor1, pTensor2, pRetVal, leftDimCount, &indices1](std::vector<long long>& indices2) 
						{
							X::Value val, val_1, val_2;
							std::cout << "	";
							for (int i = 0; i < indices2.size(); i++) 
								indices1[leftDimCount+i] = indices2[i];

							val_1 = pTensor1->GetDataWithIndices(indices1);
							val_2 = pTensor2->GetDataWithIndices(indices2);
							val_1 += val_2;
							pRetVal->SetDataWithIndices(indices1, val_1);
						};
						indices1.resize(tensor1_dims);
						pTensor2->IterateAll(it_proc_tensor_add_a);
					};
					auto it_proc_tensor_add_b = [pTensor1, pTensor2, pRetVal](std::vector<long long>& indices) 
					{
						X::Value val, val_1, val_2;
						val_1 = pTensor1->GetDataWithIndices(indices);
						val_2 = pTensor2->GetDataWithIndices(indices);
						val_1 += val_2;
						pRetVal->SetDataWithIndices(indices, val_1);
					};

					if (leftDimCount > 0) 
						pTensor1->IterateLeft(it_proc_tensor_add, leftDimCount);
					else 
						pTensor1->IterateAll(it_proc_tensor_add_b);


				} //bAddable
			} // both tensors
		}// Add

		void Minus(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_minus = [pTensor, input2, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val += input2;  //t1-10 is changed to t1+(-10)
					pRetVal->SetDataWithIndices(indices, val);
				};
				pTensor->IterateAll(it_proc_scaler_minus);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());

				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_minus = [pTensor, input1, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = input1;
					val -= pTensor->GetDataWithIndices(indices);
					pRetVal->SetDataWithIndices(indices, val);
				};
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
					/*
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
					*/
					int leftDimCount = pTensor1->GetDimCount() - pTensor2->GetDimCount();
					int tensor1_dims = pTensor1->GetDimCount();

					auto it_proc_tensor_minus = [pTensor1, pTensor2, tensor1_dims, leftDimCount, pRetVal](std::vector<long long>& indices1)
					{
						auto it_proc_tensor_minus_a = [pTensor1, pTensor2, pRetVal, leftDimCount, &indices1](std::vector<long long>& indices2) 
						{
							X::Value val, val_1, val_2;
							std::cout << "	";
							for (int i = 0; i < indices2.size(); i++) 
								indices1[leftDimCount+i] = indices2[i];

							val_1 = pTensor1->GetDataWithIndices(indices1);
							val_2 = pTensor2->GetDataWithIndices(indices2);
							val_1 -= val_2;
							pRetVal->SetDataWithIndices(indices1, val_1);
						};
						indices1.resize(tensor1_dims);
						pTensor2->IterateAll(it_proc_tensor_minus_a);
					};
					auto it_proc_tensor_minus_b = [pTensor1, pTensor2, pRetVal](std::vector<long long>& indices) 
					{
						X::Value val, val_1, val_2;
						val_1 = pTensor1->GetDataWithIndices(indices);
						val_2 = pTensor2->GetDataWithIndices(indices);
						val_1 -= val_2;
						pRetVal->SetDataWithIndices(indices, val_1);
					};

					if (leftDimCount > 0) 
						pTensor1->IterateLeft(it_proc_tensor_minus, leftDimCount);
					else 
						pTensor1->IterateAll(it_proc_tensor_minus_b);

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

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input2, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val *= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				pTensor->IterateAll(it_proc_scaler_add);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_add = [pTensor, input1, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val *= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
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
					/*
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
					*/
					int leftDimCount = pTensor1->GetDimCount() - pTensor2->GetDimCount();
					int tensor1_dims = pTensor1->GetDimCount();

					auto it_proc_tensor_mul = [pTensor1, pTensor2, tensor1_dims, leftDimCount, pRetVal](std::vector<long long>& indices1)
					{
						auto it_proc_tensor_mul_a = [pTensor1, pTensor2, pRetVal, leftDimCount, &indices1](std::vector<long long>& indices2) 
						{
							X::Value val, val_1, val_2;
							std::cout << "	";
							for (int i = 0; i < indices2.size(); i++) 
								indices1[leftDimCount+i] = indices2[i];

							val_1 = pTensor1->GetDataWithIndices(indices1);
							val_2 = pTensor2->GetDataWithIndices(indices2);
							val_1 = val_1 * val_2;
							pRetVal->SetDataWithIndices(indices1, val_1);
						};
						indices1.resize(tensor1_dims);
						pTensor2->IterateAll(it_proc_tensor_mul_a);
					};
					auto it_proc_tensor_mul_b = [pTensor1, pTensor2, pRetVal](std::vector<long long>& indices) 
					{
						X::Value val, val_1, val_2;
						val_1 = pTensor1->GetDataWithIndices(indices);
						val_2 = pTensor2->GetDataWithIndices(indices);
						val_1 = val_1 * val_2;
						pRetVal->SetDataWithIndices(indices, val_1);
					};

					if (leftDimCount > 0) 
						pTensor1->IterateLeft(it_proc_tensor_mul, leftDimCount);
					else 
						pTensor1->IterateAll(it_proc_tensor_mul_b);

				} //bAddable

			} // both tensors
		} //Multiply

		void Divide(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			bool IsTensor1 = IsTensor (input1);
			bool IsTensor2 = IsTensor (input2);
			bool bAddable =false;
			bool bIsNum = false;
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			if (!IsTensor1 && !IsTensor2)
			{
				return;  //todo, error handling
			}
			else if (IsTensor1 && !IsTensor2)//if input1 is a tensor, input2 is not a tensor
			{
				if (!IsNum(input2))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input1.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_div = [pTensor, input2, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = pTensor->GetDataWithIndices(indices);
					val /= input;
					pRetVal->SetDataWithIndices(indices, val);
				};
				pTensor->IterateAll(it_proc_scaler_div);
			}
			else if (!IsTensor1 && IsTensor2) {//if input2 is a tensor, input1 is not a tensor
				std::cout << "In tensor_cpu.h::Divide(), input1 is not a tensor, input2 is a tensor" << std::endl;
				if (!IsNum(input1))	//the other must be a number
					return;
				X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input2.GetObj());
				pRetVal->CreateBaseOnTensor(pTensor);
				auto it_proc_scaler_div = [pTensor, input1, pRetVal](std::vector<long long>& indices)
				{
					X::Value val = input;
					val /= (pTensor->GetDataWithIndices(indices));
					pRetVal->SetDataWithIndices(indices, val);
				};
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

					/*
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
						val_1 = pTensor1->GetDataWithOffset(cur_element_count_1*pTensor1->GetItemSize());
						val_2 = pTensor2->GetDataWithOffset(cur_element_count_2*pTensor2->GetItemSize());
						val_1 /= val_2;
						pRetVal->SetDataWithOffset(cur_element_count_1*pTensor2->GetItemSize(), val_1);
						cur_element_count_1 ++;
						cur_element_count_2 ++;
					}
					*/
					int leftDimCount = pTensor1->GetDimCount() - pTensor2->GetDimCount();
					int tensor1_dims = pTensor1->GetDimCount();

					auto it_proc_tensor_div = [pTensor1, pTensor2, tensor1_dims, leftDimCount, pRetVal](std::vector<long long>& indices1)
					{
						auto it_proc_tensor_div_a = [pTensor1, pTensor2, pRetVal, leftDimCount, &indices1](std::vector<long long>& indices2) 
						{
							X::Value val, val_1, val_2;
							for (int i = 0; i < indices2.size(); i++) 
								indices1[leftDimCount+i] = indices2[i];

							val_1 = pTensor1->GetDataWithIndices(indices1);
							val_2 = pTensor2->GetDataWithIndices(indices2);
							val_1 = val_1/val_2;
							pRetVal->SetDataWithIndices(indices1, val_1);
						};
						indices1.resize(tensor1_dims);
						pTensor2->IterateAll(it_proc_tensor_div_a);
					};
					auto it_proc_tensor_div_b = [pTensor1, pTensor2, pRetVal](std::vector<long long>& indices) 
					{
						X::Value val, val_1, val_2;
						val_1 = pTensor1->GetDataWithIndices(indices);
						val_2 = pTensor2->GetDataWithIndices(indices);
						val_1 = val_1/val_2;
						pRetVal->SetDataWithIndices(indices, val_1);
					};

					if (leftDimCount > 0) 
						pTensor1->IterateLeft(it_proc_tensor_div, leftDimCount);
					else 
						pTensor1->IterateAll(it_proc_tensor_div_b);				
				} //bAddable
			} // both tensors
		} //Divide

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
								val_1 = val_1 * val_2;
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
							val_1 = val_1 * val_2;
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

		void Conv2d_old(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			
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
			indices.resize(2);
			indices1.resize(2);
			indices2.resize(2);

			for ( i = 0; i < m+u-1; i++) {
				for (j = 0; j < n+v-1; j ++) {
					indices[0] = i;
					indices[1] = j;
					val = 0;
					for (k = 0; k < m; k++) { 
						for (l = 0; l < n; l++) { 
							if (i-k >=0 && i-k < u && j-l>=0 && j-l < v) 
							{
								indices1[0] = k;
								indices1[1] = l;
								indices2[0] = i-k;
								indices2[1] = j-l;
								val_1 = pTensor1->GetDataWithIndices(indices1);
								val_2 = pTensor2->GetDataWithIndices(indices2);								
								val_1 = val_1 * val_2;
								val += val_1;
							} //if
						} //for l
					} //for k
					pRetVal->SetDataWithIndices(indices, val);
				}//for j
			}//for i
		}
		void Transpose(X::Data::Tensor* pTensor, X::Data::Tensor* pTensorTran, long long u, long long v)
		{
			//X::Data::Tensor* pTensor = dynamic_cast<X::Data::Tensor*>(input.GetObj());  
			//X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		
			auto it_proc_tensor_transpose = [pTensor, pTensorTran, u, v](std::vector<long long>& indices2) 
			{
				X::Value val_2;
				val_2 = pTensor->GetDataWithIndices(indices2);
				std::cout << "input index (" << indices2[0] << "," << indices2[1] <<"), val =" << val_2.GetLongLong();
				std::vector<long long> indices;
				indices.resize(2);
				indices[0] = u - 1 - indices2[0];
				indices[1] = v - 1 - indices2[1];
				std::cout << ", output index (" << indices[0] << "," << indices[1] <<")" ;
				std::cout << ", val = " << val_2.GetLongLong() << std::endl ;
				pTensorTran->SetDataWithIndices(indices, val_2);
			};
			std::cout << "input u = " << u << ", v = " << v << std::endl;
			pTensor->IterateAll(it_proc_tensor_transpose);
		}
		void Conv2d_old2(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "in tensor_cpu.h::conv2d()" << std::endl;
			
			auto input_channels = kwParams.find("input_channels");  
			auto output_channels = kwParams.find("output_channels");  
			auto stride = kwParams.find("stride");  
			auto dilation = kwParams.find("dilation");  
			auto conv_mode = kwParams.find("conv_mode");  //- full, same, valid
			auto padding = kwParams.find("padding");
			auto padding_mode = kwParams.find("padding_mode");
			auto bias = kwParams.find("bias");

			X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());  
			X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj()); //core or filter
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			/*
			X::Value input_matrix, weight_matrix;
			std::vector<Data::TensorIndex> IdxAry;
			IdxAry.push_back({0,1});  //matrix only
			pTensor1->Get(IdxAry, input_matrix);
			pTensor2->Get(IdxAry, weight_matrix);
			*/


			int m = pTensor1->GetDims()[0].size; //rows of matrix1
			int n = pTensor1->GetDims()[1].size; //columns of matrix1
			int u = pTensor2->GetDims()[0].size; //rows of matrix1
			int v = pTensor2->GetDims()[1].size; //columns of matrix1

			std::vector<int> dims;
			dims.push_back(m);
			dims.push_back(n);
			TensorDataType dataType = pTensor1->GetDataType();
			pRetVal->CreateBaseOnShape(dims);
			pRetVal->SetDataType(dataType);

			//X::Data::Tensor* pTensor2t = new X::Data::Tensor(*pTensor2); 
			X::Data::Tensor* pTensor2t = new X::Data::Tensor(); 
			TensorDataType dataType2 = pTensor2->GetDataType();
			pTensor2t->CreateBaseOnTensor(pTensor2);
			pTensor2t->SetDataType(dataType2);
			Transpose(pTensor2, pTensor2t, u, v); 

			Conv2d_internal(pTensor1, pTensor2t, pRetVal, /*(int)padding*/ 0);

		}

		void Conv2d_internal(X::Data::Tensor* pTensor1 , X::Data::Tensor* pTensor2, X::Data::Tensor* pRetVal, int padding = 0)
		{

			std::cout << "in tensor_cpu.h::Conv2d_internal()" << std::endl;

			padding = 0;	
			int m = pTensor1->GetDims()[0].size; //rows of matrix1
			int n = pTensor1->GetDims()[1].size; //columns of matrix1
			int u = pTensor2->GetDims()[0].size; //rows of matrix2
			int v = pTensor2->GetDims()[1].size; //columns of matrix2

			//if ( m+u-1 < 0 || n+v-1 < 0)
			//	return;

			std::vector<int> dims;
			//dims.push_back(m+u-1);  // full
			//dims.push_back(n+v-1);
			dims.push_back(m); //same
			dims.push_back(n);
			TensorDataType dataType = pTensor1->GetDataType();
			pRetVal->SetDataType(dataType);
			pRetVal->CreateBaseOnShape(dims);

			/*
			int i, j, k, l;
			X::Value val_1, val_2, val;
			std::vector<long long> indices1, indices2, indices;
			indices.resize(2);
			indices1.resize(2);
			indices2.resize(2);
			*/

			/*			method 1, full conv
			for ( i = 0; i < m+u-1; i++) {
				for (j = 0; j < n+v-1; j ++) {
					indices[0] = i;
					indices[1] = j;
					val = 0;  //C(i,j) = Sigma(k) Sigma (l) Matrix1(k,l)*Matrix2(i-k+1, j-l+1)
					for (k = 0; k < m; k++) { 
						for (l = 0; l < n; l++) { 
							if (i-k >=0 && i-k < u && j-l>=0 && j-l < v) 
							{
								indices1[0] = k;
								indices1[1] = l;
								indices2[0] = i-k;
								indices2[1] = j-l;
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
			*/

			//  method 2, same conv
			/*
			for ( i = 0; i < m; i++) 
			{
				for (j = 0; j < n; j ++) 
				{
					indices[0] = i;
					indices[1] = j;
					val = 0;  //C(i,j) = Sigma(k) Sigma (l) Matrix1(k,l)*Matrix2(i-k+1, j-l+1)
					 		  //C(i,j) = Sigma(k) Sigma (l) Matrix1(k,l)*Matrix2(i-k, j-l)  // k,l from -1 
					for (k = 0; k < u; k++) 
					{ 
						for (l = 0; l < v; l++) 
						{ 
							std::cout << "i =" << i << ", j = " <<j;
							std::cout << ", k = " << k << ", l = " <<l;
							if (i-k+1 >= 0 && i-k+1 < m && j-l+1 >=0 && j-l+1 < n) 
							{
								indices1[0] = i-k+1;
								indices1[1] = j-l+1;
								val_1 = pTensor1->GetDataWithIndices(indices1);
							}
							else 
								val_1 = padding;
							std::cout << ", i-k+1 =" << i-k+1 << ", j-l+1 = " << j-l+1;
							indices2[0] = k;
							indices2[1] = l;
							val_2 = pTensor2->GetDataWithIndices(indices2);	
							std::cout << ", val_1 = " << val_1.GetLongLong() << ", val_2 = " << val_2.GetLongLong();							
							val_1 *= val_2;
							std::cout << ", val_1*val_2 = " << val_1.GetLongLong();							
							val += val_1;
							std::cout << ", val = " << val.GetLongLong() <<std::endl;							
							
						} //for l
					} //for k
					pRetVal->SetDataWithIndices(indices, val);
					std::cout << "                           " <<"i  =" << i << ", j = " <<j << ", val = " << val.GetLongLong() <<std::endl;							
				}//for j
			}//for i
			*/

			auto it_proc_tensor_conv2d = [pTensor1, pTensor2, pRetVal, m,n,u,v, padding](std::vector<long long>& indices)
			{
				std::cout << "      m =" << m << ", n = " << n <<", u = " << u <<", v = " << v << std::endl;
				X::Value val;
				auto it_proc_tensor_conv2d_a = [pTensor1, pTensor2, pRetVal, indices, m,n,u,v, padding, &val](std::vector<long long>& indices2) 
				{
					X::Value val_1, val_2;
					val_2 = pTensor2->GetDataWithIndices(indices2);
					std::cout << "Weight matrix(" << indices2[0] << "," << indices2[1] <<") = " << val_2.GetLongLong() << "  |  ";
					std::vector<long long> indices1;
					indices1.resize(2);
					indices1[0] = indices[0] + indices2[0] - (u/2);
					indices1[1] = indices[1] + indices2[1] - (v/2);
					if (indices1[0] >= 0 && indices1[1] >= 0 && indices1[0] < m && indices1[1] < n)
						val_1 = pTensor1->GetDataWithIndices(indices1);
					else
						val_1 = padding; 
					std::cout << "Input matrix(" << indices1[0] << "," << indices1[1] <<") = " << val_1.GetLongLong() << "  |  ";
					val_1 = val_1 * val_2;
					val += val_1;
					std::cout << "val_1 *= val_2 = " << val_1.GetLongLong() << ", val = " << val.GetLongLong() << std::endl;

				};
				pTensor2->IterateAll(it_proc_tensor_conv2d_a);
				pRetVal->SetDataWithIndices(indices, val);
				std::cout << "       Output matrix(" << indices[0] << "," << indices[1] <<") = " << val.GetLongLong() << std::endl;

			};
			pRetVal->IterateAll(it_proc_tensor_conv2d);				
		}

		void Conv2d(X::ARGS& params, X::KWARGS& kwParams,X::Value input1, X::Value input2, X::Value& retVal)
		{
			std::cout << "in tensor_cpu.h::conv2d()" << std::endl;
			
			auto input_channels = kwParams.find("input_channels");  
			auto output_channels = kwParams.find("output_channels");  
			auto stride = kwParams.find("stride");  
			auto dilation = kwParams.find("dilation");  
			auto conv_mode = kwParams.find("conv_mode");  //- full, same, valid
			auto padding = kwParams.find("padding");
			auto padding_mode = kwParams.find("padding_mode");
			auto bias = kwParams.find("bias");

			X::Data::Tensor* pTensor1 = dynamic_cast<X::Data::Tensor*>(input1.GetObj());  
			X::Data::Tensor* pTensor2 = dynamic_cast<X::Data::Tensor*>(input2.GetObj()); //core or filter
			X::Data::Tensor* pRetVal = dynamic_cast<X::Data::Tensor*>(retVal.GetObj());		

			int tensor1_dims = pTensor1->GetDimCount();
			int tensor2_dims = pTensor2->GetDimCount();
			int leftDimCount = pTensor1->GetDimCount() - pTensor2->GetDimCount();

			int m,n,u,v = 0;
			if (tensor1_dims >= 2)
			{
				m = pTensor1->GetDims()[tensor1_dims - 2].size; //rows of matrix1
				n = pTensor1->GetDims()[tensor1_dims - 1].size; //columns of matrix1
			}
			if (tensor1_dims >= 2)
			{
				u = pTensor2->GetDims()[tensor2_dims - 2].size; //rows of matrix2
				v = pTensor2->GetDims()[tensor2_dims - 1].size; //columns of matrix2
			}
			
			//std::vector<int> dims;
			//dims.push_back(m);
			//dims.push_back(n);
			//TensorDataType dataType = pTensor1->GetDataType();
			//pRetVal->CreateBaseOnShape(dims);
			//pRetVal->SetDataType(dataType);
			pRetVal->CreateBaseOnTensor(pTensor1);

			// assume tensor2 is just a matrix for now
			//X::Data::Tensor* pTensor2t = new X::Data::Tensor(*pTensor2); 
			X::Data::Tensor* pTensor2t = new X::Data::Tensor(); 
			TensorDataType dataType2 = pTensor2->GetDataType();
			pTensor2t->CreateBaseOnTensor(pTensor2);
			pTensor2t->SetDataType(dataType2);
			Transpose(pTensor2, pTensor2t, u, v); 

			/*
			X::Value input_matrix, weight_matrix;
			std::vector<Data::TensorIndex> IdxAry;
			IdxAry.push_back({0,1});  //matrix only
			pTensor1->Get(IdxAry, input_matrix);
			pTensor2->Get(IdxAry, weight_matrix);
			*/

			auto it_proc_tensor_conv2d_a = [pTensor1, pTensor2t, pRetVal, m,n,u,v, padding, tensor1_dims, tensor2_dims](std::vector<long long>& indices1)
			{
				static int i =0;
				std::cout << "it_proc_tensor_conv2d_a called - " << i++ << std::endl;
				std::cout << "      m =" << m << ", n = " << n <<", u = " << u <<", v = " << v << std::endl;

				//std::vector<Data::TensorIndex> IdxAry;
				//IdxAry.push_back({i:i, m-2,m-1});  //matrix only
				//X::Value input_matrix;

				auto it_proc_tensor_conv2d_b = [pTensor1, pTensor2t, pRetVal, m,n,u,v, padding, tensor1_dims, tensor2_dims](std::vector<long long>& indices1)
				{
					static int j =0;
					std::cout << "it_proc_tensor_conv2d_b called - " << j++;
					for (int k = 0; k < tensor1_dims; k++)
						std::cout << ", indices1[" << k <<"] = " << indices1[k];
					std::cout << std::endl;
					X::Value val;
					auto it_proc_tensor_conv2d_c = [pTensor1, pTensor2t, indices1, m,n,u,v, padding, tensor1_dims, tensor2_dims, &val](std::vector<long long>& indices2) 
					{
						X::Value val_1, val_2;
						val_2 = pTensor2t->GetDataWithIndices(indices2);
						//std::cout << "Weight matrix(" << indices2[0] << "," << indices2[1] <<") = " << val_2.GetLongLong() << "  |  ";
						std::cout << "Weight matrix(" ;
						for (int k = 0; k < tensor2_dims; k++)
							std::cout << indices2[k] << ",";
						std::cout << ") = " << val_2.GetLongLong() << "  |  ";
						std::vector<long long> indices1n;	
						indices1n.resize(tensor1_dims);
						for (int i = 0; i < tensor1_dims - 2; i++)
							indices1n[i] = indices1[i];
						indices1n[tensor1_dims-2] = indices1[tensor1_dims-2] + indices2[tensor2_dims-2] - (u/2);
						indices1n[tensor1_dims-1] = indices1[tensor1_dims-1] + indices2[tensor2_dims-1] - (v/2);
						if (indices1n[0] >= 0 && indices1n[1] >= 0 && indices1n[0] < m && indices1n[1] < n)
					    	val_1 = pTensor1->GetDataWithIndices(indices1n);
						else
							val_1 = padding; 
						std::cout << "Input matrix(" ;
						for (int k = 0; k < tensor1_dims; k++)
							std::cout << indices1n[k] << ",";
						std::cout << ") = " << val_1.GetLongLong() << "  |  ";
						val_1 = val_1 * val_2;
						val += val_1;
						std::cout << "val_1 *= val_2 = " << val_1.GetLongLong() << ", val = " << val.GetLongLong() << std::endl;
					};
					pTensor2t->IterateAll(it_proc_tensor_conv2d_c);
					pRetVal->SetDataWithIndices(indices1, val);
					std::cout << "Output matrix(";
					for (int k = 0; k < tensor1_dims; k++)
						std::cout << "," << indices1[k];
					std::cout << ") = " << val.GetLongLong() << std::endl;
				};
				indices1.resize(tensor1_dims);
				pTensor1->IterateRight(it_proc_tensor_conv2d_b, indices1, 2);
			};

			auto it_proc_tensor_conv2d = [pTensor1, pTensor2,pTensor2t, pRetVal](std::vector<long long>& indices) 
			{
				X::Value val, val_1, val_2;
				val_1 = pTensor1->GetDataWithIndices(indices);
				val_2 = pTensor2->GetDataWithIndices(indices);
				val_1 = val_1/val_2;
				pRetVal->SetDataWithIndices(indices, val_1);
			};

			if (leftDimCount > 0) 
				pTensor1->IterateLeft(it_proc_tensor_conv2d_a, leftDimCount);
			else 
				pTensor1->IterateAll(it_proc_tensor_conv2d);				


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


