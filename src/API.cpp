//
//  API.cpp
//  DB_test
//
//  Created by Frank He on 11/2/15.
//  Copyright © 2015 Frank He. All rights reserved.
//

#include "API.hpp"
#include "CatalogManager.hpp"
#include "IndexManager.hpp"
#include "RecordManager.hpp"
#include "BufferManager.hpp"
#include "CommonHeader.hpp"
#include "Block.hpp"
#include <cstring>

bool displayComments = false;
bool displayRecordContents = false;
bool enableCheckUnique = false;

CatalogManager * cm;
IndexManager * im;
RecordManager * rm;
BufferManager * bm;


void init()
{
    cm = new CatalogManager();
    im = new IndexManager();
    rm = new RecordManager();
    bm = new BufferManager();
}

void APIQuit()
{
    delete bm;
    delete cm;
    delete rm;
    delete im;
    cout<<"exit!"<<endl;
    exit(0);
}

void APICreateTable(TransferArguments transferArg)
{
    if (cm->checkTable(transferArg.tableName)) {
        cout<<"ERROR: table already exists!"<<endl;
        return;
    }

    cm->createTableInfo(&transferArg);
    rm->createTable(transferArg.tableName);
    im->addIndex(transferArg.tableName, transferArg.primary_key);

}

int * offset;
Record *record;
void APIInsertInto(TransferArguments transferArg)
{
    if (!cm->checkTable(transferArg.tableName)) {
        cout<<"ERROR: table does not exist!"<<endl;
        return;
    }
    
    offset = new int();
    record = new Record();
    record->empty=false;
    
    vector<int> types=cm->getAttributeTypes(transferArg.tableName);
//    for (vector<int>::iterator it=types.begin(); it!=types.end(); it++) {
//        cout<<*it<<endl;
//    }
    if (types.size()!=transferArg.args.size()) {
        cout<<"ERROR: Insert record does not fit this table!"<<endl;
        return;
    }
    
    // update type of every arg
    for (int i = 0; i != types.size(); i++)
    {
        transferArg.args[i].type = types[i];
    }
    
    vector<string> attributeNames=cm->getAttributeNames(transferArg.tableName);
    
    memset(record->data, 0, sizeof(record->data));
    int current_pos=0;
    
    Value new_element;
    for (int i=0;i<types.size();i++)
    {
        //if unique or primary
        new_element.Vname=attributeNames[i];
        new_element.op="=";
        bool checkUnique = false;
        if (cm->checkAttribute(transferArg.tableName, attributeNames[i])>=1) {
            checkUnique =true;
        }
        
        if (types[i]==0) {
            memcpy(record->data+current_pos, &(transferArg.args[i].Vfloat), sizeof(transferArg.args[i].Vfloat));
            current_pos+=sizeof(transferArg.args[i].Vfloat);
            if (checkUnique) {
                new_element.Vfloat=transferArg.args[i].Vfloat;
            }
        } else
        if (types[i]==-1) {
            memcpy(record->data+current_pos, &(transferArg.args[i].Vint), sizeof(transferArg.args[i].Vint));
            current_pos+=sizeof(transferArg.args[i].Vint);
            if (checkUnique) {
                new_element.Vint=transferArg.args[i].Vint;
            }
        } else
        {
            memcpy(record->data+current_pos, transferArg.args[i].Vstring.c_str(),
                   strlen(transferArg.args[i].Vstring.c_str()));
            current_pos+=types[i];
            if (checkUnique) {
                new_element.Vstring=transferArg.args[i].Vstring;
            }
        }
        
        if (enableCheckUnique && checkUnique) {
            vector<Value> args;
            args.push_back(new_element);
            if (rm->selectRecord(transferArg.tableName, args, false)) {
                //true means already have
                cout<<"ERROR: this record has same unique value as others"<<endl;
                delete offset;
                delete record;
                return;
            }
        }

    }
    if (displayRecordContents) {
        cout<<"this is the record in int: "<<endl;
        for (int i=0; i<current_pos; i++) {
            cout<<int(record->data[i])<<' ';
        }
        cout<<endl;
    }
    
    rm->insertRecord(transferArg.tableName, *record, *offset);
    
    for (int i=0; i<attributeNames.size(); i++) {
        if (cm->checkIndex(transferArg.tableName, attributeNames[i])) {
            if (displayComments) cout<<"attribute:"<<attributeNames[i]<<" has index"<<endl;
            KeyValue keyValue(transferArg.args[i]);
            im->insertUpdate(transferArg.tableName, attributeNames[i], keyValue, *offset);
        }
    }
    
    delete offset;
    delete record;
    //not finished
}

void APISelect(TransferArguments transferArg)
{
    // check whether the table exists
    if (!cm->checkTable(transferArg.tableName))
    {
        cout << "The table '" << transferArg.tableName << "'does not exist." << endl;
        return;
    }
    
    // check whether every attribute exists in that table
    for (vector<Value>::iterator iter = transferArg.args.begin(); iter != transferArg.args.end(); iter++) {
        if (cm->checkAttribute(transferArg.tableName, iter->Vname) == -1)
        {
            cout << "The attribute '" << iter->Vname << "' does not exist in table '" << transferArg.tableName << "'." << endl;
            return;
        }
    }
    
    // update type of every arg
    for (vector<Value>::iterator iter = transferArg.args.begin(); iter != transferArg.args.end(); iter++) {
        iter->type = cm->getAttributeType(transferArg.tableName, iter->Vname);
    }
    
    // call RecordManager to select and print records
    rm->selectRecord(transferArg.tableName, transferArg.args, true);
}

void APIDropTable(TransferArguments transferArg)
{
    // check whether the table exists
    if (!cm->checkTable(transferArg.tableName))
    {
        cout << "The table '" << transferArg.tableName << "'does not exist." << endl;
        return;
    }
    
    // delete table information in catalog, record and index
    rm->dropTable(transferArg.tableName);
    im->dropIndexAll(transferArg.tableName);
    cm->dropTableInfo(transferArg.tableName);
}

void APICreateIndex(TransferArguments transferArg)
{
    // check whether the table exists
    if (!cm->checkTable(transferArg.tableName))
    {
        cout << "The table '" << transferArg.tableName << "'does not exist." << endl;
        return;
    }
    
    // check whether every attribute exists in that table
    for (vector<Value>::iterator iter = transferArg.args.begin(); iter != transferArg.args.end(); iter++) {
        if (cm->checkAttribute(transferArg.tableName, iter->Vname) == -1)
        {
            cout << "The attribute '" << iter->Vname << "' does not exist in table '" << transferArg.tableName << "'." << endl << endl;
            return;
        }
    }
    
    // check whether the index is already existent
    if (cm->checkIndex(transferArg.tableName, transferArg.args[0].Vname))
    {
        cout << "There is already index on '" << transferArg.args[0].Vname << "' in '" << transferArg.tableName << "'." << endl;
        return;
    }
    
    // update information in catalog
    cm->addIndexInfo(transferArg.indexName, transferArg.tableName, transferArg.args[0].Vname);
    
    // create new index
    im->addIndex(transferArg.tableName, transferArg.args[0].Vname);
}

void APIDropIndex(TransferArguments transferArg)
{
    // check whether the index exist
    if (!cm->checkIndex(transferArg.indexName))
    {
        cout << "There is no index named '" << transferArg.indexName << "'." << endl;
        return;
    }
    
    string table;
    string attr;
    cm->getIndexInfo(transferArg.indexName, table, attr);
    
    // delete the index
    im->dropIndex(table, attr);
    
    // delete information from catalog
    cm->dropIndexInfo(transferArg.indexName);
}

void APIDelete(TransferArguments transferArg)
{
    // check whether the table exists
    if (!cm->checkTable(transferArg.tableName))
    {
        cout << "The table '" << transferArg.tableName << "'does not exist." << endl;
        return;
    }
    
    // check whether every attribute exists in that table
    for (vector<Value>::iterator iter = transferArg.args.begin(); iter != transferArg.args.end(); iter++) {
        if (cm->checkAttribute(transferArg.tableName, iter->Vname) == -1)
        {
            cout << "The attribute '" << iter->Vname << "' does not exist in table '" << transferArg.tableName << "'." << endl;
            return;
        }
    }
    
    // update type of every arg
    for (vector<Value>::iterator iter = transferArg.args.begin(); iter != transferArg.args.end(); iter++) {
        iter->type = cm->getAttributeType(transferArg.tableName, iter->Vname);
    }
    
    rm->deleteRecord(transferArg.tableName, transferArg.args);
}
