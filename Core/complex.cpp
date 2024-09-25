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

#include "complex.h"
#include "function.h"
#include <string.h>
#include <iostream>

namespace X
{
	namespace Data
	{
		Complex::Complex():XComplex(0),Object(){
			m_t = ObjType::Complex;
			m_real = 0;
			m_imaginary = 0;
		}
		Complex::Complex(double real, double imaginary):XComplex(0),Object(){
			m_t = ObjType::Complex;
			m_real = real;
			m_imaginary = imaginary;
		}

		Complex::Complex(Complex& val):XComplex(0),Object() {
			m_t = ObjType::Complex;
			m_real = val.m_real;
			m_imaginary = val.m_imaginary;
		}
		Complex& Complex::operator+=(X::Value& val){
			AutoLock autoLock(m_lock);
			if (val.IsObject())
			{
				auto* pObj = val.GetObj();
				if (pObj->GetType() == ObjType::Complex)
				{
					Complex* pComp = dynamic_cast<Complex*> (pObj);
					m_real += pComp->m_real;
					m_imaginary += pComp->m_imaginary;
				}
				else {
					//todo
				}
			}
			else {
				m_real += (double)val;
			}
			return *this;
		} 
		Complex& Complex::operator-=(X::Value& val){
			AutoLock autoLock(m_lock);
			if (val.IsObject())
			{
				auto* pObj = val.GetObj();
				if (pObj->GetType() == ObjType::Complex)
				{
					Complex* pComp = dynamic_cast<Complex*> (pObj);
					m_real -= pComp->m_real;
					m_imaginary -= pComp->m_imaginary;
				}
				else {
					//todo
				}
			}
			else {
				m_real -= (double)val;
			}
			return *this;
		} 
		Complex& Complex::operator*=(X::Value& val){
			AutoLock autoLock(m_lock);
			if (val.IsObject())
			{
				auto* pObj = val.GetObj();
				if (pObj->GetType() == ObjType::Complex)
				{
					Complex* pComp = dynamic_cast<Complex*> (pObj);
					double temp_real = m_real * pComp->m_real - m_imaginary * pComp->m_imaginary;
					double temp_imaginary = m_real * pComp->m_imaginary + m_imaginary * pComp->m_imaginary;
					m_real = temp_real;
					m_imaginary = temp_imaginary;
				}
				else {
					//todo
				}
			}
			else {
				double temp_val = (double) val;
				m_real = m_real * temp_val;
				m_imaginary = m_imaginary * temp_val;
			}
			return *this;
		} 

		Complex& Complex::operator/=(X::Value& val){
			AutoLock autoLock(m_lock);
			if (val.IsObject())
			{
				auto* pObj = val.GetObj();
				if (pObj->GetType() == ObjType::Complex)
				{
					Complex* pComp = dynamic_cast<Complex*> (pObj);
					double u = m_real;
					double v = m_imaginary;
					double x = pComp->m_real;
					double y = pComp->m_imaginary;
					if (x == 0 && y ==0) {
						//todo
					}
					else {
						m_real = (u*x+v*y)/(x*x+y*y);
						m_imaginary = (v*x-u*y)/(x*x+y*y);
				    }
				}
				else{
					//todo
				}
			}
			else {
				double temp_val = (double) val;
				if (temp_val != 0) {
					m_real = m_real / temp_val;
					m_imaginary = m_imaginary/temp_val;
				}
				else {
					//todo
				}
			}
			return *this;
		}		
		const char* Complex::ToString(bool WithFormat)
		{ 		
			std::string strOut;
			if (m_real != 0) {
				strOut += std::to_string(m_real);
			}
			if (m_imaginary !=0) {
				if (m_imaginary > 0)
				    strOut += "+";
				strOut += std::to_string(m_imaginary);
				strOut += "j";
			}
			else if (m_real== 0) {
				strOut = "0";				
			}
			return GetABIString(strOut);
		}
	}
}