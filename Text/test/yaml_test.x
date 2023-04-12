import yaml

y = "---

 search_path: 
  - path1
  - path2
"

k = yaml.loads(y)
print("end")
