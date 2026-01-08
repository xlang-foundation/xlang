
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
                
                # Checkpoint Verification Logic
                status = "PASS"
                missing_cps = []
                declared_cps = []
                
                # Search for Checkpoint Declaration
                found_header = False
                for line in output_lines:
                    if line.startswith("CHECKPOINTS:"):
                        # format: "CHECKPOINTS: cp1, cp2, cp3"
                        cps_str = line.split(":", 1)[1]
                        declared_cps = [cp.strip() for cp in cps_str.split(',') if cp.strip()]
                        found_header = True
                        break
                
                with open(log_path, "w") as log:
                    log.write(output)
                    log.write("\n\n--- VERIFICATION ---\n")
                    if found_header:
                        log.write(f"Declared Checkpoints: {declared_cps}\n")
                    else:
                        log.write("No CHECKPOINTS header found.\n")

                if "Compile Error" in output or "Error:" in output:
                     status = "FAIL (Compile/Runtime Error)"
                elif not found_header:
                     # Fallback to simple explicit PASS marker if no checkpoints declared
                     # But strictly, user wants checkpoint verification.
                     # We will mark separate status if no checkpoints found.
                     status = "WARN (No CHECKPOINTS header)"
                else:
                    # Verified each declared checkpoint exists in output as "(cpN)"
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

                print(f" {status}")
                results.append((rel_path, status))
                report.write(f"{rel_path}: {status}\n")
                if missing_cps:
                     report.write(f"  Missing: {missing_cps}\n")
                
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
