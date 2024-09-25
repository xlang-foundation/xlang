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

from xlang_win import App

uiTaskPool = taskpool(run_in_ui=True)
taskPool = taskpool(10)

w = App.Window()
w.SetText("Test")
w.OnSize+=(){
	print("w.OnSize");
}
w.Create()

box = w.Box()
box.SetLeft(w,0,10)
box.SetTop(w,3,-50)
box.SetRight(w,2,10)
box.SetBottom(w,3,-10)

#Menu
main_menu = w.Menu()
file_menu = w.Menu()
file_menu.Insert(0,"New")
file_menu.Insert(1,"Open")
main_menu.InsertSubMenu(0,file_menu,"File")
edit_menu = w.Menu()
edit_menu.Insert(0,"Go to")
edit_menu.Insert(1,"Find")
edit_menu.Insert(2,"Search")
main_menu.InsertSubMenu(1,edit_menu,"Edit")

w.SetMenu(main_menu)

bot_line = 520
toolbar_img = App.Image("bg2.jpg")
toolbar = w.Toolbar()
toolbar.SetLeft(w,0,0)
toolbar.SetTop(w,1,0)
toolbar.SetRight(w,2,0)
toolbar.SetBottom(w,1,50)
toolbar.SetImageList(toolbar_img,12,64,64)
toolbar.SetButtonText(0,"File")
toolbar.SetButtonText(1,"Edit")
toolbar.SetButtonText(2,"View")
toolbar.Create()

r_txt = w.TextEditBox()
r_txt.SetLeft(w,2,-300)
r_txt.SetTop(w,1,100)
r_txt.SetRight(w,2,-10)
r_txt.SetBottom(box,1,-10)

r_txt.Create()
c = w.CreateChildWindow(10,100,700,bot_line-120)
c.SetLeft(w,0,10)
c.SetTop(w,1,100)
c.SetRight(r_txt,0,-5)
c.SetBottom(w,3,-70)


draw = c.Draw()
img = App.Image("bg.jpg")
img2 = App.Image("bg2.jpg")
brush = draw.Brush("Cyan")
brush2 = [draw.Brush("DarkMagenta"),
	draw.Brush("Coral"),
	draw.Brush("DarkGreen"),
	draw.Brush("DarkKhaki"),
	draw.Brush("CadetBlue"),
	draw.Brush("HotPink"),
	draw.Brush("Black"),
	draw.Brush("MediumPurple"),
	draw.Brush("BlueViolet"),
	draw.Brush("PaleGoldenrod"),
	draw.Brush("SlateBlue")
]
pos =10
c.OnDraw+= (){
	print("c.OnDraw=",pos);
	draw.Begin();
	draw.Clear("AliceBlue");
	draw.DrawRectangle(10,20,300,300,brush);
	size = 11;
	for i in range(size):
		draw.DrawRectangle(pos+40,70+i*20,pos+200,140+i*20,brush2[i])
	if pos ==1:
		draw.DrawImage(img2,100-pos,100,700-pos,400);
	else:
		draw.DrawImage(img,100-pos,100,700-pos,400);
	draw.End();
}

txt = w.TextEditBox()
txt.SetRect(410,bot_line,200,30)
txt.SetTop(box,1,0)
txt.SetRight(box,2,-100)
txt.Create()
btn = w.Button()
btn.SetText("Reset")
btn.SetRect(10,bot_line,100,40)
btn.SetTop(box,1,0)

btn.Click+= (){
	global pos;
	pos =1;
	c.Repaint();
	txt.SetText("Button Reset clicked");
	print("button clicked")
}
btn.Create()
btn2 = w.Button()
btn2.SetText("Move")
btn2.SetRect(200,bot_line,200,40)
print("btn.rect={",btn.Left,",",btn.Top,",",btn.Right,",",btn.Bottom,"}")
print("btn2.rect={",btn2.Left,",",btn2.Top,",",btn2.Right,",",btn2.Bottom,"}")
btn2.SetTop(box,1,0)
txt.SetLeft(btn2,2,10)

def sleep_fn(x,y):
	print("before sleep","tid=",threadid())
	sleep(5000)
	print("after sleep","tid=",threadid())
	return x+y

def Move(x):
	global pos
	pos +=x
	c.Repaint()
	txt.SetText("Button Move clicked,pos="+str(pos)+"tid="+str(threadid()))

btn2.Click+= (){
	sleep_fn.taskrun(taskPool,10,20).then((r){
		print("r=",r,"tid=",threadid());
		Move.taskrun(uiTaskPool,5);
	})
	print("button2 clicked")
}
btn2.Create()

w.Show(True)
btn.Show(True)
App.Loop()
print("end")
