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

public:
    // constructor
    IndexManager();

    // add an index to an empty table or a table with data
    // @return      result of operation
    bool addIndex(string table, string attr);

    // drop all indices on the table
    // @return      result of operation
    bool dropIndexAll(string table);

    // drop one index
    // @return      result of operation
    bool dropIndex(string table, string attr);

    // update index when inserting a record
    // @param   value   the value of key
    // @param   offset  the offset of a record
    // @return          result of operation
    bool insertUpdate(string table, string attr, KeyValue value, int offset);

    // update index when deleting a record
    // @param   value   the value of key
    // @return          result of operation
    bool deleteUpdate(string table, string attr, KeyValue value);

    // retrieve offsets of records within a range
    // @param   lower   the lower bound of the range
    // @param   lowerIn inclusive or exclusive, 'true' stands for inclusive
    // @param   upper   the upper bound of the range
    // @param   upperIn inclusive or exclusive, 'true' stands for inclusive
    // @return          offsets of records
    vector<int> retrieve(string table, string attr, KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn);

    // retrieve offsets of records within single-buond range
    // @param   bound   the value of the bound
    // @param   mode    indicating upper bound or lower bound, must be UPPER_BOUND or LOWER_BOUND
    // @return          offsets of records
    vector<int> retrieve(string table, string attr, KeyValue bound, bool in, int mode);

};

#endif /* INDEXMANAGER_H */

