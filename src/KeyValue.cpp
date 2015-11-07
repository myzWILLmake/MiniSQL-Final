#include "KeyValue.hpp"
#include <iostream>
#include <string>
#include <climits>
#include <cfloat>

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
	s.resize(len);
}

KeyValue::KeyValue(Value v)
{
    if (v.type == -2)
    {
        cout << "A Value object with unknown type." << endl;
    }
	switch (v.type)
	{
	case -1: i = v.Vint; break;
	case 0: f = v.Vfloat; break;
    default: s = v.Vstring; s.resize(v.type); break;
	}
	type = v.type;
}

KeyValue::KeyValue(int type, int mode)
{
	if (mode == MAX_VALUE)
	{
		switch (type)
		{
		case -1: i = INT_MAX; break;
		case 0: f = FLT_MAX; break;
		default: s = string(type, CHAR_MAX); break;
		}
	}
	else if (mode == MIN_VALUE)
	{
		switch (type)
		{
		case -1: i = INT_MIN; break;
		case 0: f = FLT_MIN; break;
        default: s = string(""); s.resize(type); break;
		}
	}
	this->type = type;
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
