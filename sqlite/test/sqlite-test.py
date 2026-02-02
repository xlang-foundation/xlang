#
# Copyright (C) 2025 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

"""
xlang SQLite Module Test
Demonstrates all DBStatement functions:
- db.exec(sql)          : Execute non-SELECT SQL directly
- db.statement(sql)     : Create prepared statement
- stmt.bind(idx, val)   : Bind value at index (1-based)
- stmt.step()           : Execute/fetch next row
- stmt.get(idx)         : Get column value (0-based)
- stmt.reset()          : Reset statement for reuse
- stmt.colnum()         : Get number of columns
- stmt.colname(idx)     : Get column name by index
- stmt.close()          : Close statement
"""

import xlang

# Import the sqlite module from xlang
sqlite = xlang.importModule("sqlite", fromPath="xlang_sqlite")

def test_sqlite():
    # Create or open database
    db = sqlite.Database(":memory:")  # Use in-memory database for testing
    # db = sqlite.Database("example.db")  # Or use a file-based database
    
    print("=" * 60)
    print("xlang SQLite Test - All Statement Functions")
    print("=" * 60)
    
    # =========================================
    # 1. CREATE TABLE (using exec)
    # =========================================
    print("\n[1] Creating table 'company'...")
    db.exec("""
        CREATE TABLE IF NOT EXISTS company (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            age INTEGER,
            department TEXT,
            salary REAL
        )
    """)
    print("Table 'company' created successfully!")
    
    # =========================================
    # 2. INSERT with bind() - reusing statement with reset()
    # =========================================
    print("\n[2] Inserting data with bind() and reset()...")
    
    employees = [
        ("Alice", 30, "Engineering", 85000.0),
        ("Bob", 25, "Marketing", 65000.0),
        ("Charlie", 35, "Engineering", 95000.0),
        ("Diana", 28, "HR", 55000.0),
        ("Eve", 32, "Engineering", 90000.0),
    ]
    
    # Prepare statement once, reuse with reset()
    stmt = db.statement("INSERT INTO company (name, age, department, salary) VALUES (?, ?, ?, ?)")
    
    for name, age, dept, salary in employees:
        stmt.bind(1, name)
        stmt.bind(2, age)
        stmt.bind(3, dept)
        stmt.bind(4, salary)
        stmt.step()
        stmt.reset()  # Reset for next iteration
        print(f"  Inserted: {name}")
    
    stmt.close()
    print(f"Inserted {len(employees)} employees.")
    
    # =========================================
    # 3. SELECT with colnum() and colname()
    # =========================================
    print("\n[3] Demonstrating colnum() and colname()...")
    
    stmt = db.statement("SELECT id, name, age, department, salary FROM company")
    
    # Get column count
    num_cols = stmt.colnum()
    print(f"  Number of columns: {num_cols}")
    
    # Get column names
    col_names = []
    for i in range(num_cols):
        col_name = stmt.colname(i)
        col_names.append(col_name)
    print(f"  Column names: {col_names}")
    
    # Print header dynamically
    print("-" * 60)
    header = "  ".join(f"{name:<12}" for name in col_names)
    print(header)
    print("-" * 60)
    
    # Fetch rows
    while sqlite.ROW == stmt.step():
        row = []
        for i in range(num_cols):
            row.append(stmt.get(i))
        print("  ".join(f"{str(v):<12}" for v in row))
    stmt.close()
    
    # =========================================
    # 4. SELECT with WHERE using bind()
    # =========================================
    print("\n[4] SELECT with WHERE using bind()...")
    
    stmt = db.statement("SELECT name, salary FROM company WHERE department = ?")
    stmt.bind(1, "Engineering")
    
    print("  Engineering department:")
    while sqlite.ROW == stmt.step():
        print(f"    {stmt.get(0)}: ${stmt.get(1):.2f}")
    stmt.close()
    
    # =========================================
    # 5. Reusing SELECT statement with reset()
    # =========================================
    print("\n[5] Reusing SELECT statement with reset()...")
    
    stmt = db.statement("SELECT name, salary FROM company WHERE department = ?")
    
    departments = ["Engineering", "Marketing", "HR"]
    for dept in departments:
        stmt.bind(1, dept)
        print(f"  {dept} department:")
        while sqlite.ROW == stmt.step():
            print(f"    {stmt.get(0)}: ${stmt.get(1):.2f}")
        stmt.reset()  # Reset to query again with different parameter
    
    stmt.close()
    
    # =========================================
    # 6. SELECT with multiple bind parameters
    # =========================================
    print("\n[6] SELECT with multiple bind parameters...")
    
    stmt = db.statement("SELECT name, salary FROM company WHERE department = ? AND salary > ?")
    stmt.bind(1, "Engineering")
    stmt.bind(2, 80000.0)
    
    print("  Engineers with salary > $80,000:")
    while sqlite.ROW == stmt.step():
        print(f"    {stmt.get(0)}: ${stmt.get(1):.2f}")
    stmt.close()
    
    # =========================================
    # 7. Aggregate with bind
    # =========================================
    print("\n[7] Aggregate functions with bind...")
    
    stmt = db.statement("SELECT COUNT(*), AVG(salary), MAX(salary) FROM company WHERE department = ?")
    stmt.bind(1, "Engineering")
    
    if sqlite.ROW == stmt.step():
        print(f"  Engineering stats:")
        print(f"    Count: {stmt.get(0)}")
        print(f"    Avg Salary: ${stmt.get(1):.2f}")
        print(f"    Max Salary: ${stmt.get(2):.2f}")
    stmt.close()
    
    # =========================================
    # 8. UPDATE with bind()
    # =========================================
    print("\n[8] UPDATE with bind()...")
    
    stmt = db.statement("UPDATE company SET salary = ? WHERE name = ?")
    stmt.bind(1, 70000.0)
    stmt.bind(2, "Bob")
    stmt.step()
    stmt.close()
    
    # Verify
    stmt = db.statement("SELECT name, salary FROM company WHERE name = ?")
    stmt.bind(1, "Bob")
    if sqlite.ROW == stmt.step():
        print(f"  Updated: {stmt.get(0)}'s new salary is ${stmt.get(1):.2f}")
    stmt.close()
    
    # =========================================
    # 9. Batch UPDATE with reset()
    # =========================================
    print("\n[9] Batch UPDATE with reset()...")
    
    raises = [("Alice", 90000.0), ("Charlie", 100000.0), ("Eve", 95000.0)]
    
    stmt = db.statement("UPDATE company SET salary = ? WHERE name = ?")
    for name, new_salary in raises:
        stmt.bind(1, new_salary)
        stmt.bind(2, name)
        stmt.step()
        stmt.reset()
        print(f"  Updated {name}'s salary to ${new_salary:.2f}")
    stmt.close()
    
    # =========================================
    # 10. DELETE with bind()
    # =========================================
    print("\n[10] DELETE with bind()...")
    
    stmt = db.statement("DELETE FROM company WHERE department = ?")
    stmt.bind(1, "HR")
    stmt.step()
    stmt.close()
    print("  Deleted HR department employees")
    
    # =========================================
    # 11. SELECT with LIKE and bind
    # =========================================
    print("\n[11] SELECT with LIKE pattern...")
    
    stmt = db.statement("SELECT name FROM company WHERE name LIKE ?")
    stmt.bind(1, "%e")  # Names ending with 'e'
    
    print("  Names ending with 'e':")
    while sqlite.ROW == stmt.step():
        print(f"    {stmt.get(0)}")
    stmt.close()
    
    # =========================================
    # 12. SELECT with BETWEEN and bind
    # =========================================
    print("\n[12] SELECT with BETWEEN...")
    
    stmt = db.statement("SELECT name, age FROM company WHERE age BETWEEN ? AND ?")
    stmt.bind(1, 30)
    stmt.bind(2, 35)
    
    print("  Employees aged 30-35:")
    while sqlite.ROW == stmt.step():
        print(f"    {stmt.get(0)}: {stmt.get(1)} years old")
    stmt.close()
    
    # =========================================
    # 13. Final SELECT with dynamic columns
    # =========================================
    print("\n[13] Final employee list (dynamic columns):")
    
    stmt = db.statement("SELECT * FROM company ORDER BY salary DESC")
    
    # Build header from column names
    num_cols = stmt.colnum()
    col_names = [stmt.colname(i) for i in range(num_cols)]
    
    print("-" * 60)
    print("  ".join(f"{name:<12}" for name in col_names))
    print("-" * 60)
    
    while sqlite.ROW == stmt.step():
        row = [stmt.get(i) for i in range(num_cols)]
        print("  ".join(f"{str(v):<12}" for v in row))
    stmt.close()
    
    # =========================================
    # 14. Cleanup
    # =========================================
    print("\n[14] Cleanup...")
    db.exec("DROP TABLE IF EXISTS company")
    print("  Table dropped successfully!")
    
    print("\n" + "=" * 60)
    print("All tests completed successfully!")
    print("=" * 60)


if __name__ == "__main__":
    test_sqlite()
