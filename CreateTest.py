__author__ = 'frankhe'

import random

MAX_NUMBER_OF_SQLS = 1000
MAX_NUMBER_OF_ATTRIBUTES = 10

letters = "abcdefghijklmnopqrstuvwxyz"
numbers = "1234567890"
types = ["int", "char(100)", "float"]

file = open("createTable_test.txt", mode="w+")

def generateAName():
    length = random.randint(1, 20)
    name = ""
    for i in range(length):
        name = name+random.choice(letters)
    return name

tableNames = set()

tableName = "student"

for i in range(MAX_NUMBER_OF_SQLS):
    sql = "create table "
    while tableName in tableNames:
        tableName = generateAName()
    tableNames.add(tableName)
    sql = sql + tableName + "(\n"

    attributeNames = set()
    attributeName = generateAName()
    for j in range(random.randint(1, MAX_NUMBER_OF_ATTRIBUTES)):
        while attributeName in attributeNames:
            attributeName = generateAName()
        attributeNames.add(attributeName)
        type = random.choice(types)
        sql = sql + '\t' + attributeName + ' ' + type + ',\n'
    sql += "\tprimary key (" + random.choice(list(attributeNames)) +")\n"
    sql += ");\n"
    file.write(sql)
