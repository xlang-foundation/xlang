from xlang_win import App
w = App.Window()
w.SetText("Test")
w.Create()
w.OnSize+=(){
	print("w.OnSize");
}

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
toolbar_img = App.Image("C:\\Dev\\X\\winner\\test.jpg")
toolbar = w.Toolbar()
toolbar.SetImageList(toolbar_img,8,58,58)
toolbar.SetButtonText(0,"File")
toolbar.SetButtonText(1,"Edit")
toolbar.SetButtonText(2,"View")

toolbar.Create()
c = w.CreateChildWindow(10,100,700,bot_line-120)
draw = c.Draw()
img = App.Image("C:\\Data\\2.jpg")
img2 = App.Image("C:\\Dev\\Cantor\\factory\\bg2.jpg")
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
	draw.DrawImage(img2,100-pos,100,700-pos,400);
	draw.DrawImage(img,100-pos,400,700-pos,800);
	draw.End();
}

txt = w.TextEditBox()
txt.SetRect(410,bot_line,200,30)
txt.Create()
btn = w.Button()
btn.SetText("Reset")
btn.SetRect(10,bot_line,100,40)
btn.Click+= (){
	global pos;
	pos =1;
	c.Repaint();
	txt.SetText("Button Reset clicked");
	btn2.Left =btn2.Left+30;
	print("button clicked")
}
btn.Create()
btn2 = w.Button()
btn2.SetText("Move")
btn2.SetRect(200,bot_line,200,40)
print("btn.rect={",btn.Left,",",btn.Top,",",btn.Right,",",btn.Bottom,"}")
print("btn2.rect={",btn2.Left,",",btn2.Top,",",btn2.Right,",",btn2.Bottom,"}")
btn2.Click+= (){
	global pos;
	pos +=5;
	c.Repaint();
	txt.SetText("Button Move clicked");
	print("button2 clicked")
}
btn2.Create()
w.Show(True)
btn.Show(True)
App.Loop()
print("end")
