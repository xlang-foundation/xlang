#const ca.san_jose.jack.OK =10.33
#print("ca.san_jose.jack.OK=",ca.san_jose.jack.OK)
#const ca.san_jose.shawn.OK =3.141
#print("ca.san_jose.shawn.OK=",ca.san_jose.shawn.OK)
namespace xlang
	version = 1.0
	Maker = "The Xlang Foundation"
	namespace Contributes:
		Jack = 100
		var Others:
			QA_IDs = [119,200]
			Release_Engs ="Alpha"
x = xlang.Contributes.Others.Release_Engs
print(x)
#var xyz =[111,222]
print("end")

