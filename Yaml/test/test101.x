from xlang_yaml import yaml
x1 = yaml.load('no_file.yml')
if x1.is_error():
	print("Error: ",x1.getInfo())
x = yaml.load('agentflow.yml')
x.set("new_key",[1,3,4,5])
yaml.save(x,'newfile.yml')
print("Done")
