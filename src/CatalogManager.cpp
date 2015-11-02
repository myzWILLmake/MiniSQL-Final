#include "CatalogManager.hpp"

/**
 * constructor of CatalogManager
 * it ensures that there are two files:'table_catalog' & 'index_catalog' in directory 'catalogs'
 */
CatalogManager::CatalogManager()
{
	createTableCatalog();
	createIndexCatalog();
}

/**
 * destructor of CatalogManager
 */
CatalogManager::~CatalogManager()
{
}

/**
 * open './catalogs/table_catalog' in app mode
 */
void CatalogManager::createTableCatalog()
{
	ofstream fout;
	fout.open(tableCatalog, fstream::app);
	if (fout) {
		fout.close();
	}
	else {
		cout << "Failed to create or open file 'table_catalog'" << endl;
	}
}

/**
 * open './catalogs/index_catalog' in app mode
 */
void CatalogManager::createIndexCatalog()
{
	ofstream fout;
	fout.open(indexCatalog, fstream::app);
	if (fout) {
		fout.close();
	}
	else {
		cout << "Failed to create or open file 'index_catalog'" << endl;
	}
}

/**
 * In a tableinfo file, first there are n(n=tableInfoLines) lines basic information about the table
 * Here, n=3:
 * 		line 1: table name
 * 		line 2: attribute number of the table
 * 		line 3: primary key of the table
 * Then each line is an attribute, there are four values deivided by tab:
 * 		name/type/unique/primary
 */
bool CatalogManager::createTableInfo(TransferArguments * args)
{
	string pathName = getTableInfoPathName(args->tableName);
	ofstream fout;
	fout.open(pathName, fstream::app);
	if (fout) {
		fout << args->tableName << endl;
		fout << args->args.size() << endl;
		fout << args->primary_key << endl;
		for (vector<Value>::iterator it = args->args.begin(); it != args->args.end(); it++) {
			fout << it->Vname << "\t" << it->type<<"\t"<<it->unique<<"\t"<<it->primary<<endl;
		}

		addIndexInfo(args->tableName+"_primary_key_index", args->tableName, args->primary_key);
		addTableInfo(args->tableName);
		fout.close();
		return true;
	}
	else {
		cout << "Failed to create/open file " << pathName << endl;
		return false;
	}
}

/**
 * Drop the tableinfo of a table needs 3 things to do:
 *		1.update index_catalog
 *		2.update table_catalog
 * 		3.delete tableinfo file of this table
 */
bool CatalogManager::dropTableInfo(string tableName)
{
	//update index_catalog
	ifstream fin;
	fin.open(indexCatalog);
	ofstream fout;
	fout.open(tempFile);
	if (fin && fout) {
		string s;
		vector<string> vs;
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			if (vs[1] != tableName) {
				fout << s<< endl;
			}
		}
		fin.close();
		fout.close();
		remove(indexCatalog.c_str());
		rename(tempFile.c_str(), indexCatalog.c_str());
	}
	else {
		cout << "Failed to open file 'index_catalog' or failed to create file 'temp_file'" << endl;
		return false;
	}

	//update table_catalog
	fin.open(tableCatalog);
	fout.open(tempFile);
	if (fin && fout) {
		string s;
		vector<string> vs;
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			if(s!=tableName) {
				fout << s << endl;
			}
		}
		fin.close();
		fout.close();
		remove(tableCatalog.c_str());
		rename(tempFile.c_str(), tableCatalog.c_str());
	}
	else {
		cout << "Failed to open file 'index_catalog' or failed to create file 'temp_file'" << endl;
		return false;
	}

	//delete the tableinfo file
	string pathName = getTableInfoPathName(tableName);
	if (remove(pathName.c_str()) == 0){
		//cout << "Delete file " + pathName + " success" << endl;
		return true;
	}
	else {
		cout << "Failed to delete file " << pathName << endl;
		return false;
	}
}

/**
 * Drop an index info by index name
 */
bool CatalogManager::dropIndexInfo(string indexName)
{
	ifstream fin;
	fin.open(indexCatalog, fstream::in);
	ofstream fout;
	fout.open(tempFile, fstream::trunc);
	if (fin && fout) {
		string s;
		vector<string> vs;
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			if (!(vs.size()>0 && vs[0] == indexName)) {
				fout << s << endl;
			}
		}
		fin.close();
		fout.close();
		remove(indexCatalog.c_str());
		rename(tempFile.c_str(), indexCatalog.c_str());
		return true;
	}
	else {
		cout << "Failed to open file 'index_catalog' or failed to create file 'temp_file'" << endl;
		return false;
	}
}

/**
 * Add  an index info to index_catalog
 * Each line of index_catalog is an index info
 * There are 3 values divided by tab:
 * 		index name/table name/attribute name
 */
bool CatalogManager::addIndexInfo(string indexName, string tableName, string attributeName)
{
	ofstream fout;
	fout.open(indexCatalog, ofstream::app);
	if (fout) {
		fout << indexName << "\t" << tableName << "\t" << attributeName << endl;
		fout.close();
		return true;
	}
	else {
		cout << "Failed to open file 'index_catalog'" << endl;
		return false;
	}
}

/**
 * Add a table info to table_catalog
 * Each line of table_catalog is an table info
 * There are only 1 value:
 * 		table name
 */
bool CatalogManager::addTableInfo(string tableName)
{
	ofstream fout;
	fout.open(tableCatalog, fstream::app);
	if (fout) {
		fout << tableName << endl;
		fout.close();
		return true;
	}
	else {
		cout << "Failed to open file 'table_catalog'" << endl;
		return false;
	}
}

/**
 * Check table in table_catalog
 * 		return true if this table exists
 * 		return false if this table doesn't exist
 */
bool CatalogManager::checkTable(string tableName)
{
	ifstream fin;
	fin.open(tableCatalog, fstream::in);
	if (fin) {
		string s;
		while (!fin.eof()){
			getline(fin, s);
			if (s == tableName) {
				fin.close();
				return true;
			}
		}
		fin.close();
		return false;
	}
	else {
		cout << "Failed to open file 'table_catalog'" << endl;
		return false;
	}
}

/**
 * Check index in index_catalog by table name and attribute name
 * 		return true if this index exists
 * 		return false if this index doesn't exist
 */
bool CatalogManager::checkIndex(string tableName, string attributeName)
{
	ifstream fin;
	fin.open(indexCatalog, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		while (!fin.eof()) {
			getline(fin, s);
			vs = split(s, "\t");
			if (vs.size()>=3 && vs[1]== tableName && vs[2]==attributeName) {
				fin.close();
				return true;
			}
		}
		fin.close();
		return false;
	}
	else {
		cout << "Failed to open file 'index_catalog'" << endl;
		return false;
	}
}

/**
 * Check index in index_catalog by index name
 * 		return true if this index exists
 * 		return false if this index doesn't exist
 */
bool CatalogManager::checkIndex(string indexName)
{
	ifstream fin;
	fin.open(indexCatalog, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			if (vs[0] == indexName) {
				fin.close();
				return true;
			}
		}
		fin.close();
		return false;
	}
	else {
		cout << "Failed to open file 'index_catalog'" << endl;
		return false;
	}
}

/**
 * Get index name in index_catalog by table name and attribute name
 * Before call this function, you need to call checkIndex to ensure the index exists
 */
string CatalogManager::getIndexName(string tableName, string attributeName)
{
	ifstream fin;
	fin.open(indexCatalog, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		while (!fin.eof()) {
			getline(fin, s);
			vs = split(s, "\t");
			if (vs[1] == tableName && vs[2] == attributeName) {
				fin.close();
				return vs[0];
			}
		}
		fin.close();
		return false;
	}
	else {
		cout << "Failed to open file 'index_catalog'" << endl;
		return false;
	}
}

/**
 * Check one attribute in a table
 * 		return 2: primary, also unique
 * 		return 1: unique
 * 		return 0: exist but not unique
 * 		return -1: not exist
 */
int CatalogManager::checkAttribute(string tableName, string attributeName)
{
	string pathName = getTableInfoPathName(tableName);
	ifstream fin;
	fin.open(pathName, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		for (int i = 0; i < tableInfoLines; i++) {
			getline(fin, s);
		}
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			if (vs[0] == attributeName) {
				fin.close();
				if (stoi(vs[3]) == 1)
					return 2;  //primary
				else if (stoi(vs[2]) == 1)
					return 1;  //unique
				else return 0; //does exist
			}
		}
		fin.close();
		return -1;   //not exist
	}
	else {
		cout << "Failed to open file "+pathName << endl;
		return -1;
	}
}

/**
 * Table info files are all in directory 'catalogs'
 * and is named as the table name
 */
string CatalogManager::getTableInfoPathName(string tableName)
{
	return "catalogs_"+tableName;
}

/**
 * Get all attributes' names of a table
 * When print the result of select statement,
 * we need to print the table head first
 */
vector<string> CatalogManager::getAttributeNames(string tableName)
{
	vector<string> attributeNames;
	string pathName = getTableInfoPathName(tableName);
	ifstream fin;
	fin.open(pathName, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		for (int i = 0; i < tableInfoLines; i++) {
			getline(fin, s);
		}
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			attributeNames.push_back(vs[0]);
		}
	}
	else {
		cout << "Failed to open file " + pathName << endl;
	}
	return attributeNames;
}

/**
 * Get all attributes' types of a table
 */
vector<int> CatalogManager::getAttributeTypes(string tableName)
{
	vector<int> attributeTypes;
	string pathName = getTableInfoPathName(tableName);
	ifstream fin;
	fin.open(pathName, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		for (int i = 0; i < tableInfoLines; i++) {
			getline(fin, s);
		}
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			attributeTypes.push_back(stoi(vs[1]));
		}
	}
	else {
		cout << "Failed to open file " + pathName << endl;
	}
	return attributeTypes;
}

/**
 * Get the type of an attribute by table name and attribute name
 */
int CatalogManager::getAttributeType(string tableName, string attributeName)
{
	string pathName = getTableInfoPathName(tableName);
	ifstream fin;
	fin.open(pathName, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;
		for (int i = 0; i < tableInfoLines; i++) {
			getline(fin, s);
		}
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			if (vs[0] == attributeName) {
				return stoi(vs[1]);
			}
		}
	}
	else {
		cout << "Failed to open file " + pathName << endl;
	}
	return -2;
}

/**
 * Get an attribute's position and length in a record
 * return pair<int, int>(position, size)
 * e.g. table student(sname char(10), sage int, gpa float)
 *		position of gpa=10+4=14
 *		size of gpa=4 because float value is 4 bytes
 */
pair<int, int> CatalogManager::getAttributePos(string tableName, string attributeName)
{
	int pos=0, size;
	pair<int, int>p(0,0);
	string pathName = getTableInfoPathName(tableName);
	ifstream fin;
	fin.open(pathName, fstream::in);
	if (fin) {
		string s;
		vector<string> vs;

		for (int i = 0; i < tableInfoLines; i++) {
			getline(fin, s);
		}
		while (!fin.eof()) {
			getline(fin, s);
			if (s.empty())
				break;
			vs = split(s, "\t");
			size = stoi(vs[1])>0 ? stoi(vs[1]) : 4;
			pos += size;
			if (vs[0] == attributeName) {
				p.first = pos-size;
				p.second = size;
				break;
			}
		}
		return p;
	}
	else {
		cout << "Failed to open file " + pathName << endl;
		return p;
	}
}
