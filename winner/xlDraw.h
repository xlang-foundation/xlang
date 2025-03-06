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
#include "xpackage.h"
#include "xlang.h"

namespace XWin
{
	class Window;
	class Draw;
	class Color;
	class Image;
	class Brush
	{
		void* m_pObj = nullptr;
	public:
		template<typename T>
		T* Obj() { return (T*)m_pObj; }
		BEGIN_PACKAGE(Brush)
		END_PACKAGE
		Brush(Draw* pDraw, std::string color);
		~Brush();
	};
	class Color
	{
		void* m_pObj = nullptr;
	public:
		template<typename T>
		T* Obj() { return (T*)m_pObj; }
		BEGIN_PACKAGE(Color)
		END_PACKAGE
		Color(unsigned int rgb,float a = 1.0);
		Color(std::string color);
		~Color();
	};
	class Draw
	{
		void* m_pDrawInfo = nullptr;
	public:
		BEGIN_PACKAGE(Draw)
			APISET().AddClass<1, Brush, Draw>("Brush");
			APISET().AddClass<2, Color>("Color");
			APISET().AddFunc<0>("Begin", &Draw::Begin);
			APISET().AddFunc<0>("End", &Draw::End);
			APISET().AddFunc<1>("Clear", &Draw::Clear);
			APISET().AddFunc<5>("DrawRectangle", &Draw::DrawRectangle);
			APISET().AddFunc<5>("DrawImage", &Draw::DrawImage);
		END_PACKAGE
		void* GetDrawInfo() { return m_pDrawInfo; }
		Draw(Window* pWin);
		~Draw();
		bool Begin();
		bool End();
		bool DrawRectangle(int left, int top, int right, int bottom, Brush* pBrush);
		bool DrawImage(Image* pImg,int left, int top, int right, int bottom);
		bool Clear(std::string color);
	};
}