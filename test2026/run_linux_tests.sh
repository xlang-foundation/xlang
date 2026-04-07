#!/bin/bash
# Wrapper to run test2026 suite on Linux

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

# Ensure python3 is used
python3 run_tests.py
