#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H
#include "CommonHeader.hpp"
#include "Block.hpp"
#include "CatalogManager.hpp"
#include "BufferManager.hpp"
#include "compare.hpp"
#include <iostream>
#include <fstream>
#include <map>

typedef map<string, vector<Value>>  Condition;

class IndexManager;

class RecordManager
{
private:
	CatalogManager *cm;
	BufferManager *bm;
	IndexManager *im;
public:
	RecordManager();
	~RecordManager();
	bool createTable(string tableName); // create a table record file
	bool dropTable(string tableName); // drop a table record file
	bool insertRecord(string tableName, Record &record); // insert a record to a table, api should check record and if the table exists
	bool deleteRecord(string tableName, vector<Value> &inputCondition); // delete record with conditions
	bool selectRecord(string tableName, vector<Value> &inputCondition); // query records with conditions
	bool getKeysOffsets(string tableName, string attributeName, vector<pair<Value, int>> &tmp); //get all values of an attribute in a table and offsets of those records
	void printRecord(const Record &record, vector<int> &attributeTypes); // print a record in a line
	void printHead(vector<string> &attributeNames); // print the table head
	bool optimize(string tableName, vector<Value> &inputCondition, Condition &condition); // optimaze the condition, invoked in the RM
	bool match(const Record &record, Condition &condition, string tableName); // check if a record matchs the condition
	bool compare(Value &v1, Value &v2, string op); // compare two values
	//string getRecordPathName(string tableName);
};

#endif
