//
//  main.cpp
//  DB_test
//
//  Created by Frank He on 9/24/15.
//  Copyright © 2015 Frank He. All rights reserved.
//

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <set>
#include "Interpreter.hpp"
#include "CommonHeader.hpp"
#include "API.hpp"


using namespace std;

int main(int argc, const char * argv[]) {
    init();
    cout<<"Welcome to Database System. Author: Huangshi Tian, SHIBI HE, Yunzhe Ma, Wolong Yuan."<<endl;
    while (true) {
        analyze(sqlRead(cin));
    }
    
    return 0;
}

