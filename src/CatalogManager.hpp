#ifndef CATALOGMANAGER_H
#define CATALOGMANAGER_H

#include "CommonHeader.hpp"
#include "Interpreter.hpp"
#include <fstream>
#include <iostream>

using namespace std;
const int tableInfoLines = 3;
const string tableCatalog = "./catalogs/table_catalog";
const string indexCatalog = "./catalogs/index_catalog";
const string tempFile = "./catalogs/temp_file";

class CatalogManager
{
public:
	CatalogManager();
	~CatalogManager();
	void createTableCatalog();
	void createIndexCatalog();
	bool createTableInfo(TransferArguments *args);
	bool dropTableInfo(string tableName);
	bool dropIndexInfo(string indexName);
	bool addIndexInfo(string indexName, string tableName, string attributeName);
	bool addTableInfo(string tableName);
	bool checkTable(string tableName);
	bool checkIndex(string tableName, string attributeName);
	bool checkIndex(string indexName);
	string getIndexName(string tableName, string attributeName);
	int checkAttribute(string tableName, string attributeName);
	string getTableInfoPathName(string tableName);
	vector<string> getAttributeNames(string tableName);
	vector<int> getAttributeTypes(string tableName);
	int getAttributeType(string tableName, string attributeName);
	pair<int, int> getAttributePos(string tableName, string attributeName);
};

#endif