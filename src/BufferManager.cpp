//
//  BufferManager.cpp
//  MiniSQLBM
//
//  Created by Yunze on 15/10/14.
//  Copyright © 2015年 Yunze. All rights reserved.
//

#include <fstream>

#include "BufferManager.hpp"

#ifdef _WIN32
    std::string SEP = "\\";
#elif __APPLE__
    std::string SEP = "/";
#else
    std::string SEP = "/";
#endif

/* -------- private functions -------- */

std::string BufferManager::formatNotoString(int numNo) {
    int numTmp = numNo;
    char numChar0 = '0' + numTmp / 1000;
    numTmp %= 1000;
    char numChar1 = '0' + numTmp / 100;
    numTmp %= 100;
    char numChar2 = '0' + numTmp / 10;
    numTmp %= 10;
    char numChar3 = '0' + numTmp;
    std::string tmpString = "";
    tmpString += numChar0;
    tmpString += numChar1;
    tmpString += numChar2;
    tmpString += numChar3;
    return tmpString;
}

Block* BufferManager::getBlockFromRecordBlockPool() {
    Block* returnPtr = NULL;
    Block* currentBlock = recordBlockPool[currentRBPPos];
    if (currentBlock->active) {
        closeBlock(currentBlock);
    }
    currentBlock->active = true;
    returnPtr = currentBlock;
    currentRBPPos = (currentRBPPos + 1) % RECORD_BLOCK_NUM;
    return returnPtr;
}

void BufferManager::closeBlock(Block *block) {
    // If the block is not used
    if (!block->active) {
        return;
    }
    std::ofstream fout;
    std::string path = "table" + SEP + block->tableName + SEP;
    std::string fileName = block->tableName + "_" + formatNotoString(block->blockNo);
    fout.open(path + fileName, std::ios::out | std::ios::binary);
    if (fout) {
        fout.write((char*)block->records, sizeof(block->records));
        fout.close();
    }
    // Set the block unactive
    block->active = false;
    // Erase the block from the map
    std::map<std::string, Block*>::iterator it = recordBlockMap.find(fileName);
    recordBlockMap.erase(it);
}

IndexBlock* BufferManager::getBlockFromIndexBlockPool() {
    IndexBlock* returnPtr = NULL;
    IndexBlock* currentBlock = indexBlockPool[currentIBPPos];
    while (currentBlock->active && currentBlock->pin) {
        currentIBPPos = (currentIBPPos + 1) % INDEX_BLOCK_NUM;
        currentBlock = indexBlockPool[currentIBPPos];
    }
    if (currentBlock->active) {
        closeIndexBlock(currentBlock);
    }
    currentBlock->active = true;
    returnPtr = currentBlock;
    currentIBPPos = (currentIBPPos + 1) % INDEX_BLOCK_NUM;
    return returnPtr;
}

void BufferManager::closeIndexBlock(IndexBlock *block) {
    // If the block is not used
    if (!block->active) {
        return;
    }
    std::ofstream fout;
    std::string path = "index" + SEP + block->tableName + SEP + block->attrName + SEP;
    std::string fileName = block->tableName + "_" + block->attrName + "_" + formatNotoString(block->blockNo);
    fout.open(path + fileName, std::ios::out | std::ios::binary);
    if (fout) {
        fout.write(block->address, 0x1000);
        fout.close();
    }
    // Reset the indexBlock
    block->active = false;
    block->pin = false;
    block->dirty = false;
    block->tableName = "";
    block->attrName = "";
    block->blockNo = 0;
    // Erase the block from the map
    std::map<std::string, IndexBlock*>::iterator it = indexBlockMap.find(fileName);
    indexBlockMap.erase(it);
}

/* -------- public functions -------- */

BufferManager::BufferManager() {
#if defined _MSC_VER
    _mkdir("table");
    _mkdir("index");
#elif defined __GNUC__
    mkdir("table", 0777);
    mkdir("index", 0777);
#endif
    // Initial the position for the pool
    currentIBPPos = 0;
    currentRBPPos = 0;
    // Initial recordBlockPool
    for (int i=0; i<RECORD_BLOCK_NUM; i++) {
        Block* nodeBlock = new Block();
        recordBlockPool[i] = nodeBlock;
    }
    // Initial indexBlockPool and initial memory space for indexBlock
    for (int i=0; i<INDEX_BLOCK_NUM; i++) {
        IndexBlock* nodeBlock = new IndexBlock();
        char* memptr = (char*) malloc(0x1000);
        nodeBlock->address = memptr;
        indexBlockPool[i] = nodeBlock;
    }
}

BufferManager::~BufferManager() {
    for (int i=0; i<RECORD_BLOCK_NUM; i++) {
        closeBlock(recordBlockPool[i]);
        delete recordBlockPool[i];
    }
    for (int i=0; i<INDEX_BLOCK_NUM; i++) {
        closeIndexBlock(indexBlockPool[i]);
        free(indexBlockPool[i]->address);
        delete indexBlockPool[i];
    }
}

/**
 * Initial the first block file to create a table. A boolean value to show the
 * result. True is success, false is failure.
 * @param   tablaName   string of a file name
 * @return              result for the operation
 */
bool BufferManager::createTable(std::string tableName) {
    bool returnBool = false;
    std::string path = "table" + SEP + tableName + SEP;
#if defined _MSC_VER
    _mkdir(path.c_str());
#elif defined __GNUC__
    mkdir(path.c_str(), 0777);
#endif
    std::ofstream fout;
    fout.open(path + tableName + "_" + formatNotoString(0), std::ios::out | std::ios::binary);
    if (fout) {
        Block* tmpBlock = new Block();
        for (int i=0; i < EACH_BLOCK_RECORDS; i++) {
            tmpBlock->records[i].empty = true;
        }
        fout.write((char*)tmpBlock->records, sizeof(tmpBlock->records));
        delete tmpBlock;
        returnBool = true;
        fout.close();
    }
    return returnBool;
}

/**
 * Prepartion to drop a table. A boolean value to show the result. True is
 * success, false is failure.
 * @param   tableName   string of a table name
 * @return              result for the operation
 */
bool BufferManager::dropTable(std::string tableName) {
    // Erase all the data from the map and pool
    for (int i=0; i<MAX_BLOCK_FILE_NUM; i++) {
        std::string tmpFileName = tableName + "_" + formatNotoString(i);
        std::map<std::string, Block*>::iterator it = recordBlockMap.find(tmpFileName);
        if (it == recordBlockMap.end())
            break;
        Block* tmpBlock = recordBlockMap[tmpFileName];
        tmpBlock->active = false;
        recordBlockMap.erase(it);
    }
    std::string path = "table" + SEP + tableName + SEP;
    // Delete the files
    for (int i=0; i<MAX_BLOCK_FILE_NUM; i++) {
        std::string tmpFileName = path + tableName + "_" + formatNotoString(i);
        const char* tmpFileNameC = tmpFileName.c_str();
        int res = remove(tmpFileNameC);
        if (res)
            break;
    }
#if defined _MSC_VER
    _rmdir(path.c_str());
#elif defined __GNUC__
    rmdir(path.c_str());
#endif
    return true;
}

/**
 * To get the first block of a table. Return a block pointer pointing to a
 * block which is prepared.
 * @param   tableName   string of a table name
 * @return              block pointer
 */
Block* BufferManager::getFirstBlock(std::string tableName) {
    Block* returnPtr = NULL;
    std::string path = "table" + SEP + tableName + SEP;
    std::string fileName = tableName + "_" + formatNotoString(0);
    if (recordBlockMap.find(fileName) != recordBlockMap.end()) {
        // If existed in the map
        returnPtr = recordBlockMap[fileName];
    } else {
        // To open the Block
        std::ifstream fin;
        fin.open(path + fileName, std::ios::in | std::ios::binary);
        if (fin) {
            Block* tmpBlock = getBlockFromRecordBlockPool();
            int tmpReCordNum = 0;
            fin.read((char*)tmpBlock->records, sizeof(tmpBlock->records));
            for (int i=0; i<EACH_BLOCK_RECORDS; i++) {
                if (!tmpBlock->records[i].empty)
                    tmpReCordNum++;
            }
            tmpBlock->recordNum = tmpReCordNum;
            tmpBlock->blockNo = 0;
            tmpBlock->tableName = tableName;
            recordBlockMap[fileName] = tmpBlock;
            returnPtr = tmpBlock;
            fin.close();
        } else {
            // Doesn't exist
        }
    }
    return returnPtr;
}

/**
 * To get the next block (the No. is the current block's block No. plus 1).
 * Return a block pointer pointing to a block which is prepared.
 * @param   blockNoew   block pointer pointing to the curren block
 * @return              block pointer
 */
Block* BufferManager::getNextBlock(Block *blockNow, int mode) {
    Block* returnPtr = NULL;
    std::string path = "table" + SEP + blockNow->tableName + SEP;
    std::string nextBlockFileName = blockNow->tableName + "_" + formatNotoString(blockNow->blockNo + 1);
    if (recordBlockMap.find(nextBlockFileName) != recordBlockMap.end()) {
        // If existed in the map
        returnPtr = recordBlockMap[nextBlockFileName];
    } else {
        // To open the Block
        std::ifstream fin;
        fin.open(path + nextBlockFileName, std::ios::in | std::ios::binary);
        if (fin) {
            Block* tmpBlock = getBlockFromRecordBlockPool();
            int tmpRecordNum = 0;
            fin.read((char*)tmpBlock->records, sizeof(tmpBlock->records));
            for (int i=0; i<EACH_BLOCK_RECORDS; i++) {
                if (!tmpBlock->records[i].empty)
                    tmpRecordNum++;
            }
            tmpBlock->recordNum = tmpRecordNum;
            tmpBlock->tableName = blockNow->tableName;
            tmpBlock->blockNo = blockNow->blockNo + 1;
            recordBlockMap[nextBlockFileName] = tmpBlock;
            returnPtr = tmpBlock;
            fin.close();
        } else {
            // Doesn't exist
            // Need to create the next block file
            if (mode == INSERT_MODE) {
                std::ofstream fout;
                fout.open(path + nextBlockFileName, std::ios::out | std::ios::binary);
                if (fout) {
                    Block* tmpBlock = getBlockFromRecordBlockPool();
                    for (int i=0; i<EACH_BLOCK_RECORDS; i++) {
                        tmpBlock->records[i].empty = true;
                    }
                    fout.write((char*)tmpBlock->records, sizeof(tmpBlock->records));
                    tmpBlock->recordNum = 0;
                    tmpBlock->blockNo = blockNow->blockNo + 1;
                    tmpBlock->tableName = blockNow->tableName;
                    recordBlockMap[nextBlockFileName] = tmpBlock;
                    returnPtr = tmpBlock;
                    fout.close();
                } else {
                    returnPtr = NULL;
                }
            }
        }
    }
    return returnPtr;
}

/**
 * To get a block which includes a record specified by a offset. Return a
 * block pointer pointing to a block which is prepared.
 * @param   offset  a offset of a record used to find a block which
 *                  contains the record
 * @return          block pointer
 */
Block* BufferManager::getBlockByOffset(std::string tableName, int offset) {
    Block* returnPtr = NULL;
    int blockNo = offset / EACH_BLOCK_RECORDS;
    std::string path = "table" + SEP + tableName + SEP;
    std::string fileName = tableName + "_" + formatNotoString(blockNo);
    if (recordBlockMap.find(fileName) != recordBlockMap.end()) {
        // If existed in the map
        returnPtr = recordBlockMap[fileName];
    } else  {
        // To open the Block
        std::ifstream fin;
        fin.open(path + fileName, std::ios::in | std::ios::binary);
        if (fin) {
            Block* tmpBlock = getBlockFromRecordBlockPool();
            int tmpRecordNum = 0;
            fin.read((char*)tmpBlock->records, sizeof(tmpBlock->records));
            for (int i=0; i<EACH_BLOCK_RECORDS; i++) {
                if (!tmpBlock->records[i].empty)
                    tmpRecordNum++;
            }
            tmpBlock->recordNum = tmpRecordNum;
            tmpBlock->tableName = tableName;
            tmpBlock->blockNo = blockNo;
            recordBlockMap[fileName] = tmpBlock;
            returnPtr = tmpBlock;
            fin.close();
        } else {
            // Error: Doesn't exist
            /*// Need to create the next block file
            std::ofstream fout;
            fout.open(fileName, std::ios::out | std::ios::binary);
            if (fout) {
                Block* tmpBlock = getBlockFromRecordBlockPool();
                for (int i=0; i<EACH_BLOCK_RECORDS; i++) {
                    tmpBlock->records[i].empty = true;
                }
                fout.write((char*)tmpBlock->records, sizeof(tmpBlock->records));
                tmpBlock->recordNum = 0;
                tmpBlock->blockNo = blockNo;
                tmpBlock->tableName = tableName;
                recordBlockMap[fileName] = tmpBlock;
                returnPtr = tmpBlock;
                fout.close();
            }*/
        }
    }

    return returnPtr;
}

/**
 * To get a specified index block. Return a index block pointer pointing
 * to a index block which is prepared.
 * @param   tableName   string of a table name
 * @param   attr        string of an attribute
 * @param   blockNo     the No. of the index block
 * @return              index block pointer
 */
IndexBlock* BufferManager::getIndexBlock(std::string tableName, std::string attr, int blockNo) {
    IndexBlock* returnPtr = NULL;
    std::string path = "index" + SEP + tableName + SEP + attr + SEP;
    std::string fileName = tableName + "_" + attr + "_" + formatNotoString(blockNo);
    if (indexBlockMap.find(fileName) != indexBlockMap.end()) {
        // Existed in the map
        returnPtr = indexBlockMap[fileName];
    } else {
        std::ifstream fin;
        fin.open(path + fileName, std::ios::in | std::ios::binary);
        if (fin) {
            IndexBlock* tmpBlock = getBlockFromIndexBlockPool();
            fin.read(tmpBlock->address, 0x1000);
            tmpBlock->tableName = tableName;
            tmpBlock->attrName = attr;
            tmpBlock->blockNo = blockNo;
            tmpBlock->dirty = false;
            tmpBlock->pin = false;
            indexBlockMap[fileName] = tmpBlock;
            returnPtr = tmpBlock;
            fin.close();
        } else {
            if (blockNo == 0) {
                // Create the dir
                std::string mdCommand0 = "index" + SEP + tableName;
                std::string mdCommand1 = path;
#if defined _MSC_VER
                _mkdir(mdCommand0.c_str());
                _mkdir(mdCommand1.c_str());
#elif defined __GNUC__
                mkdir(mdCommand0.c_str(), 0777);
                mkdir(mdCommand1.c_str(), 0777);
#endif
            }
            // Create the file
            std::ofstream fout;
            fout.open(path + fileName, std::ios::out | std::ios::binary);
            fout.close();
            // Get the indexblock
            IndexBlock* tmpBlock = getBlockFromIndexBlockPool();
            memset(tmpBlock->address, 0, 0x1000);
            tmpBlock->tableName = tableName;
            tmpBlock->attrName = attr;
            tmpBlock->blockNo = blockNo;
            tmpBlock->dirty = false;
            tmpBlock->pin = false;
            indexBlockMap[fileName] = tmpBlock;
            returnPtr = tmpBlock;
        }
    }
    return returnPtr;
}

/**
 * To create a new index block (acturlly, find a not used index block sequentially
 * and create it). Return a index block pointer pointing to a index block which
 * is prepared.
 * @param   tableName   string of a table name
 * @param   attr        string of an attribute
 * @return              index block pointer
 */
IndexBlock* BufferManager::getIndexNewBlock(std::string tableName, std::string attr) {
    IndexBlock* returnPtr = NULL;
    std::string path = "index" + SEP + tableName + SEP + attr + SEP;
    for (int i=0; i<MAX_BLOCK_FILE_NUM; i++) {
        std::string tmpFileName = tableName + "_" + attr + "_" + formatNotoString(i);
        std::string tmpFileNameWithDir = path + tmpFileName;
        std::ifstream inFile(tmpFileNameWithDir.c_str());
        // If the file doesn't exist
        if (!inFile.good()) {
            if (i == 0) {
                // Create the dir
                std::string mdCommand0 = "index" + SEP + tableName;
                std::string mdCommand1 = path;
#if defined _MSC_VER
                _mkdir(mdCommand0.c_str());
                _mkdir(mdCommand1.c_str());
#elif defined __GNUC__
                mkdir(mdCommand0.c_str(), 0777);
                mkdir(mdCommand1.c_str(), 0777);
#endif
            }
            // Create the file
            std::ofstream fout;
            fout.open(path + tmpFileName, std::ios::out | std::ios::binary);
            fout.close();
            // Get the indexBlock
            IndexBlock* tmpBlock = getBlockFromIndexBlockPool();
            memset(tmpBlock->address, 0, 0x1000);
            tmpBlock->tableName = tableName;
            tmpBlock->attrName = attr;
            tmpBlock->blockNo = i;
            tmpBlock->dirty = false;
            tmpBlock->pin = false;
            indexBlockMap[tmpFileName] = tmpBlock;
            returnPtr = tmpBlock;

            break;
        }
    }
    return returnPtr;
}

/**
 * delete a specified index block. Return a boolean value, ture is success and
 * false is failure.
 * @param   tableName   string of a table name
 * @param   attr        string of an attribute
 * @param   blockNo     the No. of a inde block
 * @return              result for the operation
 */
bool BufferManager::deleteIndexBlock(std::string tableName, std::string attr, int blockNo) {
    std::string path = "index" + SEP + tableName + SEP + attr + SEP;
    std::string fileName = tableName + "_" + attr + "_" + formatNotoString(blockNo);
    std::map<std::string, IndexBlock*>::iterator it = indexBlockMap.find(fileName);
    if (it != indexBlockMap.end()) {
        IndexBlock* tmpBlock = indexBlockMap[fileName];
        closeIndexBlock(tmpBlock);
    } else {
        // The file haven't been opened
    }
    std::string rmCommand = path + fileName;
    remove(rmCommand.c_str());
    return true;
}
