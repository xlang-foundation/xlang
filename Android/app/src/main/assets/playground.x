#
# Copyright (C) 2024 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

import android
uiTaskPool = taskpool(run_in_ui=True)
taskPool = taskpool(10)

app = android.app
color = app.Color("Red")
color2 =app.Color([0.3,0.3,0.5,0.6])

page1 = None
page2 = None

def buildPage2():
    page = app.Page()
    l1 = page.LinearLayout()
    btn21  = page.Button();
    btn21.setText("Previous")
    def btn21_onclick():
        app.showPage(page1)
    btn21.setOnClickListener(btn21_onclick)
    l1.add(btn21)
    page.add(l1)
    return page

page = app.Page()
#page.setBackgroundColor(app.Color("Yellow"))
l1 = page.LinearLayout()
tv = page.TextView()
tv.setText("Please Input:")
tv.setTextSize(2,30);
input1 = page.EditText()
input1.setPadding(0,50,0,50)
input1.setTextColor(color2)
input1.setBackgroundColor(app.Color("Gray"))
tv2 = page.TextView()
tv2.setTextColor(color)
tv.setBackgroundColor(color2)
tv2.setText(android.AppInfo.Title)
btn1  = page.Button();
#btn1.setPadding(0,50,0,50)
btn1.setText("Copy To TextView")
btn2  = page.Button();
btn2.setText("Next")

def sleep_fn(info):
    print("before sleep","tid=",threadid())
    sleep(5000)
    print("after sleep","tid=",threadid())
    return info
def tv2_setText(info):
    tv2.setText(info)
def btn1_onclick():
    info = input1.getText()
    sleep_fn.taskrun(taskPool,info).then((r){
        print("r=",r,"tid=",threadid());
        tv2_setText.taskrun(uiTaskPool,r);
    })
    tv2.setText(info)
def btn2_onclick():
    app.showPage(page2)
btn1.setOnClickListener(btn1_onclick)
btn2.setOnClickListener(btn2_onclick)
l1.add(tv)
l1.add(input1)
l1.add(btn1)
btn1.setMargins(100,100,100,100);
tv.setMargins(0,100,100,100);
l1.setPadding(100,20,100,10)
l1.add(tv2)
l1.add(btn2)
sv = page.ScrollView()
sv.add(l1)
page.add(sv)
l1.setGravity(android.Gravity.LEFT)
page1 = page
page2 = buildPage2()

app.showPage(page1)
def test(info):
   android.print('from test func:${info}')
test('first call')
