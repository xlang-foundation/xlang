
def test_return(x):
	if x == 1:
		x +=100
		return x
	elif x == 2:
		return 2
	elif x == 3:
		return 3
	print("This is last code")
	return 10

y = test_return(1)
print(y)
y = test_return(9)
print(y)
