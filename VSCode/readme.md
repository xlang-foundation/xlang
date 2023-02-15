# xlang
* A new dynamic programing language for **AI and IOT** with natural born **distributed computing ability**    
* A super glue to easily integrating with other languages such as c++/c, python and javascript and any framework cross operation system barriers.  
* Running faster than python about 3x-5x  

# Start xlang engine
- go to https://github.com/xlang-foundation/xlang.bin
- choose the latest version
- download to a local folder for example: xlang
- inside folder xlang, run command below

  xlang -dbg -event_loop -enable_python

# In vscode
- new or open a .x file which use same syntax like python
- Click on menu Run/Start Debugging
- will break on the first line

# Example Code
```python

def test():
    x = ["Hello Xlang","Awesome coding"]
    print(x)
    y = 2023
    print("y=${y}")
test()
print("well done")

```

# Example code with SQL

Directly Embedded SQL statements in xlang 

```python

from xlang_sqlite import sqlite

xlang_addr ="some where in usa"
cantor_addr ="cantor way in usa"
pushWritepad(sqlite)
#create a database:example and a table: commpany

%USE example;
%CREATE TABLE company(\
   ID INT PRIMARY KEY     NOT NULL,\
   NAME           TEXT    NOT NULL,\
   AGE            INT     NOT NULL,\
   ADDRESS        CHAR(50),\
   Score          REAL\
);
%INSERT INTO company VALUES(1,'XLang',2,${xlang_addr},100);
%INSERT INTO company VALUES(2,'CantorAI',1,${cantor_addr},99.9);
%SELECT * FROM company;
%DROP TABLE company;

popWritepad()
print("end")



```