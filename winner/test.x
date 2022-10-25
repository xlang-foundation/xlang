from xlang_win import WinApp
w = WinApp.Window("test")
w.OnDraw+= (){
	print("event fired")
}
btn = w.Button("Click Me",10,10,100,100)
btn.click+= (){
	print("button clicked")
}
btn2 = w.Button("Click Me-2",200,10,200,100)
btn2.click+= (){
	print("button2 clicked")
}
w.Show(True)
btn.Show(True)
WinApp.Loop()
print("end")
