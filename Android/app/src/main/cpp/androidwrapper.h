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

		class UIBase
		{
		protected:
			Page* m_page = nullptr;
			jobject m_object = nullptr;
			UIBase* m_parent = nullptr;
            BEGIN_PACKAGE(UIBase)
            END_PACKAGE
		public:
            inline jobject GetObject() {return m_object;}
			UIBase(Page* page)
			{
				m_page = page;
			}
			~UIBase();
			void SetParent(UIBase* p)
			{
				m_parent =p;
			}
		};
		class ViewGroup:
				public UIBase
		{
        protected:
			std::vector<UIBase*> m_kids;
			BEGIN_PACKAGE(ViewGroup)
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

		//UI Element
		class UIElement:
				public UIBase
		{
		    BEGIN_PACKAGE(UIElement)
				ADD_BASE(UIBase);
				APISET().AddFunc<1>("setText", &UIElement::setText);
                APISET().AddFunc<0>("getText", &UIElement::getText);
                APISET().AddFunc<1>("setOnClickListener", &UIElement::setOnClickListener);
			END_PACKAGE
			UIElement(Page* page):
					UIBase(page)
			{

			}
			bool setText(std::string txt);
            std::string getText();
            bool setOnClickListener(X::Value handler);
		};

		class TextView: public UIElement
		{
		BEGIN_PACKAGE(UIElement)
				ADD_BASE(UIElement);
			END_PACKAGE
			TextView(Page* page);
		};
        class EditText: public UIElement
        {
            BEGIN_PACKAGE(UIElement)
                ADD_BASE(UIElement);
            END_PACKAGE
            EditText(Page* page);
        };
		class Button: public UIElement
		{
			BEGIN_PACKAGE(Button)
				ADD_BASE(UIElement);
			END_PACKAGE
			Button(Page* page);
		};
		class Page: public ViewGroup {
			App* m_app = nullptr;
			BEGIN_PACKAGE(Page)
				ADD_BASE(ViewGroup);
				APISET().AddClass<0, LinearLayout,Page>("LinearLayout");
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
				APISET().AddClass<0, Page,App>("Page");
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
		};


		class AndroidWrapper {
			JNIEnv *m_env = nullptr;
			Locker m_mapLock;
            jobject m_objHost = nullptr;
			std::unordered_map<unsigned  int,jobject> m_objHostMap;
			App* m_app;
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

			AndroidWrapper(JNIEnv *env, jobject objHost):
					AndroidWrapper(){
				m_env = env;
				SetHostPerThread(objHost);
			}

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