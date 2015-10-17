#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include "CatalogManager.h"
#include "KeyValue.hpp"
#include <string>

using namespace std;

class IndexManager {

	CatalogManager cm;

	// add an index to an empty table or a table with data
	int addIndex(string table, string attr);

	// drop all indices on the table
	bool dropIndexAll(string table);

	// drop a specific index
	bool dropIndex(string table, string attr);

	// update index when inserting a record
	bool insertUpdate(string table, string attr, KeyValue value, int offset);

	// update index when deleting a record
	bool deleteUpdate(string table, string attr, KeyValue value);

	// retrieve index
	vector<int> retrieve(string table, string attr, KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn);
};

#endif

