# test_path_import.x
# Verify importing a Python module using global path modifiers

print("CHECKPOINTS: cp1")

# Add a dummy path and then remove it to ensure engine doesn't crash
python_add_path("C:\\dummy_path_test_123")
python_remove_path("C:\\dummy_path_test_123")

import vision_detect_impl_sim deferred as vision_detect_impl

try:
    vision_detect_impl.load()
    vision_detect_impl.test_func() 
    # EXPECT: vision_detect_impl_sim dummy function executed
    print("(cp1)")
except e:
    print("Error:", e)
