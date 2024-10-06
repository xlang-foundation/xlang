def HandleYaml(fileName):
	print("start ", fileName)
	return "https://www.bing.com"
def StopYaml(fileName):
	print("stop ", fileName)

def Register():
	return [{".yml":{"run":HandleYaml,"stop":StopYaml}}]