#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include "CatalogManager.hpp"
#include "KeyValue.hpp"
#include <string>

#define UPPER_BOUND 0
#define LOWER_BOUND 1

using namespace std;

class RecordManager;

class IndexManager {
private:
	CatalogManager *cm;
	RecordManager *rm;

public:
	IndexManager();

	// add an index to an empty table or a table with data
	bool addIndex(string table, string attr);

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

	// retrieve index with only upper bound or lower bound
	vector<int> retrieve(string table, string attr, KeyValue bound, bool in, int mode);

};

#endif

