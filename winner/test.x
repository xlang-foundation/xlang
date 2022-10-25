from xlang_win import App
w = App.Window("test")
w.OnDraw+= (){
	print("event fired")
}
txt = w.TextEditBox(220,10,400,350)
btn = w.Button("Click Me",10,400,100,40)
btn.Click+= (){
	txt.SetText("Button 1 clicked");
	print("button clicked")
}
btn2 = w.Button("Click Me-2",200,400,200,40)
btn2.Click+= (){
	txt.SetText("Button 2 clicked");
	print("button2 clicked")
}
w.Show(True)
btn.Show(True)
App.Loop()
print("end")
