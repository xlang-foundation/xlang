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

