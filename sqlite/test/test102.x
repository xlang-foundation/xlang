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

db = sqlite.Database("example.db")
stmt = db.Statement("SELECT * FROM company");
while sqlite.ROW == stmt.step():
	x = stmt.get(1)
	print(x)
stmt.close()
%DROP TABLE company;

popWritepad()

print("end")