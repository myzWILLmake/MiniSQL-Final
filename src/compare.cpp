#include "compare.hpp"

bool operator<(const Value &v1, const Value &v2) {
	if (v1.type != v2.type)
	{
		cout << "Type Mismatch: Error occurs when comparing Value objects." << endl;
		exit(0);
	}
	switch (v1.type)
	{
	case -1:
		return v1.Vint < v2.Vint;
	case 0:
		return v1.Vfloat < v2.Vfloat;
	default:
		return v1.Vstring < v2.Vstring;
	}
}
bool operator>(const Value &v1, const Value &v2) {
	return v2 < v1;
}
bool operator>=(const Value &v1, const Value &v2) {
	return !(v1 < v2);
}
bool operator<=(const Value &v1, const Value &v2) {
	return !(v2 < v1);
}
bool operator==(const Value &v1, const Value &v2) {
	return !(v1 < v2) && !(v2 < v1);
}
bool operator!=(const Value &v1, const Value &v2) {
	return (v1 < v2) || (v2 < v1);
}
