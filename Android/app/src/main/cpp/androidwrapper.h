#pragma once

#include <jni.h>
#include <vector>
#include <unordered_map>
#include "value.h"
#include "xpackage.h"
#include "xlang.h"
#include "utility.h"
#include "Locker.h"

namespace X
{
	namespace Android {
		class AndroidWrapper;
		class App;
		class Page;
		class Color;

		class UIBase
		{
		protected:
			Page* m_page = nullptr;
			jobject m_object = nullptr;
			UIBase* m_parent = nullptr;
            BEGIN_PACKAGE(UIBase)
                APISET().AddFunc<1>("setBackgroundColor", &UIBase::setBackgroundColor);
				APISET().AddFunc<4>("setPadding", &UIBase::setPadding);
                APISET().AddFunc<4>("setMargins", &UIBase::setMargins);
				APISET().AddFunc<1>("setGravity", &UIBase::setGravity);
            END_PACKAGE
		public:
            inline jobject GetObject() {return m_object;}
			UIBase(Page* page)
			{
				m_page = page;
			}
			~UIBase();
			bool setGravity(int gravity);
			bool setPadding(int left, int top, int right, int bottom);
            bool setMargins(int left, int top, int right, int bottom);
			void SetParent(UIBase* p)
			{
				m_parent =p;
			}
            bool setBackgroundColor(Color* objColor);
        };
		class ViewGroup:
				public UIBase
		{
        protected:
			std::vector<UIBase*> m_kids;
			BEGIN_PACKAGE(ViewGroup)
                ADD_BASE(UIBase);
				APISET().AddFunc<1>("add", &ViewGroup::Add);
			END_PACKAGE
			ViewGroup(Page* page):
					UIBase(page)
			{
			}
			bool Add(UIBase* pElement);
		};
		class LinearLayout: public ViewGroup{
			BEGIN_PACKAGE(LinearLayout)
				ADD_BASE(ViewGroup);
			END_PACKAGE
			LinearLayout(Page* page);
		};

		class ConstraintLayout: public ViewGroup{
			BEGIN_PACKAGE(ConstraintLayout)
				ADD_BASE(ViewGroup);
			END_PACKAGE
			ConstraintLayout(Page* page):
				ViewGroup(page)
			{

			}
		};
		class ScrollView: public ViewGroup{
			BEGIN_PACKAGE(ScrollView)
					ADD_BASE(ViewGroup);
			END_PACKAGE
			ScrollView(Page* page);
		};
		//UI Element
		class UIElement:
				public UIBase
		{
		    BEGIN_PACKAGE(UIElement)
				ADD_BASE(UIBase);
                APISET().AddFunc<1>("setOnClickListener", &UIElement::setOnClickListener);
			END_PACKAGE
			UIElement(Page* page):
					UIBase(page)
			{

			}
            bool setOnClickListener(X::Value handler);
		};

		class TextView: public UIElement
		{
		    BEGIN_PACKAGE(TextView)
				ADD_BASE(UIElement);
                APISET().AddFunc<1>("setText", &TextView::setText);
                APISET().AddFunc<1>("setTextColor", &TextView::setTextColor);
                APISET().AddFunc<0>("getText", &TextView::getText);
                APISET().AddFunc<2>("setTextSize", &TextView::setTextSize);
			END_PACKAGE
			TextView(Page* page);
            bool setText(std::string txt);
            bool setTextColor(Color* objColor);
            std::string getText();
            bool setTextSize(int unit,float size);
		};
        class EditText: public UIElement
        {
            BEGIN_PACKAGE(UIElement)
                ADD_BASE(TextView);
            END_PACKAGE
            EditText(Page* page);
        };
		class Button: public TextView
		{
			BEGIN_PACKAGE(Button)
				ADD_BASE(TextView);
			END_PACKAGE
			Button(Page* page);
		};
        class LayoutParams
        {
            BEGIN_PACKAGE(LayoutParams)
            END_PACKAGE
        };
        class Color
        {
		protected:
			int _color=0;
            BEGIN_PACKAGE(Color)
            END_PACKAGE
            Color(X::ARGS& params, X::KWARGS& kwParams);
            int getColor(std::string& name)
            {
                if(name =="BLACK") return 16777216;
                else if(name =="BLUE") return -16776961;
                else if(name =="CYAN") return -16711681;
                else if(name =="GRAY") return -7829368;
                else if(name =="GREEN") return -16711936;
                else if(name =="LTGRAY") return -3355444;
                else if(name =="MAGENTA") return -65281;
                else if(name =="RED") return -65536;
                else if(name =="TRANSPARENT") return 0;
                else if(name =="WHITE") return -1;
                else if(name =="YELLOW") return -256;
                else return -16777216;//black
            }
            int getColor(float R,float G,float B,float A)
            {
                int r = int(255*R);
                int g = int(255*G);
                int b = int(255*B);
                int a = int(255*A);
                //argb
                return (a<<24)|(r<<16)|(g<<8)|b;
            }
			int getColor()
			{
				return _color;
			}
        };
		class Page: public ViewGroup {
			App* m_app = nullptr;
			BEGIN_PACKAGE(Page)
				ADD_BASE(ViewGroup);
				APISET().AddClass<0, LinearLayout,Page>("LinearLayout");
                APISET().AddClass<0, ScrollView,Page>("ScrollView");
				APISET().AddClass<0, ConstraintLayout,Page>("ConstraintLayout");
				APISET().AddClass<0, TextView,Page>("TextView");
                APISET().AddClass<0, EditText,Page>("EditText");
				APISET().AddClass<0, Button,Page>("Button");
			END_PACKAGE
			bool Create();
			App* GetApp()
			{
				return m_app;
			}
			jobject CreateLinearLayout();
            jobject CreateScrollView();
			jobject CreateTextView(std::string txt);
            jobject CreateEditText(std::string txt);
			jobject CreateButton(std::string txt);
			Page(App* pParent): ViewGroup(nullptr)
			{
				m_app = pParent;
				m_page = this;//point to self
                Create();
			}
		};
		class App {
			AndroidWrapper* m_parent = nullptr;
			BEGIN_PACKAGE(App)
				APISET().AddVarClass<Color>("Color","Color");
				APISET().AddClass<0, Page,App>("Page");
                APISET().AddFunc<1>("showPage", &App::ShowPage);
			END_PACKAGE
			AndroidWrapper* GetParent()
			{
				return m_parent;
			}
			App()
			{

			}
			App(AndroidWrapper* pParent)
			{
				m_parent = pParent;
			}
            bool ShowPage(Page* pPage);
		};


		class AndroidWrapper {
            JavaVM* m_jvm = nullptr;
			JNIEnv *m_env = nullptr;
			Locker m_mapLock;
            jobject m_objHost = nullptr;
			std::unordered_map<unsigned  int,jobject> m_objHostMap;
			App* m_app;
            Locker m_uiCallLock;
            std::vector<X::Value> m_uiCalls;
		public:
		BEGIN_PACKAGE(AndroidWrapper)
                APISET().AddClass<0, App>("App");
				APISET().AddProp("app", &AndroidWrapper::GetApp);
				APISET().AddFunc<1>("print", &AndroidWrapper::Print);
		END_PACKAGE

		public:
			AndroidWrapper() {
				m_app = new App(this);
			}
            void SetJVM(JavaVM* jvm)
            {
                m_jvm = jvm;
            }
			AndroidWrapper(JNIEnv *env, jobject objHost):
					AndroidWrapper(){
				m_env = env;
				SetHostPerThread(objHost);
			}
            void AddUICall(X::Value& c)
            {
                m_uiCallLock.Lock();
                m_uiCalls.push_back(c);
                m_uiCallLock.Unlock();
				PostCallToJavaUIThread();
            }
			void Init();
			bool PostCallToJavaUIThread();
			void CallFromUIThread();
			~AndroidWrapper() {
				if (m_env != nullptr){
					for(auto& it:m_objHostMap) {
						m_env->DeleteGlobalRef(it.second);
					}
				}
				if(m_app){
					delete m_app;
				}
			}
			void AddPlugins();
			void SetHostPerThread(jobject h)
			{
                m_objHost = m_env->NewGlobalRef(h);
                return;
				AutoLock(m_mapLock);
				unsigned long tid = GetThreadID();
				auto it = m_objHostMap.find(tid);
				if(it !=m_objHostMap.end())
				{
					m_env->DeleteGlobalRef(it->second);
				}
				jobject newRef = m_env->NewGlobalRef(h);
				m_objHostMap.emplace(std::make_pair(tid,newRef));
			}
			X::Value GetApp() {
				X::XPackageValue<App> valApp(m_app);
				return valApp;
			}
			JNIEnv* GetEnv() {return m_env;}
			jobject GetHost()
			{
                return m_objHost;
				AutoLock(m_mapLock);
				unsigned long tid = GetThreadID();
				auto it = m_objHostMap.find(tid);
				if(it !=m_objHostMap.end())
				{
					return it->second;
				}
				else
				{
					return nullptr;
				}
			}
			bool Print(std::string info);
		};
	}
}