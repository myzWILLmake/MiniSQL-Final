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