
import os
import subprocess
import glob

def run_tests():
    root_dir = r"d:\CantorAI\xlang\test2026"
    xlang_exe = r"d:\CantorAI\xlang\out\build\x64-Debug\bin\xlang.exe"
    log_dir = os.path.join(root_dir, "testlogs")
    
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
        
    report_file = os.path.join(root_dir, "test_report.txt")
    
    # Find all .x files recursively
    test_files = [y for x in os.walk(root_dir) for y in glob.glob(os.path.join(x[0], '*.x'))]
    
    results = []
    
    print(f"Found {len(test_files)} test files.")
    print("Running tests with EXPECT verification...\n")
    
    with open(report_file, "w") as report:
        report.write("XLang Test Report\n")
        report.write("=================\n\n")
        
        for test_file in test_files:
            rel_path = os.path.relpath(test_file, root_dir)
            log_name = rel_path.replace(os.sep, "_") + ".log"
            log_path = os.path.join(log_dir, log_name)
            
            print(f"Testing: {rel_path} ...", end="")
            
            # Run xlang
            try:
                result = subprocess.run([xlang_exe, test_file], capture_output=True, text=True, timeout=10)
                output = result.stdout + "\n" + result.stderr
                output_lines = [line.strip() for line in output.splitlines() if line.strip()]
                
                # Verification Logic
                status = "PASS"
                
                # 1. Search for Checkpoint Declaration Header (Legacy)
                declared_cps = []
                found_header = False
                for line in output_lines:
                    if line.startswith("CHECKPOINTS:"):
                        # format: "CHECKPOINTS: cp1, cp2, cp3"
                        cps_str = line.split(":", 1)[1]
                        declared_cps = [cp.strip() for cp in cps_str.split(',') if cp.strip()]
                        found_header = True
                        break
                
                # 2. Search for EXPECT lines in source file (New)
                expected_lines = []
                with open(test_file, 'r', encoding='utf-8') as f:
                    for line in f:
                        if "# EXPECT:" in line:
                            exp = line.split("# EXPECT:", 1)[1].strip()
                            expected_lines.append(exp)

                with open(log_path, "w") as log:
                    log.write(output)
                    log.write("\n\n--- VERIFICATION ---\n")
                    if found_header:
                        log.write(f"Declared Checkpoints: {declared_cps}\n")
                    if expected_lines:
                        log.write(f"Expected Lines: {len(expected_lines)}\n")

                if "Compile Error" in output or "Error:" in output or "(cp-error)" in output:
                     status = "FAIL (Compile/Runtime Error)"
                else:
                    if found_header:
                        # Logic for Checkpoints
                        missing_cps = []
                        for cp in declared_cps:
                            cp_marker = f"({cp})"
                            found_cp = False
                            for line in output_lines:
                                if cp_marker in line:
                                    found_cp = True
                                    break
                            if not found_cp:
                                missing_cps.append(cp)
                        
                        if missing_cps:
                            status = f"FAIL (Missing Checkpoints: {missing_cps})"
                    
                    elif expected_lines:
                         # Logic for EXPECT
                         missing_expects = []
                         for exp in expected_lines:
                             if exp not in output_lines:
                                 missing_expects.append(exp)
                         
                         if missing_expects:
                             status = f"FAIL (Missing Expected Output: {missing_expects})"
                    
                    else:
                         status = "WARN (No Verification Found)"

                print(f" {status}")
                results.append((rel_path, status))
                report.write(f"{rel_path}: {status}\n")
                
            except subprocess.TimeoutExpired as e:
                 print(" TIMEOUT")
                 # Try to save partial output
                 output = ""
                 if e.stdout: output += e.stdout.decode('utf-8', errors='ignore') if isinstance(e.stdout, bytes) else e.stdout
                 if e.stderr: output += "\n" + (e.stderr.decode('utf-8', errors='ignore') if isinstance(e.stderr, bytes) else e.stderr)
                 
                 with open(log_path, "w") as log:
                    log.write(output)
                    log.write("\n\n--- TIMEOUT ---\n")
                 
                 results.append((rel_path, "TIMEOUT"))
                 report.write(f"{rel_path}: TIMEOUT\n")
            except Exception as e:
                print(f" ERROR: {e}")
                results.append((rel_path, f"EXECUTION ERROR: {e}"))
                report.write(f"{rel_path}: EXECUTION ERROR: {e}\n")

    print("\nTest Summary:")
    print("-------------")
    passed = sum(1 for r in results if r[1] == "PASS")
    print(f"Total: {len(results)}")
    print(f"Passed: {passed}")
    print(f"Failed: {len(results) - passed}")
    print(f"Report saved to: {report_file}")

if __name__ == "__main__":
    run_tests()
