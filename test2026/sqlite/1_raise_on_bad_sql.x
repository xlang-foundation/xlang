# 1_raise_on_bad_sql.x
# Verify sqlite package raises exception on SQL errors (bad SQL syntax).
#
# Pattern is based on xlang/sqlite/test/test101.x

from xlang_sqlite import sqlite

pushWritepad(sqlite)

try:
    %USE example;
    # Intentionally invalid SQL keyword to force sqlite3_prepare_v2 failure
    %SEELCT 1;
    print("NO_EXCEPTION")
except e:
    print("CAUGHT")  # EXPECT: CAUGHT
finally:
    popWritepad()
