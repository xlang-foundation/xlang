class Point():
    x:int
    y:int
    def constructor(x:int,y:int):
        this.x = x
        this.y = y


class Car():
    color:str ='red'
    curPos:Point
    on_move:event

    def MoveTo(pos):
        on_move(1,"22","333")
        this.curPos =pos
        print("call MoveTo")


c = Car()
p = Point(10,100)
f = (x,y,z){
    print("on_move_fired,x=",x,",y=",y,",z=",z)
}
c.on_move+=f
c.on_move+=(x,y){
    print("on_move_fired-Second,x=",x,",y=",y)
}
c.MoveTo(p)
print("end")

