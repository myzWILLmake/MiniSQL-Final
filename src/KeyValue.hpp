#ifndef KEYVALUE_H
#define KEYVALUE_H

#include <string>

using namespace std;

class KeyValue 
{
public:
	int i;
	float f;
	string s;
	int type;

	KeyValue(int init);
	KeyValue(float init);
	KeyValue(string init, int len);

	void output(char *&p);
};

bool operator<(const KeyValue &v1, const KeyValue &v2);
bool operator>(const KeyValue &v1, const KeyValue &v2);
bool operator>=(const KeyValue &v1, const KeyValue &v2);
bool operator<=(const KeyValue &v1, const KeyValue &v2);
bool operator==(const KeyValue &v1, const KeyValue &v2);
bool operator!=(const KeyValue &v1, const KeyValue &v2);
ostream &operator<<(ostream &os, const KeyValue &v);

#endif