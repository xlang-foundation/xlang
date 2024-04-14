from xlang_os import os
process = os.Process()
process.ProgName = "powershell"
process.Parameter = '-Command "Get-ChildItem D:/test"'
retInfo = process.Run(True)


from xlang_os import fs

folder = fs.Folder("D:/test/")
newPath = folder.BuildPath("new01")
new01 = fs.Folder(newPath)
all = folder.Scan()
simple = folder.List()
folder_1 = fs.Folder("D:/test/set01")
folder.CreateFolder("D:/test/new01")
folder_1.CopyFolder("D:/test/new01")
folder.RemoveFolder("D:/test/new01")
print("done")

