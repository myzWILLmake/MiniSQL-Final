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
    
    memset(record->data, 0, sizeof(record->data));
    int current_pos=0;
    for (int i=0;i<types.size();i++)
    {
        if (types[i]==0) {
            memcpy(record->data+current_pos, &(transferArg.args[i].Vfloat), sizeof(transferArg.args[i].Vfloat));
            current_pos+=sizeof(transferArg.args[i].Vfloat);
        } else
        if (types[i]==-1) {
            memcpy(record->data+current_pos, &(transferArg.args[i].Vint), sizeof(transferArg.args[i].Vint));
            current_pos+=sizeof(transferArg.args[i].Vint);
        } else
        {
            memcpy(record->data+current_pos, transferArg.args[i].Vstring.c_str(),
                   strlen(transferArg.args[i].Vstring.c_str()));
            current_pos+=32;
        }
    }
    cout<<"this is the record in int: "<<endl;
    for (int i=0; i<current_pos; i++) {
        cout<<int(record->data[i])<<' ';
    }
    cout<<endl;
    
    rm->insertRecord(transferArg.tableName, *record, *offset);
    
    vector<string> attributeNames=cm->getAttributeNames(transferArg.tableName);
    for (int i=0; i<attributeNames.size(); i++) {
        if (cm->checkIndex(transferArg.tableName, attributeNames[i])) {
            cout<<"attribute:"<<attributeNames[i]<<" has index"<<endl;
            KeyValue keyValue(transferArg.args[i]);
            im->insertUpdate(transferArg.tableName, attributeNames[i], keyValue, *offset);
        }
    }
    //not finished
}
