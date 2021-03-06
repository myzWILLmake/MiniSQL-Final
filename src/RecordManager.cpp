#include "API.hpp"
#include "RecordManager.hpp"
#include <iomanip>
#include <algorithm>

RecordManager::RecordManager()
{
}


RecordManager::~RecordManager()
{
}

/**
 * call bm to create a record file in disk
 */
bool RecordManager::createTable(string tableName)
{
	if (bm->createTable(tableName) == true) {
		return true;
	}
	return false;
}

/**
 * call bm to delete a record file in disk
 */
bool RecordManager::dropTable(string tableName)
{
	if (bm->dropTable(tableName) == true) {
		return true;
	}
	return false;
}

/**
 * insert a record to a table
 */
bool RecordManager::insertRecord(string tableName, Record& record, int &offset)
{
	// buffer manager should ensure won't return null, if the table is empty, 
	// it should return a empty block but not null
	Block *block = bm->getFirstBlock(tableName);
	// get a block not full
	while (block!=NULL && block->recordNum>=EACH_BLOCK_RECORDS) {
		block = bm->getNextBlock(block, INSERT_MODE);
	}	
	int i;
	for ( i = 0; i < EACH_BLOCK_RECORDS; i++) { 
		if (block->records[i].empty == true) {
			record.empty = false;
			memcpy(&block->records[i], &record, RECORD_LEN);
			break;
		}
	}
	offset = EACH_BLOCK_RECORDS*block->blockNo+i;
	block->recordNum++;
	return true;
}

/**
 * delete records in a table with condition 
 */
bool RecordManager::deleteRecord(string tableName, vector<Value> &inputCondition)
{
	Condition condition;
	Condition::iterator map_it;
	vector<Value>::iterator vec_it;
	vector<int> result;
	Value lower, upper;
	bool lowerIn, upperIn;
	string attributeName;
	int flag1=0, flag2=0; 
	// flag1=0:need scan all records
	// flag2=1:find with index 
	// optimize the inputCondition to condition
	if(optimize(tableName, inputCondition, condition)){ 
		if(!condition.empty()){
			for(map_it=condition.begin(); map_it!=condition.end(); map_it++){
				attributeName=map_it->first;
				// try to find an index
				if(cm->checkIndex(tableName, attributeName)){
					lower.type=upper.type=-2;
					upperIn=lowerIn=false;
					for(vec_it=map_it->second.begin(); vec_it!=map_it->second.end(); vec_it++){
						// equivalence find
						if(vec_it->op=="="){
							result=im->retrieve(tableName, attributeName, *vec_it, true, *vec_it, true);
							flag2=1;
							break;
						}
						else if(vec_it->op==">"){
							lower=*vec_it;
						}
						else if(vec_it->op==">="){
							lower=*vec_it;
							lowerIn=true;
						}
						else if(vec_it->op=="<"){
							upper=*vec_it;
						}
						else if(vec_it->op=="<="){
							upper=*vec_it;
							upperIn=true;
						}
					}
					// interval find
					if(lower.type!=-2 && upper.type!=-2){
						result=im->retrieve(tableName, attributeName, lower, lowerIn, upper, upperIn);
						flag2=1;
					}
					else if(lower.type!=-2){
						result=im->retrieve(tableName, attributeName, lower, lowerIn, LOWER_BOUND);
						flag2=1;
					}
					else if(upper.type!=-2){
						result=im->retrieve(tableName, attributeName, upper, upperIn, UPPER_BOUND);
						flag2=1;
					}
					
					if(flag2==1){ //check the result
						int recordIndex;
						for(int i=0; i<result.size(); i++){
							//need this function from bufferManager
							Block *block= bm->getBlockByOffset(tableName, result[i]);
							recordIndex=result[i]-block->blockNo*EACH_BLOCK_RECORDS;
							if(match(block->records[recordIndex], condition, tableName)){
                                deleteIndex(block->records[recordIndex], tableName);
								block->records[recordIndex].empty=true;
								block->recordNum--;
							}
						}
						flag1=1;
						break;
					}
					else{
						continue;  //continue to find an index that can be used to do equivalence find or interval find
					}	
				}
			}
		}
		if(flag1==0){ //attribute in condition has no index, or condition is empty, then scan all records
			Block *block = bm->getFirstBlock(tableName);

			while (block != NULL) {
				for (int i = 0; i < EACH_BLOCK_RECORDS; i++) { 
					if (!block->records[i].empty) {
						if (condition.empty() || !condition.empty() && match(block->records[i], condition, tableName)) {
                            deleteIndex(block->records[i], tableName);
							block->records[i].empty = true;
							block->recordNum--;
						}
					}
				}
				block = bm->getNextBlock(block, SELECT_MODE);
			}
		}
	}
	return true;
}

/**
 * select records in a table with condition 
 */
bool RecordManager::selectRecord(string tableName, vector<Value> &inputCondition, bool shouldPrint)
{
	Condition condition;
	Condition::iterator map_it;
	vector<Value>::iterator vec_it;
	vector<int> result;
	Value lower, upper;
	bool lowerIn, upperIn;
	string attributeName;
	vector<int> attributeTypes = cm->getAttributeTypes(tableName);
	vector<string> attributeNames = cm->getAttributeNames(tableName);
	vector<int> printWidth;
	int flag1=0, flag2=0, flag3=0;
	// flag1=0:need scan all records 
	// flag2=1:find with index 
	// flag3=0:print the table head 
	// optimize the inputCondition to condition
	if(optimize(tableName, inputCondition, condition)){
		if(!condition.empty()){ 
			for(map_it=condition.begin(); map_it!=condition.end(); map_it++){
				attributeName=map_it->first;
				// try to find an index
				if(cm->checkIndex(tableName, attributeName)){
					lower.type=upper.type=-2;
					upperIn=lowerIn=false;
					for(vec_it=map_it->second.begin(); vec_it!=map_it->second.end(); vec_it++){
						// equivalence find
						if(vec_it->op=="="){
							result=im->retrieve(tableName, attributeName, *vec_it, true, *vec_it, true);
							flag2=1;
							break;
						}
						else if(vec_it->op==">"){
							lower=*vec_it;
						}
						else if(vec_it->op==">="){
							lower=*vec_it;
							lowerIn=true;
						}
						else if(vec_it->op=="<"){
							upper=*vec_it;
						}
						else if(vec_it->op=="<="){
							upper=*vec_it;
							upperIn=true;
						}
					}
					// interval find
					if(lower.type!=-2 && upper.type!=-2){
						result=im->retrieve(tableName, attributeName, lower, lowerIn, upper, upperIn);
						flag2=1;
					}
					else if(lower.type!=-2){
						result=im->retrieve(tableName, attributeName, lower, lowerIn, LOWER_BOUND);
						flag2=1;
					}
					else if(upper.type!=-2){
						result=im->retrieve(tableName, attributeName, upper, upperIn, UPPER_BOUND);
						flag2=1;
					}
					
					if(flag2==1){ //check the result
						int recordIndex;
						for(int i=0; i<result.size(); i++){
							//need this function from bufferManager
							Block *block = bm->getBlockByOffset(tableName, result[i]);
							recordIndex=result[i]-block->blockNo*EACH_BLOCK_RECORDS;
							if(match(block->records[recordIndex], condition, tableName)){
								if (flag3 == 0) {
									flag3 = 1;
									if (shouldPrint) {
										printHead(attributeNames, attributeTypes, printWidth);
									}
								}
								if (shouldPrint) {
									printRecord(block->records[recordIndex], attributeTypes, printWidth);
								}
							}
						}
						if (flag3 == 0){
							cout << "No records found." << endl;
							return true;
						}
		
						flag1=1;
						break;
					}
					else{
						continue;  //continue to find an index that can be used to do equivalence find or interval find
					}	
				}
			}
		}
		if(flag1==0){ //attribute in condition has no index, or condition is empty, then scan all records
			Block *block = bm->getFirstBlock(tableName);

			while (block != NULL) {
				for (int i = 0; i < EACH_BLOCK_RECORDS; i++) { 
					if (!block->records[i].empty) {
						if (condition.empty() || !condition.empty() && match(block->records[i], condition, tableName)) {
							if (flag3 == 0) {
								flag3 = 1;
								if (shouldPrint) {
									printHead(attributeNames, attributeTypes, printWidth);
								}
							}
							if (shouldPrint) {
								printRecord(block->records[i], attributeTypes, printWidth);
							}
						}
					}
				}
				block = bm->getNextBlock(block, SELECT_MODE);
			}
			if (flag3 == 0){
				cout << "No records found." << endl;
				return true;
			}
		}
	}
	return false;
}

/**
 * get all values of an attribute in a table and offsets of those records
 * here, offset = blockNo*EACH_BLOCK_RECORDS+recordIndex
 * if offset = 20, then this record is at 20th record box in disk table file,
 * not means the 20th record in disk table file
 */
bool RecordManager::getKeysOffsets(string tableName, string attributeName, vector<pair<Value, int>>& tmp)
{
    int type, size, pos, offset;
    pair<int, int>p = cm->getAttributePos(tableName, attributeName);
    
    if (p.first < 0 || p.second < 0) {
        cout << "Error" << endl;
        return false;
    }
    pos = p.first;
    size = p.second;
    type = cm->getAttributeType(tableName, attributeName);
    
    Value value= Value(type);
    tmp.clear();
    Block *block =  bm->getFirstBlock(tableName);
    while (block != NULL) {
        for (int i = 0; i < EACH_BLOCK_RECORDS; i++) {
            if (block->records[i].empty == false) {
                offset = (block->blockNo*EACH_BLOCK_RECORDS) + i;
				// cout << offset << endl;
                if (type == -1) {
                    memcpy(&value.Vint, block->records[i].data + pos, size);
                }
                else if (type == 0) {
                    memcpy(&value.Vfloat, block->records[i].data + pos, size);
                }
                else {
                    char * sTmp = new char[size];
                    memcpy(sTmp, block->records[i].data + pos, size);
                    value.Vstring = sTmp;
                }
                tmp.push_back(pair<Value, int>(value, offset));
            }
        }
        block = bm->getNextBlock(block, SELECT_MODE);
    }
    return true;
}

/**
 * print records of select results
 */
void RecordManager::printRecord(const Record & record, vector<int> &attributeTypes, vector<int> &printWidth)
{
	int type, size;
	cout << "\t";
	for (int i = 0, pos=0; i < attributeTypes.size(); i++, pos+=size) {
		type = attributeTypes[i];
		size = type < 1 ? 4 : type;
	
		if (type == -1)
			cout << setiosflags(ios::left)<< setw(printWidth[i]) << *(int *)(record.data + pos);
		else if (type == 0)
			cout << setiosflags(ios::left) << setw(printWidth[i]) << *(float *)(record.data + pos);
		else {
			char * sTmp = new char[size+1];
			memcpy(sTmp, record.data + pos, size);
			sTmp[size] = '\0';
			//value.Vstring = sTmp;
			//cout << setiosflags(ios::left) << setw(printWidth[i]) << (char *)(record.data + pos);
			cout << setiosflags(ios::left) << setw(printWidth[i]) << sTmp;
		}
	}
	cout << endl;
}

void RecordManager::deleteIndex(const Record & record, string tableName)
{
    int type, size;
    
    vector<int> attributeTypes = cm->getAttributeTypes(tableName);
    vector<string> attributes = cm->getAttributeNames(tableName);
    
    for (int i = 0, pos=0; i < attributeTypes.size(); i++, pos+=size) {
        type = attributeTypes[i];
        size = type < 1 ? 4 : type;
        
        if (cm->checkIndex(tableName, attributes[i]))
        {
            Value temp(type);
            if (type == -1)
                temp.Vint = *(int *)(record.data + pos);
            else if (type == 0)
                temp.Vfloat = *(float *)(record.data + pos);
            else
            {
                string s((char *)(record.data+pos));
                temp.Vstring = s;
            }
            im->deleteUpdate(tableName, attributes[i], temp);
        }
    }
}

/**
 * print table head of select results
 */
void RecordManager::printHead(vector<string> &attributeNames, vector<int> &attributeTypes, vector<int> &printWidth)
{
	int width;
	cout << "\t";
	printWidth.clear();
	for (int i = 0; i < attributeNames.size(); i++) {
		width = max(attributeTypes[i] < 1 ? 8 : attributeTypes[i]+2, (int)attributeNames[i].length()+2);
		printWidth.push_back(width);
		cout << setiosflags(ios::left) << setw(width) << attributeNames[i];
	}
	cout << endl;
}

/**
 * optimize the inputCondition to condition
 * there may be many redundant or conflicting conditions
 * e.g. age=9 and age!=10  --> age=9
 *		age>8 and age >5   --> age>8
 *		...
 * after optimize, the condition is a map from attribute name to a vector of Value
 * each vector is like below:
 *		1.there is one and only one Value with op=="="
 *		2.there is at most one Value with op==">" or op==">="
 *		  there is at most one Value with op=="<" or op=="<="
 *		  there is no Value with op=="="
 *		  there is some Value with op=="<>"
 * return false if no records can be found on this condition
 */
bool RecordManager::optimize(string tableName, vector<Value>& inputCondition, Condition & condition)
{
	string attributeName, op;
	// 0     1     2    3      4     5   6 ...
	// >    >=     <    <=     ==    <> 
	vector<Value> attrCondition(5);   //condition on one attribute
	for (int i = 0; i < attrCondition.size(); i++) {
		attrCondition[i].type = -2; // type=-2 means this Value has not been assigned
	}
	Condition::iterator map_it;
	vector<Value>::iterator vec_it;
	int type;
	// inputCondition --> condition
	for (int i = 0; i < inputCondition.size(); i++) {
		attributeName = inputCondition[i].Vname;
		op = inputCondition[i].op;
		if (cm->checkAttribute(tableName, attributeName)>=0) {
			type = cm->getAttributeType(tableName, attributeName);
			if (type == inputCondition[i].type) {
				map_it = condition.find(attributeName);
				if (map_it == condition.end()) {
					condition.insert(Condition::value_type(attributeName, attrCondition));
				}
				map_it = condition.find(attributeName);
				if (op == ">") {
					if (map_it->second.at(0).type == -2) {
						map_it->second.at(0) = inputCondition[i];
					}
					else if (inputCondition[i] > map_it->second.at(0)) {
						map_it->second.at(0) = inputCondition[i];
					}
				}
				else if (op == ">=") {
					if (map_it->second.at(1).type == -2) {
						map_it->second.at(1) = inputCondition[i];
					}
					else if (inputCondition[i] > map_it->second.at(1)) {
						map_it->second.at(1) = inputCondition[i];
					}
				}
				else if (op == "<") {
					if (map_it->second.at(2).type == -2) {
						map_it->second.at(2) = inputCondition[i];
					}
					else if (inputCondition[i] < map_it->second.at(2)) {
						map_it->second.at(2) = inputCondition[i];
					}
				}
				else if (op == "<=") {
					if (map_it->second.at(3).type == -2) {
						map_it->second.at(3) = inputCondition[i];
					}
					else if (inputCondition[i] < map_it->second.at(3)) {
						map_it->second.at(3) = inputCondition[i];
					}
				}
				else if (op == "=") {
					if (map_it->second.at(4).type == -2) {
						map_it->second.at(4) = inputCondition[i];
					}
					else if (inputCondition[i] != map_it->second.at(4)) {
						return false;  // age=3 and age=5
					}
				}
				else if (op == "<>") {
					if (map_it->second.at(4).type != -2 && inputCondition[i] == map_it->second.at(4)) {
						return false;  // age=3 and age!=3
					}
					map_it->second.push_back(inputCondition[i]);
				}
			}
			else {
				cout << "In where statement: \'" + attributeName + "\' 's type is wrong" << endl;
				return false;
			}
		}
		else {
			cout << "In where statement: \'" + attributeName + "\' is not an attribute of table \'" + tableName + "\'" << endl;
			return false;
		}
	}	

	Value equal, lower, upper;
	for (map_it = condition.begin(); map_it != condition.end(); map_it++) {
		// handle = and <>, if has =, after this handle, there will not be <>
		if(map_it->second.at(4).type != -2){ // has =
			if(map_it->second.size()>5){  // if has <>
				for (vec_it=map_it->second.begin()+5; vec_it < map_it->second.end(); vec_it++) {   
					if (map_it->second.at(4) == *vec_it ){   // age=9 and age!=9
						return false;
					}
				}
				map_it->second.erase(map_it->second.begin()+5, map_it->second.end());
			}
		}
		
		if (map_it->second.at(0).type != -2 && map_it->second.at(1).type != -2) {  // has > and >=
			lower = map_it->second.at(0) >= map_it->second.at(1)? map_it->second.at(0): map_it->second.at(1);
			//age>10 and age>=9, delete age>=9
			//age>9 and age>=10, delete age>9
		}
		else if (map_it->second.at(0).type != -2) {  // has >
			lower = map_it->second.at(0);
		}
		else if (map_it->second.at(1).type != -2) {  // has >=
			lower = map_it->second.at(1);
		}
		else {
			lower.type = -2;
		}

		if (map_it->second.at(2).type != -2 && map_it->second.at(3).type != -2) { // has < and <=
			upper = map_it->second.at(2) <= map_it->second.at(3)?map_it->second.at(2):map_it->second.at(3);
			//age<9 and age<=10, delete age<=10
			//age<10 and age<=9, delete age<10
		}
		else if (map_it->second.at(2).type != -2) {  // has <
			upper = map_it->second.at(2);
		}
		else if(map_it->second.at(3).type != -2) {  // has <=
			upper = map_it->second.at(3);
		}
		else {

			upper.type = -2;		}

		if (lower.type != -2 && upper.type != -2) {  // has lower and upper
			if (lower > upper ) {  // age>10 and age<9
				return false;
			}
			else if (lower == upper) {
				if (lower.op == ">=" && upper.op == "<=") {  // age>=9 and age<=9  --> age=9
					equal = lower;
					equal.op = "=";
					if (map_it->second.at(4).type != -2) {  // also has =
						if (map_it->second.at(4) != equal) {  // age>=9 and age<=9 and age=8
							return false;
						}
						else { //age>=9 and age<=9 and age=9
							map_it->second.clear();
							map_it->second.push_back(equal);
							continue;
						}
					}
					else if(map_it->second.size()>5){  // if has <>
						for (int i = 5; i < map_it->second.size(); i++) {   
							if (equal == map_it->second.at(i)) {   // age>=9 and age<=9 and age<>9
								return false;
							}
						}
						//age>=9 and age<=9 and age!=8 ... and age!=6
						map_it->second.clear();
						map_it->second.push_back(equal);
						continue;
					}
				}
				else {   // age>=9 and age <9 or age>9 and age<=9....
					return false;
				}
			}
			else{  // age > / >= 9 and age < / <= 10
				if(map_it->second.at(4).type!=-2){  // has =
					if(lower.op==">" && upper.op=="<" && map_it->second.at(4)>lower && map_it->second.at(4)<upper
					||lower.op==">=" && upper.op=="<" && map_it->second.at(4)>=lower && map_it->second.at(4)<upper
					||lower.op==">" && upper.op=="<=" && map_it->second.at(4)>lower && map_it->second.at(4)<=upper
					||lower.op==">=" && upper.op=="<=" && map_it->second.at(4)>=lower && map_it->second.at(4)<=upper) {
						// age = x and x is between lower and upper
						equal=map_it->second.at(4);
						map_it->second.clear();
						map_it->second.push_back(equal);
						continue;
					}  
				}
				else{  //has not =
					map_it->second.erase(map_it->second.begin(), map_it->second.begin()+5); // may have age!=x
					map_it->second.push_back(lower);
					map_it->second.push_back(upper);
					continue;
				}
			}
		}
		else if(lower.type!=-2){  // age>9 or age>=9
			if(map_it->second.at(4).type!=-2){  // has =
				if(lower.op==">" && map_it->second.at(4)>lower
				||lower.op==">=" && map_it->second.at(4)>=lower){  // age>9 and age = 10 or age>=9 and age=9... 
					equal=map_it->second.at(4);
					map_it->second.clear();
					map_it->second.push_back(equal);
					continue;
				}
				else{  // age>9 and age=9 or age>=9 and age =8
					return false;
				}
			}
			else{ // has not =
				map_it->second.erase(map_it->second.begin(), map_it->second.begin()+5); // may have age!=x
				map_it->second.push_back(lower);
				continue;
			}	
		}
		else if(upper.type!=-2){
			if(map_it->second.at(4).type!=-2){  // has =
				if(upper.op=="<" && map_it->second.at(4)<upper
				||upper.op=="<=" && map_it->second.at(4)<=upper){ // age<9 and age=8 or age<=9 and age=9 ....
					equal=map_it->second.at(4);
					map_it->second.clear();
					map_it->second.push_back(equal);
					continue;
				}
				else{  // age<9 and age=10 or age<=9 and age=10
					return false;
				}
			}
			else{ // has not =
				map_it->second.erase(map_it->second.begin(), map_it->second.begin()+5); // may have age!=x
				map_it->second.push_back(upper);
				continue;
			}
		}
		else{  // no lower and no upper
			if(map_it->second.at(4).type!=-2){  // has =		
				equal=map_it->second.at(4);
				map_it->second.clear();
				map_it->second.push_back(equal);
				continue;
			}
			else{
				map_it->second.erase(map_it->second.begin(), map_it->second.begin()+5); // may have age!=x
				continue;
			}	
		}
	}
	return true;
}

/**
 * return true if reocord match condition 
 */
bool RecordManager::match(const Record &record, Condition & condition, string tableName)
{
	string attributeName;
	int type, size, pos;
	bool compareResult;
	
	Condition::iterator map_it;
	vector<Value>::iterator vec_it;
	for (map_it=condition.begin(); map_it != condition.end(); map_it++) {
		attributeName = map_it->first;
		pair<int, int>p = cm->getAttributePos(tableName, attributeName);

		if (p.first < 0 || p.second < 0) {
			cout << "Error" << endl;
			return false;
		}
		pos = p.first;
		size = p.second;

		type = cm->getAttributeType(tableName, attributeName);

		Value value = Value(type);
		if (type == -1) {
			memcpy(&value.Vint, record.data + pos, size);
		}
		else if (type == 0) {
			memcpy(&value.Vfloat, record.data + pos, size);
		}
		else {
			char *sTmp = new char(type);
			memcpy(sTmp, record.data + pos, size);
			value.Vstring = sTmp;
		}
		for (vec_it = map_it->second.begin(); vec_it != map_it->second.end(); vec_it++) {
			if (!compare(value, *vec_it, vec_it->op)) {
				return false;
			}
		}
	}
	return true;
}

/**
 * compare two values
 */
bool RecordManager::compare(Value & v1, Value & v2, string op)
{
	if (op == ">") {
		return v1>v2;
	}
	else if (op == ">=") {
		return v1>=v2;
	}
	else if (op == "<") {
		return v1 < v2;
	}
	else if (op == "<=") {
		return v1 <= v2 ;
	}
	else if (op == "=") {
		return v1 == v2;
	}
	else if (op == "<>") {
		return v1 != v2;
	}
	return false;
}
