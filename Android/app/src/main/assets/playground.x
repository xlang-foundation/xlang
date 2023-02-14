import android
app = android.app
page = app.Page()
l1 = page.LinearLayout()
tv = page.TextView()
tv.setText("Please Input:")
input1 = page.EditText()
tv2 = page.TextView()
tv2.setText("......")
btn1  = page.Button();
btn1.setText("Click Me")
def btn1_onclick():
    info = input1.getText()
    tv2.setText(info)
btn1.setOnClickListener(btn1_onclick)
l1.add(tv)
l1.add(input1)
l1.add(btn1)
l1.add(tv2)
page.add(l1)

def test(info):
   android.print('from test func:${info}')
test('first call')
