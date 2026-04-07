import json

x = "first line
second line"
y = {"key":x}
x_s = json.dumps(y)
x_back = json.loads(x_s)
print(x_s)
print(x_back)
print("Done")
