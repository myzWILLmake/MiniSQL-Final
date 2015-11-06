__author__ = 'frankhe'

import random

MAX_NUMBER_OF_SQLS = 5
MAX_NUMBER_OF_ATTRIBUTES = 10

letters = "abcdefghijklmnopqrstuvwxyz"
numbers = "1234567890"
types = ["int", "char(22)", "float", "char(30)"]

file1 = open("createTable_test.txt", mode="w+")
file2 = open('insertInto_test.txt', mode="w+")

def generateAName():
    length = random.randint(1, 20)
    name = ""
    for i in range(length):
        name = name+random.choice(letters)
    return name

def generateAData(data_type):
    data = ""
    if data_type == 'int':
        # length of a number is 1-5
        for i in range(random.randint(1,5)):
            data += random.choice(numbers)
        return data
    if data_type == 'float':
        for i in range(random.randint(1,5)):
            data += random.choice(numbers)
        data += '.'
        for i in range(random.randint(1,5)):
            data += random.choice(numbers)
        return data
    return generateAName()

tableNames = set()

tableName = "student"

for i in range(MAX_NUMBER_OF_SQLS):
    sql_create = "create table "
    while tableName in tableNames:
        tableName = generateAName()
    tableNames.add(tableName)
    sql_create = sql_create + tableName + "(\n"

    attributeNames = set()
    attributeName = generateAName()
    is_first_attribute = True
    first_attribute_name = ""
    tableTypeList = []
    for j in range(random.randint(1, MAX_NUMBER_OF_ATTRIBUTES)):
        while attributeName in attributeNames:
            attributeName = generateAName()
        attributeNames.add(attributeName)
        atype = random.choice(types)
        ifUnique = random.randint(0, 5)
        s_unique = ""
        if ifUnique == 0:
            s_unique = "unique"
        if is_first_attribute:
            first_attribute_name = attributeName
            s_unique = ""
            is_first_attribute = False
        tableTypeList.append(atype)
        sql_create = sql_create + '\t' + attributeName + ' ' + atype + " " + s_unique + ',\n'

    for j in range(random.randint(1, 10)):
        sql_insertInto = "insert into " + tableName + ' values ('
        for pos in range(len(tableTypeList)):
            if tableTypeList[pos].find('char') != -1:
                sql_insertInto += "'" + generateAData(tableTypeList[pos]) + "'"
            else:
                sql_insertInto += generateAData(tableTypeList[pos])
            if pos < len(tableTypeList)-1:
                sql_insertInto += ', '
        sql_insertInto += ');\n'
        print sql_insertInto
        file2.write(sql_insertInto)

    sql_create += "\tprimary key (" + first_attribute_name +")\n"
    sql_create += ");\n"
    print sql_create
    file1.write(sql_create)
