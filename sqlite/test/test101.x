from xlang_sqlite import sqlite

x ="san jose"
y ="usa"
pushWritepad(sqlite)
%USE test3;
%CREATE TABLE COMPANY(\
   ID INT PRIMARY KEY     NOT NULL,\
   NAME           TEXT    NOT NULL,\
   AGE            INT     NOT NULL,\
   ADDRESS        CHAR(50),\
   SALARY         REAL\
);
for i in range(1000):
	%insert into COMPANY values(${i},'shawn',40,'4797 voltaire',100);
%Select * from COMPANY;
popWritepad()
print("end")

