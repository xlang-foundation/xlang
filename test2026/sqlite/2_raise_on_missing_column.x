# 2_raise_on_missing_column.x
# Verify sqlite raises exception when selecting a non-existing column.

from xlang_sqlite import sqlite

pushWritepad(sqlite)

try:
    %USE example_missing_col;
    %DROP TABLE IF EXISTS company;
    %CREATE TABLE company(\
       ID INT PRIMARY KEY NOT NULL,\
       NAME TEXT NOT NULL\
    );

    # This should fail: column NOT_EXIST does not exist
    %SELECT NOT_EXIST FROM company;

    print("NO_EXCEPTION")
except e:
    print("CAUGHT")  # EXPECT: CAUGHT
finally:
    %DROP TABLE company;
    popWritepad()
