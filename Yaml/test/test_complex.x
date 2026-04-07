from xlang_yaml import yaml

def verify_comment(obj, query_key_or_index, expected_comment):
    print(f"Verifying comment for {query_key_or_index}...")
    try:
        comments = getattr(obj, "__comments__")
    except:
        comments = None

    if comments == None:
        if expected_comment == None:
             print(f"PASS: No comment as expected")
        else:    
             print(f"FAIL: No __comments__ attribute found on object")
        return
    
    # Check if key exists in comments dict
    # comments is an X::Dict. In script, we can access it via []?
    # Or maybe it's a Python dict if via pyeng?
    # No, it's X::Value (Dict).
    # If using xlang script, [] should work.
    
    try:
        actual = comments[query_key_or_index]
        if actual == expected_comment:
            print(f"PASS: '{actual}'")
        else:
            print(f"FAIL: Expected '{expected_comment}', got '{actual}'")
    except:
         if expected_comment == None:
             print(f"PASS: No comment as expected")
         else:
             print(f"FAIL: Comment key not found")

def run_test():
    folder = get_module_folder_path()
    path = folder + "/complex_test.yaml"
    print(f"Loading {path}")
    
    data = yaml.load(path)
    
    if data == None:
        print("Failed to load YAML")
        return

    # 0. Check Root Comment
    root_comment = data.getattr("__root_comment__")
    print(f"Root comment: '{root_comment}'")
    # Expected: "Header Block comment... \n Second line..."
    # My simple extractor might have stripped spaces differently?
    # Extractor: `line.substr(first)`.
    # `ExtractComments` returns map.
    # `LoadFromString` joins with `\n`.
    
    # 1. Check App Config Name
    app_config = data["app_config"]
    verify_comment(app_config, "name", "Inline comment on string val")
    verify_comment(app_config, "version", "Inline comment on number")
    
    # 2. Check List
    features = app_config["features"]
    
    # Debug comments dict
    f_comments = features.getattr("__comments__")

    verify_comment(features, "0", "List item 0 inline")
    verify_comment(features, "1", "List item 1 inline")
    
    # Check quoted string with hash
    # "quoted # no comment" - Should be NO comment
    # In my logic, if no comment, accessing comments[index] throws or returns null?
    verify_comment(features, "2", "Quoted hash should NOT be a comment") # Wait, my logic DOES find it! It follows the quote!
    
    # 3. Check DB Port (Block comment?)
    database = data["database"]
    # Expect failure for block comment with current implementation
    # But verify what we get.
    verify_comment(database, "port", "Block comment before port" ) 
    
    # 4. Save and inspect
    out_path = "d:\\CantorAI\\xlang\\Yaml\\test\\complex_test_out.yaml"
    print(f"Saving to {out_path}")
    yaml.save(data, out_path)
    print("Done")

run_test()
