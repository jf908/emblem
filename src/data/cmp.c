#include "cmp.h"

#include "data/str.h"
#include "pp/ignore_warning.h"
#include <string.h>

#define CMP(name, type)                                                                                                \
	Cmp cmp_##name##s(void* v1, void* v2)                                                                              \
	{                                                                                                                  \
		TYPE_PUN_DEREFERENCE(type p1 = *(type*)&v1);                                                                   \
		TYPE_PUN_DEREFERENCE(type p2 = *(type*)&v2);                                                                   \
		return p1 < p2 ? LT : p1 == p2 ? EQ : GT;                                                                      \
	}

CMP(char, char)
CMP(double, double)
CMP(float, float)
CMP(int, int)
CMP(size_t, size_t)
CMP(ptr, void*)

Cmp cmp_strs(void* v1, void* v2)
{
	Str* s1 = v1;
	Str* s2 = v2;
	return strcmp(s1->str, s2->str);
}

