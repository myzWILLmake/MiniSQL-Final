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

CatalogManager * cm;
IndexManager * im;
RecordManager * rm;
BufferManager * bm;


void init()
{
    CatalogManager * cm = new CatalogManager();
    IndexManager * im = new IndexManager();
    RecordManager * rm = new RecordManager();
    BufferManager * bm = new BufferManager();
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
