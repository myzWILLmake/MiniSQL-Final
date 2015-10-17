#include "KeyValue.h"
#include <iostream>
#include <string>

using namespace std;

bool operator<(const KeyValue &v1, const KeyValue &v2)
{
	if (v1.type != v2.type)
	{
		cout << "Type Mismatch: Error occurs when comparing KeyValue objects." << endl;
		exit(0);
	}
	switch (v1.type)
	{
	case -1:
		return v1.i < v2.i;
	case 0:
		return v1.f < v2.f;
	default:
		return v1.s < v2.s;
	}
}

bool operator>(const KeyValue &v1, const KeyValue &v2)
{
	return v2 < v1;
}

bool operator>=(const KeyValue &v1, const KeyValue &v2)
{
	return !(v1 < v2);
}

bool operator<=(const KeyValue &v1, const KeyValue &v2)
{
	return !(v2 < v1);
}

bool operator==(const KeyValue &v1, const KeyValue &v2)
{
	return !(v1 < v2) && !(v2 < v1);
}

bool operator!=(const KeyValue &v1, const KeyValue &v2)
{
	return (v1 < v2) || (v2 < v1);
}

ostream &operator<<(ostream &os, const KeyValue &v)
{
	switch (v.type)
	{
	case -1:
		os << v.i;
		return os;
	case 0:
		os << v.f;
		return os;
	default:
		os << v.s;
		return os;
	}
}

KeyValue::KeyValue(int init): i(init), type(-1)
{
}

KeyValue::KeyValue(float init): f(init), type(0)
{
}

KeyValue::KeyValue(string init, int len): s(init), type(len)
{
}

void KeyValue::output(char *&p)
{
	switch (type)
	{
	case -1:
		*((int *)p) = i;
		p += sizeof(int);
		break;
	case 0:
		*((float *)p) = f;
		p += sizeof(float);
		break;
	default:
		for (int j = 0; j < type; j++)
		{
			if (j < s.length())
				*p = s[j];
			else
				*p = 0;
			p++;
		}
		break;
	}
}
