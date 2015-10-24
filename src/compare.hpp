#pragma once
#include<iostream>
#include "CommonHeader.hpp"
using namespace std;
bool operator<(const Value &v1, const Value &v2);
bool operator>(const Value &v1, const Value &v2);
bool operator>=(const Value &v1, const Value &v2);
bool operator<=(const Value &v1, const Value &v2);
bool operator==(const Value &v1, const Value &v2);
bool operator!=(const Value &v1, const Value &v2);
