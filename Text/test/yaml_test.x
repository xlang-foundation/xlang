import yaml

y = "
---
name: 'this is a name'
port: 3145
search_path: 
 - path1
 - path2
"

k = yaml.loads(y)
c0 =k['search_path',0]
c0_1 =k["search_path"][0]
c =k['search_path',1]
c2 = k["key"]
print("end")
