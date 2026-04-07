
from xlang_yaml import yaml

def test():
    path = "d:/CantorAI/xlang/Yaml/test/modify_out.yaml"
    print(f"Loading {path}")
    data = yaml.load(path)
    
    app_config = data["app_config"]
    comments = getattr(app_config, "__comments__")
    print("Comments Type:", type(comments))
    
    if comments:
        print("Keys in __comments__:")
        # Enum usage to be safe
        # In XLang we can iterate dict
        for k in comments:
             v = comments[k]
             print(f"Key: '{k}' -> Val: '{v}'")
             
    # Specific check
    try:
        val = comments["new_key"]
        print(f"Direct access 'new_key': '{val}' Type: {type(val)}")
        if val is None:
             print("Val is None")
    except Exception as e:
        print(f"Direct access 'new_key' failed: {e}")

    print("--- Dictionary Behavior Check ---")
    d = {"a":1}
    try:
        x = d["b"]
        print(f"Missing key returned: '{x}' Type: {type(x)}")
    except Exception as e:
        print(f"Missing key threw: {e}")

