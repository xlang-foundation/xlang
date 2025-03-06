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
#include "exp.h"
#include <vector>
//#include <stack>
#include <stdexcept>

#if !defined(FORCE_INLINE)
#if defined(_MSC_VER)
// Microsoft Visual C++ Compiler
#define FORCE_INLINE __forceinline
#elif defined(BARE_METAL)
#define FORCE_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
// GCC or Clang Compiler
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
// Fallback for other compilers
#define FORCE_INLINE inline
#endif
#endif

namespace X
{
	namespace Exp
	{
        template <typename T>
        class Stack {
        private:
            T* elements;
            int capacity;
            int topIndex;

            // Utility function to resize the stack
            FORCE_INLINE void resize(int newCapacity) {
                T* newElements = new T[newCapacity];
                for (int i = 0; i < topIndex; ++i) {
                    newElements[i] = elements[i];
                }
                delete[] elements;
                elements = newElements;
                capacity = newCapacity;
            }

        public:
            Stack() : elements(new T[100]), capacity(100), topIndex(0) {}

            ~Stack() {
                delete[] elements;
            }

            // Check if the stack is empty
            FORCE_INLINE bool empty() const {
                return topIndex == 0;
            }

            // Add an element to the top of the stack
            FORCE_INLINE void push(const T& item) {
                if (topIndex == capacity) {
                    resize(capacity * 2); // Double the capacity if full
                }
                elements[topIndex++] = item;
            }

            // View the top element of the stack
            FORCE_INLINE T& top() {
                if (empty()) {
                    #if defined(BARE_METAL)
                        static T t;
                        return t;
                    #else
                        throw std::out_of_range("Stack<>::top(): empty stack");
                    #endif
                }
                return elements[topIndex - 1];
            }

            // Remove the top element of the stack
            FORCE_INLINE void pop() {
                //if (empty()) {
                //    throw std::out_of_range("Stack<>::pop(): empty stack");
                //}
                topIndex--;
                //if (topIndex > 0 && topIndex == capacity / 4) {
                //    resize(capacity / 2); // Reduce the capacity if too many empty spaces
                //}
            }
        };


        template <typename T>
        class Stack2
        {
        private:
            std::vector<T> elements;

        public:
            // Check if the stack is empty
            inline bool empty() const 
            {
                return elements.empty();
            }

            // Add an element to the top of the stack
            inline void push(const T& item) 
            {
                elements.push_back(item);
            }

            // View the top element of the stack
            inline T& top() 
            {
                if (empty()) 
                {
                    throw std::out_of_range("Stack<>::top(): empty stack");
                }
                return elements.back();
            }

            // Remove the top element of the stack
            inline void pop() 
            {
                if (empty()) 
                {
                    throw std::out_of_range("Stack<>::pop(): empty stack");
                }
                elements.pop_back();
            }
        };

		//Value+LValue+Expresion, triple states to hold the result of expression
		struct ExpValue
		{
			AST::Expression* exp;
			X::Value v;
			X::LValue lv;
		};
        struct Pair
        {
            AST::Expression* first;
            bool second;
        };
		typedef Stack<Pair> ExpresionStack;
		typedef Stack<ExpValue> ValueStack;

		enum class Action
		{
			ExpStackPop,
			Continue,
		};
		bool ExpExec(AST::Expression* pExp,
			XlangRuntime* rt,
			AST::ExecAction& action,
			XObj* pContext,
			Value& v,
			LValue* lValue=nullptr);
	}
}