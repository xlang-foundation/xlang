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

#pragma once
#include "object.h"
namespace X
{
	namespace Data
	{
		class Complex :
			virtual public XComplex,
			virtual public Object
		{
		private:
			double m_real;
			double m_imaginary;	
		public:
			Complex();
			Complex(double real, double imaginary);
			Complex(Complex& val);
			~Complex()
			{
			}
			virtual XObj* Clone() override
			{
				auto* newObj = new Complex(m_real, m_imaginary);
				newObj->IncRef();
				return newObj;
			}
			virtual Complex& operator+=( X::Value& val) override;
			virtual Complex& operator-=( X::Value& val) override;
			virtual Complex& operator*=( X::Value& val) override;
			virtual Complex& operator/=( X::Value& val) override;
			virtual const char* ToString(bool WithFormat = false) override;
		};
	}
}
