from xlang_yaml import yaml

x = yaml.load('agentflow.yml')
x.set("new_key",[1,3,4,5])
yaml.save(x,'newfile.yml')
print("Done")
