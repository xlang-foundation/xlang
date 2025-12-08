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
xlang SQLite Module - Simple Test
Demonstrates basic CRUD with all DBStatement functions:
- db.exec(sql)          : Execute SQL directly (CREATE/DROP)
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

# Import sqlite module
sqlite = xlang.importModule("sqlite", fromPath="xlang_sqlite")

# Create database
db = sqlite.Database("test.db")

# ---- CREATE (exec) ----
print("Creating table...")
db.exec("""
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        email TEXT UNIQUE,
        score INTEGER
    )
""")

# ---- INSERT with bind() and reset() ----
print("Inserting users...")
users = [
    ("John Doe", "john@example.com", 85),
    ("Jane Smith", "jane@example.com", 92),
    ("Bob Wilson", "bob@example.com", 78),
]

# Prepare once, reuse with reset()
stmt = db.statement("INSERT OR IGNORE INTO users (name, email, score) VALUES (?, ?, ?)")
for name, email, score in users:
    stmt.bind(1, name)
    stmt.bind(2, email)
    stmt.bind(3, score)
    stmt.step()
    stmt.reset()  # Reset for next insert
stmt.close()

# ---- SELECT with colnum() and colname() ----
print("\nDynamic column info:")
stmt = db.statement("SELECT * FROM users")

num_cols = stmt.colnum()
print(f"  Column count: {num_cols}")
print(f"  Column names: {[stmt.colname(i) for i in range(num_cols)]}")

print("\nAll users:")
while sqlite.ROW == stmt.step():
    print(f"  ID: {stmt.get(0)}, Name: {stmt.get(1)}, Email: {stmt.get(2)}, Score: {stmt.get(3)}")
stmt.close()

# ---- SELECT with bind() ----
print("\nFinding user by name (bind):")
stmt = db.statement("SELECT * FROM users WHERE name = ?")
stmt.bind(1, "Jane Smith")
if sqlite.ROW == stmt.step():
    print(f"  Found: {stmt.get(1)} - {stmt.get(2)} (Score: {stmt.get(3)})")
stmt.close()

# ---- Reuse SELECT with reset() ----
print("\nQuerying multiple scores with reset():")
stmt = db.statement("SELECT name, score FROM users WHERE score >= ?")

thresholds = [90, 80, 70]
for threshold in thresholds:
    stmt.bind(1, threshold)
    print(f"  Score >= {threshold}:")
    while sqlite.ROW == stmt.step():
        print(f"    {stmt.get(0)}: {stmt.get(1)}")
    stmt.reset()
stmt.close()

# ---- UPDATE with bind() ----
print("\nUpdating John's score...")
stmt = db.statement("UPDATE users SET score = ? WHERE name = ?")
stmt.bind(1, 95)
stmt.bind(2, "John Doe")
stmt.step()
stmt.close()

# ---- DELETE with bind() ----
print("Deleting Bob...")
stmt = db.statement("DELETE FROM users WHERE name = ?")
stmt.bind(1, "Bob Wilson")
stmt.step()
stmt.close()

# ---- Final COUNT ----
stmt = db.statement("SELECT COUNT(*) FROM users")
if sqlite.ROW == stmt.step():
    print(f"\nTotal users remaining: {stmt.get(0)}")
stmt.close()

print("\nDone!")