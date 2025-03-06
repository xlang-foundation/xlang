#
# Copyright (C) 2024 The XLang Foundation
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

from xlang_sqlite import sqlite

xlang_addr ="some where in usa"
cantor_addr ="cantor way in usa"

company_list = None

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
% company_list = SELECT * FROM company;
re =company_list.fetch()
%DROP TABLE company;

popWritepad()

print(re)

print("end")

