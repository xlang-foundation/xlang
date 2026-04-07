from xlang_yaml import yaml

def run_test():
    folder = get_module_folder_path()
    path = folder + "/complex_test.yaml"
    out_path = folder + "/modify_out.yaml"

    print(f"Loading {path}")
    data = yaml.load(path)
    
    if data == None:
        print("Failed to load")
        return

    app_config = data["app_config"]
    
    # 1. Verify initial state
    print("Verifying initial comments...")
    comments_dict = app_config.getattr("__comments__")
    
    try:
        ks = comments_dict.keys()
        print("Comments Keys:", ks)
    except:
        print("Could not list comment keys")

    try:
        print("Initial Name Comment:", comments_dict["name"])
    except:
        print("Initial Name Comment: Missing")
    
    # 2. Modify: Remove 'version'
    print("Removing 'version'...")
    if app_config.has("version"):
        app_config.remove("version")
    else:
        print("FAIL: 'version' not found")
        
    # 3. Modify: Add 'new_key'
    print("Adding 'new_key'...")
    app_config["new_key"] = "fresh_value"
    
    # 4. Save
    print(f"Saving to {out_path}")
    yaml.save(data, out_path)
    
    # 5. Reload and Verify
    print(f"Reloading {out_path}")
    reloaded = yaml.load(out_path)
    if reloaded == None:
        print("Failed to reload")
        return

    rc = reloaded["app_config"]
    
    # Check 'name' comment (Should Preserved)
    try:
        c_name = rc.getattr("__comments__")["name"]
        print(f"Name Comment: '{c_name}' (Expected: 'Inline comment on string val')")
    except:
        print("Name Comment: FAILED (Missing)")

    # Check 'version' (Should be Gone)
    if rc.has("version"):
        print("Version: FAILED (Still exists)")
    else:
        print("Version: PASS (Removed)")
        
    # Check 'new_key' (Should be present, no comment)
    if rc.has("new_key"):
        v = rc["new_key"]
        print(f"New Key: '{v}' (PASS)")
        # Comment?
        try:
             c = rc.getattr("__comments__")["new_key"]
             if c == "" or c == None:
                 print(f"New Key Comment: PASS (Empty/None)")
             else:
                 print(f"New Key Comment: '{c}' (Unexpected, should be None/Missing)")
        except:
             print("New Key Comment: PASS (Exception)")
             
run_test()
