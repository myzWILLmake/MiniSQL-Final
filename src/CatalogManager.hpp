#ifndef CATALOGMANAGER_H
#define CATALOGMANAGER_H

#include "CommonHeader.hpp"
#include "Interpreter.hpp"
#include <fstream>
#include <iostream>

using namespace std;
const int tableInfoLines = 3;
const string tableCatalog = "catalogs_table_catalog";
const string indexCatalog = "catalogs_index_catalog";
const string tempFile = "catalogs_temp_file";

class CatalogManager
{
public:
	CatalogManager();
	~CatalogManager();
	void createTableCatalog(); // create the file "catalogs_table_catalog"
	void createIndexCatalog(); // create the file "catalogs_index_catalog"
	bool createTableInfo(TransferArguments *args); // create info file of a table
	bool dropTableInfo(string tableName); // drop info file of a table by table name
	bool dropIndexInfo(string indexName); // drop info of an index by index name
	bool addIndexInfo(string indexName, string tableName, string attributeName); // add an index info with index name, table name and attribute name as input
	bool addTableInfo(string tableName); // add a table info with table name as input
	bool checkTable(string tableName); // check if this table exists, return true if it exists(vice versa)
	bool checkIndex(string tableName, string attributeName); // check if this index exists, with table name and attribute name as input
	bool checkIndex(string indexName); // check if this index exists, with index name as input
	string getIndexName(string tableName, string attributeName); // return index name corresponding to the table name and attribute name, invoked when it does exist
	int checkAttribute(string tableName, string attributeName); // check if this table has this attribute
	string getTableInfoPathName(string tableName); // return the path name of a table info file with table name as input
	vector<string> getAttributeNames(string tableName); // return all attribute names of table in order
	vector<int> getAttributeTypes(string tableName); // return all attribute types of table in order
	int getAttributeType(string tableName, string attributeName); // get type of an attribute by table name and attribute name
	pair<int, int> getAttributePos(string tableName, string attributeName); // get start position and length of an attribute in a record
};

#endif
