object_ids = []
objectIdsParam = "7411736681608380418,7411736681608380419,7411736681608380420,7411736681608380421,7411736681608380422,7411736681608380423"
if objectIdsParam != None and objectIdsParam != "":
    object_ids = [int(x.strip()) for x in objectIdsParam.split(",") if x.strip() != ""]
print(object_ids)
print("Done")