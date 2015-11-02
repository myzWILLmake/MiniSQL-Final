//
//  Interpreter.cpp
//  DB_test
//
//  Created by Frank He on 10/4/15.
//  Copyright © 2015 Frank He. All rights reserved.
//

#include "Interpreter.hpp"
#include "CommonHeader.hpp"
#include "API.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <set>
using namespace std;

bool sqlExit=false;

//like strip() in python, reducing the table, space and return
string strip(string s)
{
    //set of table,return,space
    set<char> spaceChar({char(9),char(13),char(32)});
    int pos;
    for (pos=0; pos<s.size(); pos++) {
        if (spaceChar.find(s[pos])==spaceChar.end()) {
            break;
        }
    }
    if (pos==s.size()) {
        return "";
    }
    s.erase(0,pos);
    for (pos=int(s.size()-1); pos>=0; pos--) {
        if (spaceChar.find(s[pos])==spaceChar.end()) {
            break;
        }
    }
    s.erase(pos+1);
    return s;
}

//like split() in python, every word will be stripped automatically, which means the word="   " will not be added.
vector<string> split(string s, string sign)
{
    vector<string> sList;
    while (1) {
        size_t pos=s.find(sign);
        if (pos==string::npos) {
            break;
        }
        //a is pos=2
        string section=s.substr(0,pos);
        s.erase(0,pos+sign.size());
        //section="  abc  ds  " needs strip()
        section=strip(section);
        if (section!="") {
            sList.push_back(section);
        }
    }
    s=strip(s);
    if (s!="") {
        sList.push_back(s);
    }
    return sList;
}

string sqlRead(istream &is)
{
    string inputLine="";
    while (1)
    {
        string s;
        getline(is, s);
        inputLine+=s;
        if (inputLine[inputLine.size()-1]==';')   break;
    }
    return inputLine;
}

void printTransferArguments(TransferArguments transferArg)
{
    cout<<"tableName="<<transferArg.tableName<<endl;
    cout<<"indexName="<<transferArg.indexName<<endl;
    cout<<"primary key="<<transferArg.primary_key<<endl;
    for (vector<Value>::iterator it=transferArg.args.begin(); it!=transferArg.args.end(); it++) {
        cout<<"name="<<it->Vname<<endl;
        cout<<"type="<<it->type<<endl;
        cout<<"int="<<it->Vint<<" float="<<it->Vfloat<<" Vstring="<<it->Vstring<<" op= "<<it->op<<endl;
        cout<<"unique="<<it->unique<<" primary="<<it->primary<<endl<<endl;
    }
}

void analyze(string s)
{
    TransferArguments transferArg;
    if (s.find("create table")==0)
    {
        cout<<"this is create table\n";
        s.erase(0,13);
        size_t pos=s.find("(");
        transferArg.tableName=strip(s.substr(0,pos));
        s.erase(0,pos+1);
        vector<string> tableArgs=split(s, ",");
        for (vector<string>::iterator it=tableArgs.begin(); it!=tableArgs.end(); it++) {
//            cout<<*it<<endl;
            string sLine=*it;
            
            if (sLine.find("primary key")==0) {
                size_t pos1=sLine.find("(");
                size_t pos2=sLine.find(")");
                string name=strip(sLine.substr(pos1+1,pos2-pos1-1));
                transferArg.primary_key=name;
                for (vector<Value>::iterator it_args=transferArg.args.begin(); it_args!=transferArg.args.end(); it_args++) {
                    if ((*it_args).Vname==name) {
                        it_args->unique=true;
                        it_args->primary=true;
                        break;
                    }
                }
                continue;
            }
            
            vector<string> sSec=split(sLine, " ");
            string name=strip(sSec[0]);
            int type=-1;
            if (sSec[1].find("int")==0) {
                type=1;
            }   else
                if (sSec[1].find("float")==0) {
                    type=2;
                }   else
                    if (sSec[1].find("char")==0) {
                        type=3;
                    }
            Value new_value(type);
            new_value.Vname=name;
            if (sLine.find("unique")!=string::npos) {
                new_value.unique=true;
            }
            transferArg.args.push_back(new_value);
        }
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("drop table")==0)
    {
        cout<<"this is drop table\n";
        s.erase(0,10);
        s.erase(s.size()-1,1);
        transferArg.tableName=strip(s);
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("create index")==0) {
        cout<<"this is create index\n";
        s.erase(0,12);
        vector<string> sSec=split(s, "on");
        transferArg.indexName=strip(sSec[0]);
        
        vector<string> sSec1=split(sSec[1], "(");
        transferArg.tableName=strip(sSec1[0]);
        sSec1[1].erase(sSec1[1].size()-2,2);
        
        Value new_element(1);
        new_element.Vname=sSec1[1];
        transferArg.args.push_back(new_element);
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("drop index")==0) {
        cout<<"this is drop index\n";
        s.erase(0,10);
        s.erase(s.size()-1,1);
        transferArg.indexName=strip(s);
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("select")==0) {
        cout<<"this is select\n";
        //delete select * from
        s.erase(0,13);
        
        vector<string> sSec=split(s, "where");
        transferArg.tableName=strip(sSec[0]);
        
        vector<string> conditions=split(sSec[1], "and");
        for (vector<string>::iterator it=conditions.begin(); it!=conditions.end(); it++) {
            //=	<>	<	>	<=	>=
            string ops[6]={"<>","<=",">=","<",">","="}; //order is important
            string op;
            for (int i=0; i<6; i++) {
                op=ops[i];
                if ((*it).find(op)!=string::npos) {
                    break;
                }
            }
            // op is the operator
            vector<string> name_value=split(*it, op);
//            cout<<name_value[0]<<" "<<name_value[1]<<endl;
            string value1=strip(name_value[1]);
            
            Value new_element;
            new_element.Vname=strip(name_value[0]);
            new_element.op=op;
            if (value1[0]=='"' || value1[0]=='\'' || value1[0]=='`') {
                vector<string> sSec_value1=split(value1,string(&value1[0],1));
                new_element.Vstring=sSec_value1[0];
            }   else
            {
                new_element.Vfloat=stod(value1);
                new_element.Vint=stoi(value1);
            }
            transferArg.args.push_back(new_element);
        }
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("insert into")==0) {
        cout<<"this is insert into\n";
        s.erase(0,11);
        vector<string> sSec=split(s, "values");
        transferArg.tableName=strip(sSec[0]);
        sSec[1]=strip(sSec[1]);
        sSec[1]=sSec[1].substr(1,sSec[1].size()-3);
        vector<string> values=split(strip(sSec[1]), ",");
        for (vector<string>::iterator it=values.begin(); it!=values.end(); it++) {
//            cout<<*it<<endl;
            Value new_element;
            if ((*it)[0]=='"' || (*it)[0]=='\'' || (*it)[0]=='`') {
                vector<string> sSec_it=split(*it,string(&((*it)[0]),1));
                new_element.Vstring=sSec_it[0];
            }   else
            {
                new_element.Vfloat=stod(*it);
                new_element.Vint=stoi(*it);
            }
            transferArg.args.push_back(new_element);
        }
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("delete from")==0) {
        cout<<"this is delete from\n";
        s.erase(0,11);
        vector<string> sSec=split(s, "where");
        transferArg.tableName=strip(sSec[0]);
        
        vector<string> conditions=split(sSec[1], "and");
        for (vector<string>::iterator it=conditions.begin(); it!=conditions.end(); it++) {
            //=	<>	<	>	<=	>=
            string ops[6]={"<>","<=",">=","<",">","="}; //order is important
            string op;
            for (int i=0; i<6; i++) {
                op=ops[i];
                if ((*it).find(op)!=string::npos) {
                    break;
                }
            }
            // op is the operator
            vector<string> name_value=split(*it, op);
            //            cout<<name_value[0]<<" "<<name_value[1]<<endl;
            string value1=strip(name_value[1]);
            
            Value new_element;
            new_element.Vname=strip(name_value[0]);
            new_element.op=op;
            if (value1[0]=='"' || value1[0]=='\'' || value1[0]=='`') {
                vector<string> sSec_value1=split(value1,string(&value1[0],1));
                new_element.Vstring=sSec_value1[0];
            }   else
            {
                new_element.Vfloat=stod(value1);
                new_element.Vint=stoi(value1);
            }
            transferArg.args.push_back(new_element);
        }
        
        // start to run API
        printTransferArguments(transferArg);
    }
    if (s.find("execfile")==0) {
        cout<<"this is execfile ";
        s.erase(0,8);
        s.erase(s.size()-1,1);
        string fileName=strip(s);
        cout<<fileName<<endl;
        ifstream readFile;
        readFile.open(fileName);
        while (!readFile.eof()) {
            analyze(sqlRead(readFile));
        }
        cout<<"execfile finished"<<endl;
    }
    if (s.find("quit")==0) {
        cout<<"this is quit\n";
        
        //start to run API
        
        sqlExit=true;
    }
//    cout<<"this is not a sql statement\n";
}