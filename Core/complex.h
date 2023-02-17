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
			virtual std::string ToString(bool WithFormat = false) override;
		};
	}
}
