#ifndef KEYVALUE_H
#define KEYVALUE_H

#include "CommonHeader.hpp"
#include <string>

#define MAX_VALUE 0
#define MIN_VALUE 1

using namespace std;

class KeyValue 
{
public:
	// members storing the vale
	int i;
	float f;
	string s;
	
	// indicating the type of the value
	// -1 : int
	// 0  : float
	// >0 : string, equal the length of string
	int type;

	// ctor with an int
	KeyValue(int init);

	// ctor with an float
	KeyValue(float init);

	// ctor with an string and specific length
	KeyValue(string init, int len);

	// ctor for conversion from Value to KeyValue
	KeyValue(Value v);

	// ctor for getting maximal or minimal value
	// @param 	mode 	must be MIN_VALUE or MAX_VALUE
	KeyValue(int type, int mode);

	// output an KeyValue to memory space  
	void output(char *&p);
};

bool operator<(const KeyValue &v1, const KeyValue &v2);
bool operator>(const KeyValue &v1, const KeyValue &v2);
bool operator>=(const KeyValue &v1, const KeyValue &v2);
bool operator<=(const KeyValue &v1, const KeyValue &v2);
bool operator==(const KeyValue &v1, const KeyValue &v2);
bool operator!=(const KeyValue &v1, const KeyValue &v2);
ostream &operator<<(ostream &os, const KeyValue &v);

#endif // KEYVALUE_H