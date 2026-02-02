# 3_raise_on_use_database_fail.x
# Verify sqlite raises exception when USE database fails to open.

from xlang_sqlite import sqlite

pushWritepad(sqlite)

try:
    # Use a Windows device name as a *directory* to force open failure.
    # This should not be a valid file path for sqlite.
    %USE NUL\\bad.db;
    print("NO_EXCEPTION")
except e:
    print("CAUGHT")  # EXPECT: CAUGHT
finally:
    popWritepad()
