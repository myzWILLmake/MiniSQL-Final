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

class CatalogManager;
class IndexManager;
class RecordManager;
class BufferManager;

extern BufferManager * bm;
extern CatalogManager * cm;
extern IndexManager * im;
extern RecordManager * rm;

void init();
#endif /* API_hpp */
