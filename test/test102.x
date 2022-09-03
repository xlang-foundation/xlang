l=[1,2,3]
f=(evt){
    extern l;
	print("before event!=",l);
	l+=4;
	print("event!=",l);
}
f(1)
print(l)



