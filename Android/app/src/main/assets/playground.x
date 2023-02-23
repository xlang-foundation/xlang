import android
uiTaskPool = taskpool(run_in_ui=True)
taskPool = taskpool(10)

app = android.app
color = app.Color("Red")
color2 =app.Color([0.3,0.3,0.5,0.6])
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
btn1.setText("Click Me")

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
    #tv2.setText(info)
btn1.setOnClickListener(btn1_onclick)
l1.add(tv)
l1.add(input1)
l1.add(btn1)
btn1.setMargins(100,100,100,100);
tv.setMargins(0,100,100,100);
l1.setPadding(100,20,100,10)
l1.add(tv2)
page.add(l1)
l1.setGravity(android.Gravity.LEFT)

def test(info):
   android.print('from test func:${info}')
test('first call')
