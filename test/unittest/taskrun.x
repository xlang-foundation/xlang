print("taskrun test")
pool = taskpool(max_task_num=10,run_in_ui=False)
def test_fn(x,y):
	sleep(10000)
	return x+y

def get_result(r):
	f = (){
		print("mainrun,get_result:",r,"tid=",threadid());
	}
	mainrun(f)
	print("get_result:",r,"tid=",threadid())


f = test_fn.taskrun(pool,10,20).then((r){
	print("inside Then call:r=",r,"tid=",threadid());
}).then((r){
	print("Second *** inside Then call:r=",r,"tid=",threadid());	
}
)
f.then(get_result)
k  = f.get(100)
print("get k:",k,"tid=",threadid())
x = input()
print("end")
