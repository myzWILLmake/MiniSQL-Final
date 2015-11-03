//
//  CommonHeader.hpp
//  DB_test
//
//  Created by Frank He on 10/4/15.
//  Copyright © 2015 Frank He. All rights reserved.
//

#ifndef CommonHeader_hpp
#define CommonHeader_hpp

#include <string>
#include <vector>

using namespace std;
//this Value is used for data transfer, not for storage.
class Value
{
public:
    string Vname;
    int Vint;
    float Vfloat;
    string Vstring;
    int type;   // 1=int 2=float 3=string -1=unknown
    bool unique;
    bool primary;
    string op; //=	<>	<	>	<=	>=
    Value(int type):Vint(0),Vfloat(0)
    {
        this->type=type;
        unique=primary=false;
    }
    Value():Vint(0),Vfloat(0)
    {
        type=-1;
        unique=primary=false;
    }
};

class TransferArguments
{
public:
    string tableName;
    string indexName;
    vector<Value> args;
    string primary_key;
}; 

#endif /* CommonHeader_hpp */
