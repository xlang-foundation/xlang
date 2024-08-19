# Example for Key and multiple values store
from xlang_sqlite import sqlite

def create_db():
    pushWritepad(sqlite)
    # Create a KV.db database
    %USE kv;
    %kv_store_if = SELECT name FROM sqlite_master WHERE type='table' AND name='kv_store';
    re = kv_store_if.fetch()
    if re == None:
        %CREATE TABLE kv_store (key TEXT PRIMARY KEY, value TEXT, value2 INTEGER);
    popWritepad()

def query_key(key):
    pushWritepad(sqlite)
    %USE kv;
    %kv_query = SELECT value, value2 FROM kv_store WHERE key = ${key};
    re = kv_query.fetch()
    popWritepad()
    # we assume there are two values, check
    # if one of them is None, then just return non-None value
    retVal = re
    if re[0] == None:
        retVal = re[1]
    elif re[1] == None:
        retVal = re[0]
    return retVal
def set_kv(key, value, value2):
    pushWritepad(sqlite)
    %USE kv;
    %INSERT OR REPLACE INTO kv_store (key, value, value2) VALUES (${key}, ${value}, ${value2});
    popWritepad()

def set_kv_text(key, value):
    pushWritepad(sqlite)
    %USE kv;
    %INSERT OR REPLACE INTO kv_store (key, value) VALUES (${key}, ${value});
    popWritepad()

def set_kv_int(key, value):
    pushWritepad(sqlite)
    %USE kv;
    %INSERT OR REPLACE INTO kv_store (key, value2) VALUES (${key}, ${value});
    popWritepad()


def remove_key(key):
    pushWritepad(sqlite)
    %USE kv;
    %DELETE FROM kv_store WHERE key = ${key};
    popWritepad()

def test():
    create_db()
    remove_key('kv2.x')
    set_kv('kv2.x', 'value1', 1)
    set_kv('kv2.y', 'value2', 2)
    set_kv_text('text1', 'this is a test')
    set_kv_int('int1', 100)
    t1 = query_key('text1')
    i1 = query_key('int1')
    v1 = query_key('kv2.x')
    v2 = query_key('kv2.y')
    v3 = query_key('kv2.z')
    print("Done!")

test()
