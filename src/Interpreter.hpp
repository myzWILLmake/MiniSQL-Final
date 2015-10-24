//
//  Interpreter.hpp
//  DB_test
//
//  Created by Frank He on 10/4/15.
//  Copyright © 2015 Frank He. All rights reserved.
//

#ifndef Interpreter_hpp
#define Interpreter_hpp

#include <cstdio>
#include <string>
#include <vector>
using namespace std;
string sqlRead(istream&);
void analyze(string s);

extern bool sqlExit;

//like split() in python, every word will be stripped automatically, which means the word="   " will not be added.
vector<string> split(string s, string sign);

//like strip() in python, reducing the table, space and return
string strip(string s);

#endif /* Interpreter_hpp */
