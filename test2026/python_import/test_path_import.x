# test_path_import.x
# Verify importing a Python module via a relative path

import vision_detect_impl_sim deferred as vision_detect_impl

try:
    print("CHECKPOINTS: cp1")
    vision_detect_impl.load()
    vision_detect_impl.test_func() # EXPECT: vision_detect_impl_sim dummy function executed
    print("(cp1)")
except e:
    print("Error:", e)
