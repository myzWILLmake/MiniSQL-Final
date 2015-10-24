#include "IndexManager.hpp"
#include "BPlusTree.hpp"
#include "RecordManager.hpp"
#include <utility>
#include <algorithm>

using namespace std;

IndexManager::IndexManager():cm()
{
}

bool IndexManager::addIndex(string table, string attr)
{
	vector<pair<Value, int> > tmp;
	vector<pair<KeyValue, int> > init;
	BPlusTree BT(table, attr, CREATE_TREE);
	rm->getKeysOffsets(table, attr, tmp);
	if (tmp.size() > 0)
	{
		for (vector<pair<Value, int> >::iterator iter = tmp.begin(); iter != tmp.end(); iter++)
		{
			init.push_back(make_pair(KeyValue(iter->first), iter->second));
		}
		sort(init.begin(), init.end());
		BT.build(init);
	}
	return true;
}

bool operator < (const pair<KeyValue, int> &p1, const pair<KeyValue, int> &p2)
{
	return p1.first < p2.first;
}

bool IndexManager::dropIndexAll(string table)
{
	vector<string> attrs= cm->getAttributeNames(table);
	for (vector<string>::iterator iter = attrs.begin(); iter != attrs.end(); iter++)
	{
		if (cm->checkIndex(table, *iter))
		{
			dropIndex(table, *iter);
		}
	}
	return true;
}

bool IndexManager::dropIndex(string table, string attr)
{
	if (!cm->checkIndex(table, attr))
	{
		cout << "Error visiting non-existent index on attribute \'" << attr << "\' of table \'" << table << "\'" << endl;
		exit(0);
	}
	BPlusTree BT(table, attr, FETCH_TREE);
	BT.destroy();
	return true;
}

bool IndexManager::insertUpdate(string table, string attr, KeyValue value, int offset)
{
	if (!cm->checkIndex(table, attr))
	{
		cout << "Error visiting non-existent index on attribute \'" << attr << "\' of table \'" << table << "\'" << endl;
		exit(0);
	}
	BPlusTree BT(table, attr, FETCH_TREE);
	switch (BT.insert(value, offset))
	{
	case INSERT_SUCCESS:
		return true;
	case INSERT_DUPLICATE:
	case INSERT_FAILURE:
	default:
		return false;
	}
}

bool IndexManager::deleteUpdate(string table, string attr, KeyValue value)
{
	if (!cm->checkIndex(table, attr))
	{
		cout << "Error visiting non-existent index on attribute \'" << attr << "\' of table \'" << table << "\'" << endl;
		exit(0);
	}
	BPlusTree BT(table, attr, FETCH_TREE);
	switch (BT.remove(value))
	{
	case REMOVE_SUCCESS:
		return true;
	case REMOVE_UNFOUND:
	default:
		return false;
	}
}

vector<int> IndexManager::retrieve(string table, string attr, KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn)
{
	if (!cm->checkIndex(table, attr))
	{
		cout << "Error visiting non-existent index on attribute \'" << attr << "\' of table \'" << table << "\'" << endl;
		exit(0);
	}
	BPlusTree BT(table, attr, FETCH_TREE);
	return BT.search(lower, lowerIn, upper, upperIn);
}

vector<int> IndexManager::retrieve(string table, string attr, KeyValue bound, bool in, int mode)
{
	switch (mode)
	{
	case UPPER_BOUND:
		return retrieve(table, attr, KeyValue(bound.type, MIN_VALUE), true, bound, in);
	case LOWER_BOUND:
		return retrieve(table, attr, bound, in, KeyValue(bound.type, MAX_VALUE), true);
	default:
		return vector<int>();
	}
}

