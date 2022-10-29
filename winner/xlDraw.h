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