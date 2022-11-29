x=" 012 ABCD-abc d "
y = x.regex_replace("[^a-zA-Z\\-]",'')
y = y.tolower()
print("y=",y)
print("end")
