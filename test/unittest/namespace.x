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

from xlang_os import fs
print(fs.abc)
xy = fs.init(100,200)
print("(out)xy=",xy)
namespace xlang.ca.san_jose.jack.OK =10.33
print("xlang.ca.san_jose.jack.OK=",xlang.ca.san_jose.jack.OK)
const xlang.ca.san_jose.shawn.OK =3.141
print("ca.san_jose.shawn.OK=",xlang.ca.san_jose.shawn.OK)
const xlang.ca.san_jose.jack.OK =100120.33
print("ca.san_jose.shawn.OK(changed)=",xlang.ca.san_jose.shawn.OK)
print("ca.san_jose.jack.OK=",xlang.ca.san_jose.jack.OK)
namespace xlang:
	class class_test():
		def func_in_class(x,y,z):
			print("xyz in func_in_class of class_test:",x,y,z)
			return x+y+z			
	def func1(x,y,z):
		print("xyz in func1:",x,y,z)
		return x+y+z
	version = 1.0
	Maker = "The Xlang Foundation"
	|- Contributes:
		Jack = 100
		var Others:
			QA_IDs = [119,200]
			Release_Engs ="Alpha"
namespace xlang:
	namespace Contributes:
		var Others:
			xyz = 1972.09
xlang.class_test.func_in_class("no instance call,",2,3)
cls = xlang.class_test()
cls.func_in_class(10,20,30)
xlang.func1(1,2.3,100)
x = xlang.Contributes.Others.xyz
print("xyz=",x,"\nQA_IDs=",xlang.Contributes.Others.QA_IDs)
var xlang.Contributes.Others.abc =[111,222]
print("xyz=",xlang.Contributes.Others.abc)
print("end")

