python_switch_env("d:\\CantorAI\\xlang\\test2026\\test_venv")
print("CHECKPOINTS: cp1")

import vision_detect_impl_sim

vision_detect_impl_sim.check_isolation()
# EXPECT: Isolation Verified: Global 'numpy' cannot be loaded inside venv!

vision_detect_impl_sim.test_func()
# EXPECT: vision_detect_impl_sim dummy function executed

# 2. Restore ENV
python_restore_env()

vision_detect_impl_sim.check_restore_state()
# EXPECT: Restore Verified: sys.path is clean!
# EXPECT: Restore Verified: Global 'numpy' is accessible again!

print("(cp1)")
