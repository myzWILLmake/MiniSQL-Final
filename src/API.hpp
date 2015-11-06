//
//  API.hpp
//  DB_test
//
//  Created by Frank He on 11/2/15.
//  Copyright © 2015 Frank He. All rights reserved.
//

#ifndef API_hpp
#define API_hpp

#include "CatalogManager.hpp"
#include "IndexManager.hpp"
#include "RecordManager.hpp"
#include "BufferManager.hpp"
#include "CommonHeader.hpp"

extern bool displayComments;

class CatalogManager;
class IndexManager;
class RecordManager;
class BufferManager;
class TransferArguments;

extern BufferManager * bm;
extern CatalogManager * cm;
extern IndexManager * im;
extern RecordManager * rm;

void init();
void APICreateTable(TransferArguments);
void APIInsertInto(TransferArguments);
void APISelect(TransferArguments);
void APIDropTable(TransferArguments);
void APICreateIndex(TransferArguments);
void APIDropIndex(TransferArguments);
void APIDelete(TransferArguments);
void APIQuit();
#endif /* API_hpp */
