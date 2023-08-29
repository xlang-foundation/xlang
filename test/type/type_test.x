import cantor thru 'lrpc:1000'
from Galaxy import galaxy

galaxy.cantor = cantor

class Run_Sam_filter(galaxy.BaseFilter):
	inputPin:Pin = None
	outputPin:Pin = None
	file_path ="c:\\"
	version = 1.0
	def Run_Sam_filter():
		this.inputPin = NewInputPin()
		this.outputPin = NewOutputPin()
		SetRunFunc(this.Run)
	def Run():
		print("in Run")

runSamfilter = Run_Sam_filter()

cantor_type = type(cantor)
cantor_members = cantor_type.getMembers()

galaxy_type = type(galaxy)
galaxy_members = galaxy_type.getMembers()

runSamfilter_type = type(runSamfilter)
runSamfilter_members = runSamfilter_type.getMembers()


print("end")

