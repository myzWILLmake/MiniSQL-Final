#include "IndexManager.hpp"
#include "BPlusTree.hpp"

int IndexManager::addIndex(string table, string attr)
{
	BPlusTree BT(table, attr, CREATE_TREE);
}

bool IndexManager::dropIndexAll(string table)
{
	vector<string> attrs= cm.getAttributeNames(table);
	for (vector<string>::iterator iter = attrs.begin(); iter != attrs.end(); iter++)
	{
		if (cm.checkIndex(table, *iter))
		{
			dropIndex(table, *iter);
		}
	}
}

bool IndexManager::dropIndex(string table, string attr)
{
	if (!cm.checkIndex(table, attr))
	{
		cout << "Error visiting non-existent index on attribute \'" << attr << "\' of table \'" << table << "\'" << endl;
		exit(0);
	}
	BPlusTree BT(table, attr, FETCH_TREE);
	BT.destroy();
}

bool IndexManager::insertUpdate(string table, string attr, KeyValue value, int offset)
{
	if (!cm.checkIndex(table, attr))
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
	if (!cm.checkIndex(table, attr))
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
	if (!cm.checkIndex(table, attr))
	{
		cout << "Error visiting non-existent index on attribute \'" << attr << "\' of table \'" << table << "\'" << endl;
		exit(0);
	}
	BPlusTree BT(table, attr, FETCH_TREE);
	return BT.search(lower, lowerIn, upper, upperIn);
}	