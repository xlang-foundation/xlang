from xlang_sqlite import sqlite

xlang_addr ="some where in usa"
cantor_addr ="cantor way in usa"
pushWritepad(sqlite)
%USE example_db;
%CREATE TABLE COMPANY(\
   ID INT PRIMARY KEY     NOT NULL,\
   NAME           TEXT    NOT NULL,\
   AGE            INT     NOT NULL,\
   ADDRESS        CHAR(50),\
   Score          REAL\
);
%insert into COMPANY values(1,'The XLang Foundation',2,${xlang_addr},100);
%insert into COMPANY values(2,'CantorAI',1,${cantor_addr},99.9);
%Select * from COMPANY;
popWritepad()
print("end")

