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
	bool createTable(string tableName);
	bool dropTable(string tableName);
	bool insertRecord(string tableName, Record &record);
	bool deleteRecord(string tableName, vector<Value> &inputCondition);
	bool selectRecord(string tableName, vector<Value> &inputCondition);
	bool getKeysOffsets(string tableName, string attributeName, vector<pair<Value, int>> &tmp);
	void printRecord(const Record &record, vector<int> &attributeTypes);
	void printHead(vector<string> &attributeNames);
	bool optimize(string tableName, vector<Value> &inputCondition, Condition &condition);
	bool match(const Record &record, Condition &condition, string tableName);
	bool compare(Value &v1, Value &v2, string op);
	//string getRecordPathName(string tableName);
};

#endif
