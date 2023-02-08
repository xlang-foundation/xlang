#pragma once

#include <jni.h>
#include <vector>
#include "value.h"
#include "xpackage.h"
#include "xlang.h"

namespace X
{
	namespace Android {
		class AndroidWrapper;
		class App;
		class Page;

		class UIBase
		{
			UIBase* m_parent = nullptr;
            BEGIN_PACKAGE(UIBase)
            END_PACKAGE
		public:
			void SetParent(UIBase* p)
			{
				m_parent =p;
			}
		};
		class ViewGroup:
				public UIBase
		{
			Page* m_page = nullptr;
			ViewGroup* m_parent = nullptr;
			std::vector<UIBase*> m_kids;
			BEGIN_PACKAGE(ViewGroup)
				APISET().AddFunc<1>("add", &ViewGroup::Add);
			END_PACKAGE
			ViewGroup(Page* page)
			{
				m_page = page;
			}
			bool Add(UIBase* pElement)
			{
				m_kids.push_back(pElement);
				pElement->SetParent(this);
				return true;
			}
		};
		class LinearLayout: public ViewGroup{
			BEGIN_PACKAGE(LinearLayout)
				ADD_BASE(ViewGroup);
			END_PACKAGE
			LinearLayout(Page* page):
					ViewGroup(page)
			{
			}
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
		class Page: public ViewGroup {
			App* m_app = nullptr;
			jobject m_objPage = nullptr;
			BEGIN_PACKAGE(Page)
				ADD_BASE(ViewGroup);
				APISET().AddClass<0, LinearLayout,Page>("LinearLayout");
				APISET().AddClass<0, ConstraintLayout,Page>("ConstraintLayout");
			END_PACKAGE
			bool Create();
			Page(App* pParent): ViewGroup(nullptr)
			{
				m_app = pParent;
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

		//UI Element
		class UIElement {

		};

		class TextView {

		};

		class Button {

		};

		class AndroidWrapper {
			JNIEnv *m_env = nullptr;
			jobject m_objHost = nullptr;
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
				m_objHost = objHost;
			}

			~AndroidWrapper() {
				if (m_env != nullptr && m_objHost != nullptr) {
					m_env->DeleteGlobalRef(m_objHost);
				}
				if(m_app)
				{
					delete m_app;
				}
			}

			X::Value GetApp() {
				X::XPackageValue<App> valApp(m_app);
				return valApp;
			}
			JNIEnv* GetEnv() {return m_env;}
			jobject GetHost() {return m_objHost;}
			bool Print(std::string info);
		};
	}
}