def run_test():
    d = {}
    d["name"] = "Comment 1"
    d["version"] = "Comment 2"
    
    print("name:", d["name"])
    print("version:", d["version"])
    
    try:
        print("Accessing new_key:", d["new_key"])
    except:
        print("Accessing new_key: Threw Exception (Correct for script)")
        
    # How to simulate C++ Get behavior?
    # getattr?
    # d.getattr("new_key") ?? No, that's attr.
    # We are testing Item access.
    
    # If I use 'list'
    l = ["a", "b"]
    try:
        print("List[2]:", l[2])
    except:
        print("List[2]: Threw Exception")
        
    # If C++ Get returns first item, it's very broken.
run_test()
